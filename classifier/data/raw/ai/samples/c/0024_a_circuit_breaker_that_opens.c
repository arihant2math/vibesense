#include <stdbool.h>
#include <stddef.h>
#include <time.h>

typedef enum {
    CIRCUIT_CLOSED,
    CIRCUIT_OPEN,
    CIRCUIT_HALF_OPEN
} CircuitState;

typedef struct {
    CircuitState state;
    unsigned int failure_count;
    unsigned int failure_threshold;
    unsigned int reset_timeout_seconds;
    time_t opened_at;
} CircuitBreaker;

void circuit_breaker_init(CircuitBreaker *breaker,
                          unsigned int failure_threshold,
                          unsigned int reset_timeout_seconds)
{
    if (!breaker) {
        return;
    }

    breaker->state = CIRCUIT_CLOSED;
    breaker->failure_count = 0;
    breaker->failure_threshold = failure_threshold;
    breaker->reset_timeout_seconds = reset_timeout_seconds;
    breaker->opened_at = 0;
}

bool circuit_breaker_allow_request(CircuitBreaker *breaker)
{
    time_t now;

    if (!breaker) {
        return false;
    }

    if (breaker->state == CIRCUIT_CLOSED) {
        return true;
    }

    if (breaker->state == CIRCUIT_OPEN) {
        now = time(NULL);

        if (now - breaker->opened_at >=
            (time_t)breaker->reset_timeout_seconds) {
            breaker->state = CIRCUIT_HALF_OPEN;
            return true;
        }

        return false;
    }

    return true;
}

void circuit_breaker_record_success(CircuitBreaker *breaker)
{
    if (!breaker) {
        return;
    }

    breaker->state = CIRCUIT_CLOSED;
    breaker->failure_count = 0;
    breaker->opened_at = 0;
}

void circuit_breaker_record_failure(CircuitBreaker *breaker)
{
    if (!breaker) {
        return;
    }

    if (breaker->state == CIRCUIT_HALF_OPEN) {
        breaker->state = CIRCUIT_OPEN;
        breaker->opened_at = time(NULL);
        return;
    }

    if (breaker->state != CIRCUIT_CLOSED) {
        return;
    }

    if (breaker->failure_count < breaker->failure_threshold) {
        breaker->failure_count++;
    }

    if (breaker->failure_count >= breaker->failure_threshold) {
        breaker->state = CIRCUIT_OPEN;
        breaker->opened_at = time(NULL);
    }
}

CircuitState circuit_breaker_state(const CircuitBreaker *breaker)
{
    return breaker ? breaker->state : CIRCUIT_OPEN;
}
