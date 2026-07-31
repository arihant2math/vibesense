#include <stdint.h>
#include <stdbool.h>

#define DECIMAL_SCALE INT64_C(1000000)

typedef struct {
    int64_t value;
} decimal_t;

static decimal_t decimal_from_int64(int64_t integer)
{
    decimal_t result;
    result.value = integer * DECIMAL_SCALE;
    return result;
}

static decimal_t decimal_from_scaled(int64_t scaled_value)
{
    decimal_t result;
    result.value = scaled_value;
    return result;
}

static int64_t decimal_to_scaled(decimal_t value)
{
    return value.value;
}

static bool decimal_add(decimal_t left, decimal_t right, decimal_t *result)
{
    __int128 sum;

    if (result == NULL) {
        return false;
    }

    sum = (__int128)left.value + right.value;

    if (sum > INT64_MAX || sum < INT64_MIN) {
        return false;
    }

    result->value = (int64_t)sum;
    return true;
}

static bool decimal_multiply(decimal_t left, decimal_t right, decimal_t *result)
{
    __int128 product;
    __int128 scaled_product;

    if (result == NULL) {
        return false;
    }

    product = (__int128)left.value * right.value;
    scaled_product = product / DECIMAL_SCALE;

    if (scaled_product > INT64_MAX || scaled_product < INT64_MIN) {
        return false;
    }

    result->value = (int64_t)scaled_product;
    return true;
}
