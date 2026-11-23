#pragma once

#include <DHTesp.h>


extern DHTesp dht;

void vTaskSensorSample(void *pvParameters);

void initSensor();
