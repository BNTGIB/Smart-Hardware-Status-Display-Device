#include "Sensor.h"
#include "config.h"
#include "AppContext.h"


DHT dht(DHT_DATA_PIN, DHT11);

void vTaskSensorSample(void *pvParameters){
    dht.begin();

    TickType_t xLastWakeTime = xTaskGetTickCount();
    const TickType_t xPeriod = pdMS_TO_TICKS(2000);

    for(;;){
        vTaskDelayUntil(&xLastWakeTime, xPeriod);

        float hum = dht.readHumidity(); // Do Am
        float temp = dht.readTemperature();// Nhiet Do

        if(isnan(hum) || isnan(temp)){
            if(xSemaphoreTake(xMutexSerial, pdMS_TO_TICKS(10)) == pdTRUE){
                Serial.println(F("[DHT11] Lỗi đọc dữ liệu (NaN) - Bỏ qua chu kỳ này"));
                hum = 0.99;
                temp = 0.99;
                xSemaphoreGive(xMutexSerial);
            }
            continue;
        }

        if(xSemaphoreTake(xMutexData, pdMS_TO_TICKS(50))==pdTRUE){
            g_telemetry.temperature = temp;
            g_telemetry.humidity = hum;
            xSemaphoreGive(xMutexData);
        }

    }
}