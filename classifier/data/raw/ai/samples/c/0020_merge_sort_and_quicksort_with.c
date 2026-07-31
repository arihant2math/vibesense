#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>

typedef int (*compare_fn)(const void *, const void *);

static void merge(void *base, void *temp, size_t left, size_t mid, size_t right,
                  size_t size, compare_fn compare)
{
    char *array = (char *)base;
    char *buffer = (char *)temp;
    size_t i = left;
    size_t j = mid;
    size_t k = left;

    while (i < mid && j < right) {
        if (compare(array + i * size, array + j * size) <= 0) {
            memcpy(buffer + k * size, array + i * size, size);
            i++;
        } else {
            memcpy(buffer + k * size, array + j * size, size);
            j++;
        }
        k++;
    }

    while (i < mid) {
        memcpy(buffer + k * size, array + i * size, size);
        i++;
        k++;
    }

    while (j < right) {
        memcpy(buffer + k * size, array + j * size, size);
        j++;
        k++;
    }

    for (k = left; k < right; k++)
        memcpy(array + k * size, buffer + k * size, size);
}

static void merge_sort_recursive(void *base, void *temp, size_t left, size_t right,
                                 size_t size, compare_fn compare)
{
    if (right - left < 2)
        return;

    size_t mid = left + (right - left) / 2;
    merge_sort_recursive(base, temp, left, mid, size, compare);
    merge_sort_recursive(base, temp, mid, right, size, compare);
    merge(base, temp, left, mid, right, size, compare);
}

int merge_sort(void *base, size_t count, size_t size, compare_fn compare)
{
    if (count < 2)
        return 0;

    if (!base || size == 0 || !compare)
        return -1;

    void *temp = malloc(count * size);
    if (!temp)
        return -1;

    merge_sort_recursive(base, temp, 0, count, size, compare);
    free(temp);
    return 0;
}

static void swap_elements(char *a, char *b, size_t size)
{
    while (size--) {
        char temp = *a;
        *a++ = *b;
        *b++ = temp;
    }
}

static size_t partition_array(char *base, size_t low, size_t high, size_t size,
                              compare_fn compare)
{
    char *pivot = base + high * size;
    size_t i = low;

    for (size_t j = low; j < high; j++) {
        if (compare(base + j * size, pivot) <= 0) {
            swap_elements(base + i * size, base + j * size, size);
            i++;
        }
    }

    swap_elements(base + i * size, base + high * size, size);
    return i;
}

static void quick_sort_recursive(char *base, size_t low, size_t high, size_t size,
                                 compare_fn compare)
{
    if (low >= high)
        return;

    size_t pivot = partition_array(base, low, high, size, compare);

    if (pivot > low)
        quick_sort_recursive(base, low, pivot - 1, size, compare);
    if (pivot < high)
        quick_sort_recursive(base, pivot + 1, high, size, compare);
}

int quick_sort(void *base, size_t count, size_t size, compare_fn compare)
{
    if (count < 2)
        return 0;

    if (!base || size == 0 || !compare)
        return -1;

    quick_sort_recursive((char *)base, 0, count - 1, size, compare);
    return 0;
}
