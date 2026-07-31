#include <stdio.h>
#include <stdlib.h>

typedef enum {
    STATE_RED,
    STATE_GREEN,
    STATE_YELLOW
} TrafficState;

typedef enum {
    EVENT_TIMER,
    EVENT_EMERGENCY,
    EVENT_RESET
} TrafficEvent;

typedef struct {
    TrafficState state;
} TrafficLight;

static const char *state_name(TrafficState state)
{
    switch (state) {
        case STATE_RED:    return "RED";
        case STATE_GREEN:  return "GREEN";
        case STATE_YELLOW: return "YELLOW";
        default:           return "UNKNOWN";
    }
}

static void transition(TrafficLight *light, TrafficEvent event)
{
    if (light == NULL) {
        return;
    }

    switch (light->state) {
        case STATE_RED:
            switch (event) {
                case EVENT_TIMER:
                    light->state = STATE_GREEN;
                    break;
                case EVENT_EMERGENCY:
                    light->state = STATE_RED;
                    break;
                case EVENT_RESET:
                    light->state = STATE_RED;
                    break;
            }
            break;

        case STATE_GREEN:
            switch (event) {
                case EVENT_TIMER:
                    light->state = STATE_YELLOW;
                    break;
                case EVENT_EMERGENCY:
                    light->state = STATE_RED;
                    break;
                case EVENT_RESET:
                    light->state = STATE_RED;
                    break;
            }
            break;

        case STATE_YELLOW:
            switch (event) {
                case EVENT_TIMER:
                    light->state = STATE_RED;
                    break;
                case EVENT_EMERGENCY:
                    light->state = STATE_RED;
                    break;
                case EVENT_RESET:
                    light->state = STATE_RED;
                    break;
            }
            break;

        default:
            light->state = STATE_RED;
            break;
    }
}

static int parse_event(int value, TrafficEvent *event)
{
    if (event == NULL) {
        return 0;
    }

    switch (value) {
        case 0:
            *event = EVENT_TIMER;
            return 1;
        case 1:
            *event = EVENT_EMERGENCY;
            return 1;
        case 2:
            *event = EVENT_RESET;
            return 1;
        default:
            return 0;
    }
}

int main(void)
{
    size_t count;
    TrafficLight light = { STATE_RED };

    if (scanf("%zu", &count) != 1) {
        return 0;
    }

    for (size_t i = 0; i < count; ++i) {
        int value;
        TrafficEvent event;

        if (scanf("%d", &value) != 1) {
            break;
        }

        if (parse_event(value, &event)) {
            transition(&light, event);
            printf("%s\n", state_name(light.state));
        }
    }

    return 0;
}
