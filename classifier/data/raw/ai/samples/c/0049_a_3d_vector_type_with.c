#include <stdbool.h>
#include <stdint.h>
#include <limits.h>

typedef struct {
    int64_t x;
    int64_t y;
    int64_t z;
} Vec3;

static bool add_i64(int64_t a, int64_t b, int64_t *result)
{
    if (!result) return false;

    if ((b > 0 && a > INT64_MAX - b) ||
        (b < 0 && a < INT64_MIN - b))
        return false;

    *result = a + b;
    return true;
}

static bool sub_i64(int64_t a, int64_t b, int64_t *result)
{
    if (!result) return false;

    if ((b > 0 && a < INT64_MIN + b) ||
        (b < 0 && a > INT64_MAX + b))
        return false;

    *result = a - b;
    return true;
}

static bool mul_i64(int64_t a, int64_t b, int64_t *result)
{
    if (!result) return false;

    if (a == 0 || b == 0) {
        *result = 0;
        return true;
    }

    if (a == -1) {
        if (b == INT64_MIN) return false;
        *result = -b;
        return true;
    }

    if (b == -1) {
        if (a == INT64_MIN) return false;
        *result = -a;
        return true;
    }

    if (a > 0) {
        if (b > 0) {
            if (a > INT64_MAX / b) return false;
        } else {
            if (b < INT64_MIN / a) return false;
        }
    } else {
        if (b > 0) {
            if (a < INT64_MIN / b) return false;
        } else {
            if (a < INT64_MAX / b) return false;
        }
    }

    *result = a * b;
    return true;
}

bool vec3_dot(const Vec3 *a, const Vec3 *b, int64_t *result)
{
    int64_t p1, p2, p3, sum;

    if (!a || !b || !result) return false;
    if (!mul_i64(a->x, b->x, &p1)) return false;
    if (!mul_i64(a->y, b->y, &p2)) return false;
    if (!mul_i64(a->z, b->z, &p3)) return false;
    if (!add_i64(p1, p2, &sum)) return false;
    if (!add_i64(sum, p3, result)) return false;

    return true;
}

bool vec3_cross(const Vec3 *a, const Vec3 *b, Vec3 *result)
{
    Vec3 out;
    int64_t p1, p2;

    if (!a || !b || !result) return false;

    if (!mul_i64(a->y, b->z, &p1) ||
        !mul_i64(a->z, b->y, &p2) ||
        !sub_i64(p1, p2, &out.x))
        return false;

    if (!mul_i64(a->z, b->x, &p1) ||
        !mul_i64(a->x, b->z, &p2) ||
        !sub_i64(p1, p2, &out.y))
        return false;

    if (!mul_i64(a->x, b->y, &p1) ||
        !mul_i64(a->y, b->x, &p2) ||
        !sub_i64(p1, p2, &out.z))
        return false;

    *result = out;
    return true;
}
