#include <stddef.h>
#include <stdlib.h>

typedef struct {
    size_t bucket_count;
    const double *boundaries;
    size_t *counts;
} Histogram;

/*
 * boundaries contains bucket_count + 1 ascending values.
 * Buckets are:
 *   [boundaries[i], boundaries[i + 1]) for i < bucket_count - 1
 *   [boundaries[i], boundaries[i + 1]] for the last bucket
 */
int histogram_init(Histogram *histogram,
                   const double *boundaries,
                   size_t bucket_count)
{
    size_t i;

    if (histogram == NULL || boundaries == NULL || bucket_count == 0) {
        return 0;
    }

    for (i = 0; i < bucket_count; ++i) {
        if (boundaries[i] >= boundaries[i + 1]) {
            return 0;
        }
    }

    histogram->counts = calloc(bucket_count, sizeof(*histogram->counts));
    if (histogram->counts == NULL) {
        return 0;
    }

    histogram->bucket_count = bucket_count;
    histogram->boundaries = boundaries;
    return 1;
}

void histogram_reset(Histogram *histogram)
{
    if (histogram == NULL || histogram->counts == NULL) {
        return;
    }

    for (size_t i = 0; i < histogram->bucket_count; ++i) {
        histogram->counts[i] = 0;
    }
}

void histogram_free(Histogram *histogram)
{
    if (histogram == NULL) {
        return;
    }

    free(histogram->counts);
    histogram->counts = NULL;
    histogram->boundaries = NULL;
    histogram->bucket_count = 0;
}

int histogram_add(Histogram *histogram, double value)
{
    size_t left = 0;
    size_t right;

    if (histogram == NULL || histogram->counts == NULL ||
        histogram->bucket_count == 0) {
        return 0;
    }

    if (value < histogram->boundaries[0] ||
        value > histogram->boundaries[histogram->bucket_count]) {
        return 0;
    }

    right = histogram->bucket_count;

    while (left < right) {
        size_t middle = left + (right - left) / 2;

        if (value < histogram->boundaries[middle + 1]) {
            right = middle;
        } else {
            left = middle + 1;
        }
    }

    if (left >= histogram->bucket_count) {
        left = histogram->bucket_count - 1;
    }

    histogram->counts[left]++;
    return 1;
}

size_t histogram_build(Histogram *histogram,
                       const double *values,
                       size_t value_count)
{
    size_t added = 0;

    if (histogram == NULL || (values == NULL && value_count != 0)) {
        return 0;
    }

    histogram_reset(histogram);

    for (size_t i = 0; i < value_count; ++i) {
        if (histogram_add(histogram, values[i])) {
            ++added;
        }
    }

    return added;
}
