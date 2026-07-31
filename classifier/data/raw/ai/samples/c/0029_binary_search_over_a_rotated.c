#include <stdio.h>
#include <stddef.h>

int binary_search_rotated(const int array[], size_t length, int target) {
    size_t left = 0;
    size_t right = length;

    while (left < right) {
        size_t mid = left + (right - left) / 2;

        if (array[mid] == target) {
            return (int)mid;
        }

        if (array[left] <= array[mid]) {
            if (array[left] <= target && target < array[mid]) {
                right = mid;
            } else {
                left = mid + 1;
            }
        } else {
            if (array[mid] < target && target <= array[right - 1]) {
                left = mid + 1;
            } else {
                right = mid;
            }
        }
    }

    return -1;
}

int main(void) {
    int array[] = {4, 5, 6, 7, 0, 1, 2};
    size_t length = sizeof(array) / sizeof(array[0]);
    int target = 0;

    int index = binary_search_rotated(array, length, target);

    if (index >= 0) {
        printf("Target %d found at index %d\n", target, index);
    } else {
        printf("Target %d not found\n", target);
    }

    return 0;
}
