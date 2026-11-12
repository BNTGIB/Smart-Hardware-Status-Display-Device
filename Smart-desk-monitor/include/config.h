#pragma once

#include <stdint.h>

// PIN CONFIGURATION
constexpr uint8_t PIN_ENCODER_CLK = 18;
constexpr uint8_t PIN_ENCODER_DT = 19;
constexpr uint8_t PIN_ENCODER_SW = 5;

constexpr uint8_t PIN_DHT_DATA = 4;
constexpr uint8_t PIN_HEARTBEAT_LED = 2;
constexpr uint8_t PIN_ALARM_LED = 16;

// SYSTEM CONFIGURATION
constexpr float TEMP_ALARM_THRESHOLD = 35.0f;

constexpr uint32_t SENSOR_PERIOD_MS = 2000;
constexpr uint32_t PC_PERIOD_MS = 200;
constexpr uint32_t UI_PERIOD_MS = 50;