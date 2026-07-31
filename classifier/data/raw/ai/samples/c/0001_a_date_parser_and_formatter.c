#include <stddef.h>
#include <stdint.h>

typedef struct {
    int year;
    unsigned month, day;
    unsigned hour, minute, second;
    unsigned nanosecond;
    int tz_offset_minutes;
} iso8601_time;

static int is_digit(char c) {
    return c >= '0' && c <= '9';
}

static int read_digits(const char **p, const char *end, unsigned n, unsigned *out) {
    unsigned v = 0;
    if ((size_t)(end - *p) < n) return 0;
    for (unsigned i = 0; i < n; ++i) {
        if (!is_digit((*p)[i])) return 0;
        v = v * 10u + (unsigned)((*p)[i] - '0');
    }
    *p += n;
    *out = v;
    return 1;
}

static int leap_year(int year) {
    return (year % 4 == 0 && year % 100 != 0) || year % 400 == 0;
}

static unsigned days_in_month(int year, unsigned month) {
    static const unsigned days[] = {
        31, 28, 31, 30, 31, 30,
        31, 31, 30, 31, 30, 31
    };
    if (month == 2) return days[1] + (unsigned)leap_year(year);
    return days[month - 1];
}

int iso8601_parse(const char *s, size_t length, iso8601_time *out) {
    const char *p = s, *end;
    unsigned v, fraction_digits = 0;
    int sign, tz_hour, tz_minute;

    if (!s || !out) return 0;
    end = s + length;

    if (!read_digits(&p, end, 4, &v)) return 0;
    out->year = (int)v;
    if (p >= end || *p++ != '-') return 0;
    if (!read_digits(&p, end, 2, &out->month)) return 0;
    if (p >= end || *p++ != '-') return 0;
    if (!read_digits(&p, end, 2, &out->day)) return 0;
    if (p >= end || (*p != 'T' && *p != 't' && *p != ' ')) return 0;
    ++p;
    if (!read_digits(&p, end, 2, &out->hour)) return 0;
    if (p >= end || *p++ != ':') return 0;
    if (!read_digits(&p, end, 2, &out->minute)) return 0;
    if (p >= end || *p++ != ':') return 0;
    if (!read_digits(&p, end, 2, &out->second)) return 0;

    out->nanosecond = 0;
    if (p < end && (*p == '.' || *p == ',')) {
        unsigned scale = 100000000;
        ++p;
        while (p < end && is_digit(*p)) {
            if (fraction_digits < 9)
                out->nanosecond += (unsigned)(*p - '0') * scale;
            if (fraction_digits < 9) scale /= 10;
            ++fraction_digits;
            ++p;
        }
        if (fraction_digits == 0) return 0;
    }

    if (p >= end) return 0;
    if (*p == 'Z' || *p == 'z') {
        out->tz_offset_minutes = 0;
        ++p;
    } else {
        if (*p != '+' && *p != '-') return 0;
        sign = (*p++ == '+') ? 1 : -1;
        if (!read_digits(&p, end, 2, (unsigned *)&tz_hour)) return 0;
        if (p >= end || *p++ != ':') return 0;
        if (!read_digits(&p, end, 2, (unsigned *)&tz_minute)) return 0;
        if (tz_hour > 23 || tz_minute > 59) return 0;
        out->tz_offset_minutes = sign * (tz_hour * 60 + tz_minute);
    }

    if (p != end ||
        out->month < 1 || out->month > 12 ||
        out->day < 1 || out->day > days_in_month(out->year, out->month) ||
        out->hour > 23 || out->minute > 59 || out->second > 59)
        return 0;

    return 1;
}

static size_t put2(char *p, unsigned v) {
    p[0] = (char)('0' + (v / 10) % 10);
    p[1] = (char)('0' + v % 10);
    return 2;
}

static size_t put4(char *p, unsigned v) {
    p[0] = (char)('0' + (v / 1000) % 10);
    p[1] = (char)('0' + (v / 100) % 10);
    p[2] = (char)('0' + (v / 10) % 10);
    p[3] = (char)('0' + v % 10);
    return 4;
}

size_t iso8601_format(const iso8601_time *t, char *buf, size_t capacity) {
    char tmp[32];
    size_t n = 0;
    int offset, h, m;

    if (!t || !buf) return 0;
    if (t->year < 0 || t->year > 9999 ||
        t->month < 1 || t->month > 12 ||
        t->day < 1 || t->day > days_in_month(t->year, t->month) ||
        t->hour > 23 || t->minute > 59 || t->second > 59 ||
        t->nanosecond > 999999999)
        return 0;

    n += put4(tmp + n, (unsigned)t->year);
    tmp[n++] = '-';
    n += put2(tmp + n, t->month);
    tmp[n++] = '-';
    n += put2(tmp + n, t->day);
    tmp[n++] = 'T';
    n += put2(tmp + n, t->hour);
    tmp[n++] = ':';
    n += put2(tmp + n, t->minute);
    tmp[n++] = ':';
    n += put2(tmp + n, t->second);

    if (t->nanosecond) {
        unsigned ns = t->nanosecond;
        unsigned digits = 9;
        while (digits > 0 && ns % 10 == 0) {
            ns /= 10;
            --digits;
        }
        tmp[n++] = '.';
        for (unsigned i = digits; i > 0; --i)
            tmp[n++] = (char)('0' + (ns / (unsigned)1));
        {
            unsigned div = 1;
            for (unsigned i = 1; i < digits; ++i) div *= 10;
            for (unsigned i = 0; i < digits; ++i) {
                tmp[n - digits + i] = (char)('0' + (t->nanosecond / div) % 10);
                div /= 10;
            }
        }
    }

    offset = t->tz_offset_minutes;
    if (offset == 0) {
        tmp[n++] = 'Z';
    } else {
        tmp[n++] = offset < 0 ? '-' : '+';
        if (offset < 0) offset = -offset;
        h = offset / 60;
        m = offset % 60;
        n += put2(tmp + n, (unsigned)h);
        tmp[n++] = ':';
        n += put2(tmp + n, (unsigned)m);
    }

    if (capacity <= n) return n + 1;
    for (size_t i = 0; i < n; ++i) buf[i] = tmp[i];
    buf[n] = '\0';
    return n;
}
