#pragma once

#include <stdint.h>

// PIN CONFIGURATION
constexpr uint8_t PIN_ENCODER_CLK = 5;
constexpr uint8_t PIN_ENCODER_DT = 6;
constexpr uint8_t PIN_ENCODER_SW = 7;

constexpr uint8_t PIN_HEARTBEAT_LED = 1;
constexpr uint8_t PIN_ALARM_LED = 3;
constexpr uint8_t PIN_ALARM_BUZZER = 0;

constexpr uint8_t I2C_SDA_PIN = 8;
constexpr uint8_t I2C_SCL_PIN = 9;

constexpr uint8_t DHT_DATA_PIN = 4;

// SYSTEM CONFIGURATION
constexpr float TEMP_ALARM_THRESHOLD = 35.0f;

constexpr uint32_t SENSOR_PERIOD_MS = 2000;
constexpr uint32_t PC_PERIOD_MS = 200;
constexpr uint32_t UI_PERIOD_MS = 50;

constexpr uint8_t Tab_Number = 4;