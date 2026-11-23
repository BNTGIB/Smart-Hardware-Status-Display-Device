#include "Sensor.h"
#include "config.h"
#include "AppContext.h"


DHTesp dht;

void initSensor() {
    // Khởi tạo chân GPIO4 cho loại cảm biến DHT11
    dht.setup(DHT_DATA_PIN, DHTesp::DHT11);
}

void vTaskSensorSample(void *pvParameters){
    TickType_t xLastWakeTime = xTaskGetTickCount();
    const TickType_t xPeriod = pdMS_TO_TICKS(2000);

    for(;;){
        vTaskDelayUntil(&xLastWakeTime, xPeriod);
        TempAndHumidity newValues = dht.getTempAndHumidity();

        if(isnan(newValues.humidity) || isnan(newValues.temperature)){
            if(xSemaphoreTake(xMutexSerial, pdMS_TO_TICKS(10)) == pdTRUE){
                Serial.println(F("[DHT11] Lỗi đọc dữ liệu (NaN) - Bỏ qua chu kỳ này"));
                newValues.humidity = 1;
                newValues.temperature = 1;
                xSemaphoreGive(xMutexSerial);
            }
            continue;
        }

        // Chỉ cập nhật vào g_telemetry khi trạng thái đọc là ERROR_NONE (không nhiễu)
        if (dht.getStatus() == DHTesp::ERROR_NONE) {
            if (xSemaphoreTake(xMutexData, pdMS_TO_TICKS(20)) == pdTRUE) {
                g_telemetry.temperature = newValues.temperature;
                g_telemetry.humidity    = newValues.humidity;
                                    
                xSemaphoreGive(xMutexData);
            }
        }
    }
}