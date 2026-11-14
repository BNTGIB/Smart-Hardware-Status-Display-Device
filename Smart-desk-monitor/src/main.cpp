#include <Arduino.h>
#include <Wire.h>
#include <U8g2lib.h>
#include <DHT.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"

// Định nghĩa chân I2C mặc định cho ESP32-C3 SuperMini
#define I2C_SDA_PIN 8
#define I2C_SCL_PIN 9
#define DHT_DATA_PIN 4

// Khởi tạo đối tượng U8g2 cho màn hình SSD1306 128x64 (Full Framebuffer)
U8G2_SSD1306_128X64_NONAME_F_HW_I2C u8g2(U8G2_R0, /* reset=*/ U8X8_PIN_NONE);
DHT dht(DHT_DATA_PIN, DHT11);

typedef struct {
    uint8_t  cpu_usage;
    uint8_t  ram_usage;
    float    temperature;
    float    humidity;
    uint32_t uptime_sec;
} SystemData_t;

SystemData_t g_telemetry = {0, 0, 0.0f, 0.0f, 0};

SemaphoreHandle_t xMutexData = NULL;
SemaphoreHandle_t xMutexSerial = NULL;
SemaphoreHandle_t xSemaphoreOverHeat = NULL;

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

void vTaskUIManager(void *pvParameters) {
    u8g2.begin(); // Khởi động màn hình OLED
    
    SystemData_t local_data;

    for (;;) {


        // 2. Chụp snapshot dữ liệu từ biến chung có Mutex bảo vệ
        if (xSemaphoreTake(xMutexData, pdMS_TO_TICKS(20)) == pdTRUE) {
            local_data = g_telemetry;
            xSemaphoreGive(xMutexData);
        }

        // 3. TIẾN HÀNH "VẼ" GIAO DIỆN LÊN FRAMEBUFFER
        u8g2.clearBuffer(); // Bắt buộc xóa sạch frame cũ trước khi vẽ frame mới

        // Header chung cho tất cả các trang
        // u8g2.setFont(u8g2_font_9x15B_tr);
        // u8g2.drawStr(0, 20, "=SMART MONITOR=");

        // Render nội dung theo trang hiện tại (FSM)
            // --- TRANG 1: MÔI TRƯỜNG PHÒNG ---
        u8g2.setFont(u8g2_font_helvB12_tr);
    
            char buf[20];
            snprintf(buf, sizeof(buf), "Temp: %.1f C", local_data.temperature);
            u8g2.drawStr(2, 30, buf);

            snprintf(buf, sizeof(buf), "Humidity: %.1f%%", local_data.humidity);
            u8g2.drawStr(2, 50, buf);


        // 4. BẮN TOÀN BỘ 1KB TỪ RAM XUỐNG MÀN HÌNH VẬT LÝ QUA I2C
        u8g2.sendBuffer();

        // Chu kỳ quét khung hình (~20 FPS)
        vTaskDelay(pdMS_TO_TICKS(50));
    }
}


void setup() {
    Serial.begin(115200);

    // Khởi tạo bus I2C đúng chân 8 và 9 trước khi gọi u8g2.begin()
    Wire.begin(I2C_SDA_PIN, I2C_SCL_PIN);
    u8g2.begin();
;     // Giảm độ sáng (mặc định 255) để triệt tiêu vệt nhòe ngang quanh thanh Progress Bar

    Serial.println("[OLED] Khoi tao man hinh thanh cong!");

    xMutexData         = xSemaphoreCreateMutex();
    xMutexSerial       = xSemaphoreCreateMutex();

    xTaskCreate(vTaskSensorSample, "SensorTask", 2048, NULL, 1, NULL);
    xTaskCreate(vTaskUIManager,     "UITask",     3072, NULL, 2, NULL); // Ưu tiên 2
}

void loop() {
    vTaskDelete(NULL);
}