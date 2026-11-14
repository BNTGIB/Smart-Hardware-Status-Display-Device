#pragma once

#include <stdint.h>

typedef enum {
    EVENT_NONE = 0,
    EVENT_ROTATE_CW,
    EVENT_ROTATE_CCW,
    EVENT_BUTTON_PRESS
} InputEvent_t;

typedef struct {
    uint8_t  cpu_usage;    // %
    uint8_t  ram_usage;    // %
    float    temperature;  // C
    float    humidity;     // %
    uint32_t uptime_sec;   // s
} SystemData_t;