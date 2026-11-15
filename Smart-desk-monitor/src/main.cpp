#include "config.h"
#include "AppContext.h"
#include "Sensor.h"
#include "DisplayTask.h"
#include "InputManager.h"

void setup() {
    Serial.begin(115200);

    Wire.begin(I2C_SDA_PIN, I2C_SCL_PIN);
    u8g2.begin();

    pinMode(PIN_ENCODER_CLK, INPUT_PULLUP);
    pinMode(PIN_ENCODER_DT, INPUT_PULLUP);
    pinMode(PIN_ENCODER_SW, INPUT_PULLUP);

    attachInterrupt(digitalPinToInterrupt(PIN_ENCODER_CLK), isr_encoder_rotate,  CHANGE);
    attachInterrupt(digitalPinToInterrupt(PIN_ENCODER_DT),  isr_encoder_rotate, CHANGE);
    attachInterrupt(digitalPinToInterrupt(PIN_ENCODER_SW), isr_encoder_press, FALLING);

    xQueueInputEvent   = xQueueCreate(10, sizeof(InputEvent_t));
    xMutexData         = xSemaphoreCreateMutex();
    xMutexSerial       = xSemaphoreCreateMutex();
    xSemaphoreOverheat = xSemaphoreCreateBinary();

    xTaskCreate(vTaskSensorSample, "SensorTask", 2048, NULL, 1, NULL);
    xTaskCreate(vTaskUIManager,     "UITask",     3072, NULL, 2, NULL); // Ưu tiên 2
}

void loop() {
    vTaskDelete(NULL);
}