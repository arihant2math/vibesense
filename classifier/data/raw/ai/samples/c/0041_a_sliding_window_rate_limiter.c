#include <stdint.h>
#include <stdlib.h>

typedef struct {
    uint64_t *timestamps;
    uint64_t window_ns;
    size_t capacity;
    size_t head;
    size_t count;
} rate_limiter_t;

int rate_limiter_init(rate_limiter_t *rl, size_t max_events, uint64_t window_ns)
{
    if (!rl || max_events == 0 || window_ns == 0)
        return 0;

    rl->timestamps = calloc(max_events, sizeof(*rl->timestamps));
    if (!rl->timestamps)
        return 0;

    rl->window_ns = window_ns;
    rl->capacity = max_events;
    rl->head = 0;
    rl->count = 0;
    return 1;
}

void rate_limiter_destroy(rate_limiter_t *rl)
{
    if (!rl)
        return;

    free(rl->timestamps);
    rl->timestamps = NULL;
    rl->window_ns = 0;
    rl->capacity = 0;
    rl->head = 0;
    rl->count = 0;
}

int rate_limiter_allow(rate_limiter_t *rl, uint64_t now_ns)
{
    uint64_t cutoff;

    if (!rl || !rl->timestamps || rl->capacity == 0)
        return 0;

    cutoff = now_ns > rl->window_ns ? now_ns - rl->window_ns : 0;

    while (rl->count > 0 &&
           rl->timestamps[rl->head] <= cutoff) {
        rl->head = (rl->head + 1) % rl->capacity;
        --rl->count;
    }

    if (rl->count >= rl->capacity)
        return 0;

    rl->timestamps[(rl->head + rl->count) % rl->capacity] = now_ns;
    ++rl->count;
    return 1;
}
