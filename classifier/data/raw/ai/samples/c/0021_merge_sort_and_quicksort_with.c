#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

typedef int (*compare_fn)(const void *, const void *);

enum {
    SORT_OK = 0,
    SORT_INVALID_ARGUMENT = 1,
    SORT_ALLOCATION_FAILED = 2,
    SORT_SIZE_OVERFLOW = 3
};

static void merge_ranges(unsigned char *base,
                          unsigned char *temp,
                          size_t left,
                          size_t mid,
                          size_t right,
                          size_t size,
                          compare_fn compare)
{
    size_t i = left;
    size_t j = mid;
    size_t k = left;

    while (i < mid && j < right) {
        unsigned char *a = base + i * size;
        unsigned char *b = base + j * size;

        if (compare(a, b) <= 0) {
            memcpy(temp + k * size, a, size);
            ++i;
        } else {
            memcpy(temp + k * size, b, size);
            ++j;
        }
        ++k;
    }

    while (i < mid) {
        memcpy(temp + k * size, base + i * size, size);
        ++i;
        ++k;
    }

    while (j < right) {
        memcpy(temp + k * size, base + j * size, size);
        ++j;
        ++k;
    }

    memcpy(base + left * size, temp + left * size, (right - left) * size);
}

static void merge_sort_recursive(unsigned char *base,
                                 unsigned char *temp,
                                 size_t left,
                                 size_t right,
                                 size_t size,
                                 compare_fn compare)
{
    if (right - left < 2) {
        return;
    }

    size_t mid = left + (right - left) / 2;

    merge_sort_recursive(base, temp, left, mid, size, compare);
    merge_sort_recursive(base, temp, mid, right, size, compare);
    merge_ranges(base, temp, left, mid, right, size, compare);
}

int merge_sort(void *array, size_t count, size_t element_size, compare_fn compare)
{
    if (compare == NULL || element_size == 0 || (count != 0 && array == NULL)) {
        return SORT_INVALID_ARGUMENT;
    }

    if (count == 0) {
        return SORT_OK;
    }

    if (count > SIZE_MAX / element_size) {
        return SORT_SIZE_OVERFLOW;
    }

    unsigned char *temp = malloc(count * element_size);
    if (temp == NULL) {
        return SORT_ALLOCATION_FAILED;
    }

    merge_sort_recursive((unsigned char *)array, temp, 0, count,
                         element_size, compare);

    free(temp);
    return SORT_OK;
}

static void swap_elements(unsigned char *a, unsigned char *b,
                          unsigned char *temp, size_t size)
{
    if (a == b) {
        return;
    }

    memcpy(temp, a, size);
    memcpy(a, b, size);
    memcpy(b, temp, size);
}

static size_t partition_range(unsigned char *base,
                              size_t count,
                              size_t size,
                              compare_fn compare,
                              unsigned char *temp)
{
    size_t pivot_index = count - 1;
    unsigned char *pivot = base + pivot_index * size;
    size_t store = 0;

    for (size_t i = 0; i < pivot_index; ++i) {
        if (compare(base + i * size, pivot) < 0) {
            swap_elements(base + store * size,
                          base + i * size,
                          temp, size);
            ++store;
        }
    }

    swap_elements(base + store * size, pivot, temp, size);
    return store;
}

static void quick_sort_recursive(unsigned char *base,
                                  size_t count,
                                  size_t size,
                                  compare_fn compare,
                                  unsigned char *temp)
{
    if (count < 2) {
        return;
    }

    size_t pivot = partition_range(base, count, size, compare, temp);

    quick_sort_recursive(base, pivot, size, compare, temp);

    if (pivot + 1 < count) {
        quick_sort_recursive(base + (pivot + 1) * size,
                             count - pivot - 1,
                             size, compare, temp);
    }
}

int quick_sort(void *array, size_t count, size_t element_size, compare_fn compare)
{
    if (compare == NULL || element_size == 0 || (count != 0 && array == NULL)) {
        return SORT_INVALID_ARGUMENT;
    }

    if (count == 0) {
        return SORT_OK;
    }

    if (count > SIZE_MAX / element_size) {
        return SORT_SIZE_OVERFLOW;
    }

    unsigned char *temp = malloc(element_size);
    if (temp == NULL) {
        return SORT_ALLOCATION_FAILED;
    }

    quick_sort_recursive((unsigned char *)array, count, element_size,
                         compare, temp);

    free(temp);
    return SORT_OK;
}
