#pragma once

#include <Wire.h>
#include <U8g2lib.h>

#include "AppContext.h"

extern U8G2_SSD1306_128X64_NONAME_F_HW_I2C u8g2;
extern unsigned char temperature_bits[]; // Temperature Icon
extern unsigned char humidity_bits[];   // Humidity Icon


void vTaskUIManager(void *pvParameters);
