#include <ctype.h>
#include <errno.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

int parse_duration(const char *input, double *seconds)
{
    const char *p;
    char *end;
    double total = 0.0;

    if (input == NULL || seconds == NULL)
        return 0;

    p = input;
    while (isspace((unsigned char)*p))
        p++;

    if (*p == '\0' || *p == '-')
        return 0;

    while (*p != '\0') {
        double value;
        double multiplier;

        errno = 0;
        value = strtod(p, &end);

        if (end == p || errno == ERANGE || value < 0.0)
            return 0;

        p = end;

        if (*p == 'h' || *p == 'H') {
            multiplier = 3600.0;
        } else if (*p == 'm' || *p == 'M') {
            multiplier = 60.0;
        } else if (*p == 's' || *p == 'S') {
            multiplier = 1.0;
        } else {
            return 0;
        }

        if (value > (1.7976931348623157e308 - total) / multiplier)
            return 0;

        total += value * multiplier;
        p++;

        while (isspace((unsigned char)*p))
            p++;
    }

    *seconds = total;
    return 1;
}
