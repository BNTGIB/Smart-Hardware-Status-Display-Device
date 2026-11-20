#include "config.h"
#include "AppContext.h"
#include "Sensor.h"
#include "DisplayTask.h"
#include "InputManager.h"
#include "PCTask.h"

void setup() {
    Serial.begin(115200);

    Wire.begin(I2C_SDA_PIN, I2C_SCL_PIN);
    u8g2.begin();

    pinMode(PIN_ENCODER_CLK, INPUT_PULLUP);
    pinMode(PIN_ENCODER_DT, INPUT_PULLUP);
    pinMode(PIN_ENCODER_SW, INPUT_PULLUP);

    pinMode(PIN_ALARM_BUZZER, OUTPUT);
    pinMode(PIN_HEARTBEAT_LED, OUTPUT);

    attachInterrupt(digitalPinToInterrupt(PIN_ENCODER_CLK), isr_encoder_rotate,  CHANGE);
    attachInterrupt(digitalPinToInterrupt(PIN_ENCODER_DT),  isr_encoder_rotate, CHANGE);
    attachInterrupt(digitalPinToInterrupt(PIN_ENCODER_SW), isr_encoder_press, FALLING);

    xQueueInputEvent   = xQueueCreate(10, sizeof(InputEvent_t));
    xMutexData         = xSemaphoreCreateMutex();
    xMutexSerial       = xSemaphoreCreateMutex();
    xSemaphoreOverheat = xSemaphoreCreateBinary();

    xTaskCreate(vTaskSensorSample, "SensorTask", 2048, NULL, 1, NULL);
    xTaskCreate(vTaskUIManager,     "UITask",     3072, NULL, 2, NULL);
    xTaskCreate(vPCTaskReciver, "WiFiUdpTask", 4096, NULL, 1, NULL);
}

void loop() {
    vTaskDelete(NULL);
}