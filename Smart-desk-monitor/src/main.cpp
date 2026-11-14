#include <Arduino.h>

#include "config.h"
#include "AppContext.h"
#include "Sensor.h"
#include "DisplayTask.h"

void setup() {
    Serial.begin(115200);

    Wire.begin(I2C_SDA_PIN, I2C_SCL_PIN);
    u8g2.begin();

    xMutexData         = xSemaphoreCreateMutex();
    xMutexSerial       = xSemaphoreCreateMutex();
    xSemaphoreOverheat = xSemaphoreCreateBinary();

    xTaskCreate(vTaskSensorSample, "SensorTask", 2048, NULL, 1, NULL);
    xTaskCreate(vTaskUIManager,     "UITask",     3072, NULL, 2, NULL); // Ưu tiên 2
}

void loop() {
    vTaskDelete(NULL);
}