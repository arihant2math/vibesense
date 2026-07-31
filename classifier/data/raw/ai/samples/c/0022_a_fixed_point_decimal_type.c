#include <stdint.h>
#include <stdbool.h>

#define FIXED_DECIMAL_SCALE INT64_C(1000000)

typedef struct {
    int64_t raw;
} fixed_decimal_t;

static inline fixed_decimal_t fixed_decimal_from_raw(int64_t raw)
{
    fixed_decimal_t value = { raw };
    return value;
}

static bool fixed_decimal_from_int(int64_t integer, fixed_decimal_t *result)
{
    __int128 scaled = (__int128)integer * FIXED_DECIMAL_SCALE;

    if (scaled > INT64_MAX || scaled < INT64_MIN) {
        return false;
    }

    result->raw = (int64_t)scaled;
    return true;
}

static inline int64_t fixed_decimal_raw(fixed_decimal_t value)
{
    return value.raw;
}

static bool fixed_decimal_add(
    fixed_decimal_t left,
    fixed_decimal_t right,
    fixed_decimal_t *result)
{
    __int128 sum = (__int128)left.raw + right.raw;

    if (sum > INT64_MAX || sum < INT64_MIN) {
        return false;
    }

    result->raw = (int64_t)sum;
    return true;
}

static bool fixed_decimal_multiply(
    fixed_decimal_t left,
    fixed_decimal_t right,
    fixed_decimal_t *result)
{
    __int128 product = (__int128)left.raw * right.raw;
    __int128 scaled = product / FIXED_DECIMAL_SCALE;

    if (scaled > INT64_MAX || scaled < INT64_MIN) {
        return false;
    }

    result->raw = (int64_t)scaled;
    return true;
}
