#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <float.h>
#include <math.h>

typedef struct {
    char *name;
    size_t values;
    size_t missing;
    size_t numeric;
    double min;
    double max;
    double mean;
} CsvColumn;

typedef struct {
    CsvColumn *columns;
    size_t count;
} CsvReport;

void csv_report_init(CsvReport *report) {
    if (report) {
        report->columns = NULL;
        report->count = 0;
    }
}

void csv_report_free(CsvReport *report) {
    size_t i;

    if (!report) return;

    for (i = 0; i < report->count; ++i)
        free(report->columns[i].name);

    free(report->columns);
    csv_report_init(report);
}

static char *duplicate_range(const char *start, size_t length) {
    char *result = malloc(length + 1);

    if (!result) return NULL;

    memcpy(result, start, length);
    result[length] = '\0';
    return result;
}

static int parse_csv_record(
    const char *line,
    char ***fields_out,
    size_t *count_out
) {
    const char *p = line;
    char **fields = NULL;
    size_t count = 0;

    while (*p) {
        char *field = NULL;
        size_t capacity = 32;
        size_t length = 0;
        int quoted = (*p == '"');

        field = malloc(capacity);
        if (!field) goto fail;

        if (quoted) ++p;

        while (*p) {
            if (quoted) {
                if (*p == '"') {
                    if (p[1] == '"') {
                        if (length + 1 >= capacity) {
                            capacity *= 2;
                            field = realloc(field, capacity);
                            if (!field) goto fail;
                        }
                        field[length++] = '"';
                        p += 2;
                    } else {
                        ++p;
                        quoted = 0;
                        while (*p == ' ') ++p;
                        if (*p && *p != ',') {
                            free(field);
                            goto fail;
                        }
                    }
                } else {
                    if (length + 1 >= capacity) {
                        capacity *= 2;
                        field = realloc(field, capacity);
                        if (!field) goto fail;
                    }
                    field[length++] = *p++;
                }
            } else if (*p == ',') {
                break;
            } else if (*p == '\r' || *p == '\n') {
                break;
            } else {
                if (length + 1 >= capacity) {
                    capacity *= 2;
                    field = realloc(field, capacity);
                    if (!field) goto fail;
                }
                field[length++] = *p++;
            }
        }

        field[length] = '\0';

        {
            char **grown = realloc(fields, (count + 1) * sizeof(*fields));
            if (!grown) {
                free(field);
                goto fail;
            }
            fields = grown;
            fields[count++] = field;
        }

        if (*p == ',') {
            ++p;
            if (!*p) {
                char **grown = realloc(fields, (count + 1) * sizeof(*fields));
                if (!grown) goto fail;
                fields = grown;
                fields[count] = duplicate_range("", 0);
                if (!fields[count]) goto fail;
                ++count;
            }
        } else {
            break;
        }
    }

    if (!count) {
        fields = malloc(sizeof(*fields));
        if (!fields) return 0;
        fields[0] = duplicate_range("", 0);
        if (!fields[0]) {
            free(fields);
            return 0;
        }
        count = 1;
    }

    *fields_out = fields;
    *count_out = count;
    return 1;

fail:
    if (fields) {
        size_t i;
        for (i = 0; i < count; ++i)
            free(fields[i]);
        free(fields);
    }
    return 0;
}

static void free_fields(char **fields, size_t count) {
    size_t i;
    for (i = 0; i < count; ++i)
        free(fields[i]);
    free(fields);
}

static int parse_number(const char *text, double *value) {
    char *end;
    double result;

    while (*text == ' ' || *text == '\t')
        ++text;

    if (!*text) return 0;

    errno = 0;
    result = strtod(text, &end);

    while (*end == ' ' || *end == '\t')
        ++end;

    if (errno == ERANGE || *end != '\0' || !isfinite(result))
        return 0;

    *value = result;
    return 1;
}

static int initialize_columns(
    CsvReport *report,
    char **headers,
    size_t count
) {
    size_t i;

    report->columns = calloc(count, sizeof(*report->columns));
    if (!report->columns) return 0;

    report->count = count;

    for (i = 0; i < count; ++i) {
        report->columns[i].name = strdup(headers[i]);
        if (!report->columns[i].name) {
            csv_report_free(report);
            return 0;
        }
    }

    return 1;
}

int csv_parse_file(FILE *input, CsvReport *report) {
    char *line = NULL;
    size_t capacity = 0;
    ssize_t length;
    int first_record = 1;

    if (!input || !report)
        return 0;

    csv_report_init(report);

    while ((length = getline(&line, &capacity, input)) != -1) {
        char **fields = NULL;
        size_t field_count = 0;
        size_t i;

        while (length > 0 &&
               (line[length - 1] == '\n' || line[length - 1] == '\r'))
            line[--length] = '\0';

        if (!parse_csv_record(line, &fields, &field_count))
            goto fail;

        if (first_record) {
            if (!initialize_columns(report, fields, field_count)) {
                free_fields(fields, field_count);
                goto fail;
            }
            first_record = 0;
        } else {
            for (i = 0; i < report->count; ++i) {
                CsvColumn *column = &report->columns[i];
                const char *text = i < field_count ? fields[i] : "";
                double number;

                column->values++;

                if (!*text) {
                    column->missing++;
                } else if (parse_number(text, &number)) {
                    column->numeric++;

                    if (column->numeric == 1) {
                        column->min = number;
                        column->max = number;
                        column->mean = number;
                    } else {
                        if (number < column->min) column->min = number;
                        if (number > column->max) column->max = number;
                        column->mean +=
                            (number - column->mean) / column->numeric;
                    }
                }
            }
        }

        free_fields(fields, field_count);
    }

    free(line);
    return !ferror(input);

fail:
    free(line);
    csv_report_free(report);
    return 0;
}

#ifdef CSV_STATS_MAIN
int main(int argc, char **argv) {
    FILE *input;
    CsvReport report;
    size_t i;

    if (argc != 2) {
        fprintf(stderr, "usage: %s file.csv\n", argv[0]);
        return EXIT_FAILURE;
    }

    input = fopen(argv[1], "r");
    if (!input) {
        perror(argv[1]);
        return EXIT_FAILURE;
    }

    if (!csv_parse_file(input, &report)) {
        fclose(input);
        fprintf(stderr, "failed to parse CSV\n");
        return EXIT_FAILURE;
    }

    fclose(input);

    for (i = 0; i < report.count; ++i) {
        const CsvColumn *column = &report.columns[i];

        printf("%s: values=%zu missing=%zu numeric=%zu",
               column->name,
               column->values,
               column->missing,
               column->numeric);

        if (column->numeric) {
            printf(" min=%.17g max=%.17g mean=%.17g",
                   column->min,
                   column->max,
                   column->mean);
        }

        putchar('\n');
    }

    csv_report_free(&report);
    return EXIT_SUCCESS;
}
#endif
