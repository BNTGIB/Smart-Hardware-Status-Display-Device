#pragma once

#include <stdint.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "AppType.h"

extern SystemData_t g_telemetry;

extern QueueHandle_t xQueueInputEvent;

extern SemaphoreHandle_t xMutexData;
extern SemaphoreHandle_t xMutexSerial;
extern SemaphoreHandle_t xSemaphoreOverheat;


