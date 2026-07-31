#include <stdbool.h>
#include <stdint.h>

typedef enum {
    TRAFFIC_LIGHT_RED,
    TRAFFIC_LIGHT_YELLOW,
    TRAFFIC_LIGHT_GREEN
} TrafficLightColor;

typedef enum {
    TRAFFIC_STATE_RED,
    TRAFFIC_STATE_GREEN,
    TRAFFIC_STATE_YELLOW
} TrafficLightState;

typedef struct {
    TrafficLightState state;
    uint32_t elapsed_ms;
    uint32_t red_duration_ms;
    uint32_t green_duration_ms;
    uint32_t yellow_duration_ms;
} TrafficLightController;

typedef struct {
    TrafficLightColor north_south;
    TrafficLightColor east_west;
} TrafficLightOutput;

void traffic_light_init(TrafficLightController *controller,
                        uint32_t red_duration_ms,
                        uint32_t green_duration_ms,
                        uint32_t yellow_duration_ms)
{
    if (controller == NULL) {
        return;
    }

    controller->state = TRAFFIC_STATE_RED;
    controller->elapsed_ms = 0;
    controller->red_duration_ms = red_duration_ms;
    controller->green_duration_ms = green_duration_ms;
    controller->yellow_duration_ms = yellow_duration_ms;
}

static void traffic_light_transition(TrafficLightController *controller)
{
    switch (controller->state) {
    case TRAFFIC_STATE_RED:
        controller->state = TRAFFIC_STATE_GREEN;
        break;

    case TRAFFIC_STATE_GREEN:
        controller->state = TRAFFIC_STATE_YELLOW;
        break;

    case TRAFFIC_STATE_YELLOW:
        controller->state = TRAFFIC_STATE_RED;
        break;

    default:
        controller->state = TRAFFIC_STATE_RED;
        break;
    }

    controller->elapsed_ms = 0;
}

void traffic_light_tick(TrafficLightController *controller, uint32_t delta_ms)
{
    uint32_t duration;

    if (controller == NULL) {
        return;
    }

    controller->elapsed_ms += delta_ms;

    switch (controller->state) {
    case TRAFFIC_STATE_RED:
        duration = controller->red_duration_ms;
        break;

    case TRAFFIC_STATE_GREEN:
        duration = controller->green_duration_ms;
        break;

    case TRAFFIC_STATE_YELLOW:
        duration = controller->yellow_duration_ms;
        break;

    default:
        controller->state = TRAFFIC_STATE_RED;
        controller->elapsed_ms = 0;
        return;
    }

    if (duration == 0 || controller->elapsed_ms >= duration) {
        traffic_light_transition(controller);
    }
}

TrafficLightState traffic_light_state(const TrafficLightController *controller)
{
    if (controller == NULL) {
        return TRAFFIC_STATE_RED;
    }

    return controller->state;
}

TrafficLightOutput traffic_light_output(const TrafficLightController *controller)
{
    TrafficLightOutput output = {
        .north_south = TRAFFIC_LIGHT_RED,
        .east_west = TRAFFIC_LIGHT_GREEN
    };

    if (controller == NULL) {
        return output;
    }

    switch (controller->state) {
    case TRAFFIC_STATE_RED:
        output.north_south = TRAFFIC_LIGHT_RED;
        output.east_west = TRAFFIC_LIGHT_GREEN;
        break;

    case TRAFFIC_STATE_GREEN:
        output.north_south = TRAFFIC_LIGHT_GREEN;
        output.east_west = TRAFFIC_LIGHT_RED;
        break;

    case TRAFFIC_STATE_YELLOW:
        output.north_south = TRAFFIC_LIGHT_YELLOW;
        output.east_west = TRAFFIC_LIGHT_RED;
        break;

    default:
        output.north_south = TRAFFIC_LIGHT_RED;
        output.east_west = TRAFFIC_LIGHT_RED;
        break;
    }

    return output;
}

bool traffic_light_is_expired(const TrafficLightController *controller)
{
    uint32_t duration;

    if (controller == NULL) {
        return false;
    }

    switch (controller->state) {
    case TRAFFIC_STATE_RED:
        duration = controller->red_duration_ms;
        break;

    case TRAFFIC_STATE_GREEN:
        duration = controller->green_duration_ms;
        break;

    case TRAFFIC_STATE_YELLOW:
        duration = controller->yellow_duration_ms;
        break;

    default:
        return true;
    }

    return duration == 0 || controller->elapsed_ms >= duration;
}
