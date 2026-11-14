#include <Arduino.h>


#include "config.h"
#include "AppContext.h"
#include "Sensor.h"
#include "DisplayTask.h"
// Định nghĩa chân I2C mặc định cho ESP32-C3 SuperMini
#define I2C_SDA_PIN 8
#define I2C_SCL_PIN 9
#define DHT_DATA_PIN 4


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