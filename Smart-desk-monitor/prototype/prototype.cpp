#include <Arduino.h>
#include <stdint.h>
#include <U8g2lib.h>
#include <Wire.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "freertos/semphr.h"

// Khởi tạo màn hình SSD1306 128x64 chế độ Full Framebuffer bằng Hardware I2C
U8G2_SSD1306_128X64_NONAME_F_HW_I2C u8g2(U8G2_R0, /* reset=*/ U8X8_PIN_NONE);

// --- PINOUT ---
#define PIN_ENCODER_CLK     18
#define PIN_ENCODER_DT      19
#define PIN_ENCODER_SW      5
#define PIN_DHT_DATA        4
#define PIN_HEARTBEAT_LED   2
#define PIN_ALARM_LED       4   // LED cảnh báo quá nhiệt / quá tải

// --- CẤU TRÚC DỮ LIỆU ---
typedef enum {
    EVENT_NONE = 0,
    EVENT_ROTATE_CW,
    EVENT_ROTATE_CCW,
    EVENT_BUTTON_PRESS
} InputEvent_t;

typedef struct {
    uint8_t  cpu_usage;    // %
    uint8_t  ram_usage;    // %
    float    temperature;  // Độ C
    float    humidity;     // %
    uint32_t uptime_sec;   // Giây
} SystemData_t;

// --- BIẾN DÙNG CHUNG (Shared Resource) ---
SystemData_t g_telemetry = {0, 0, 25.0f, 50.0f, 0};

// --- RTOS PRIMITIVES ---
QueueHandle_t     xQueueInputEvent    = NULL;
SemaphoreHandle_t xMutexData          = NULL; // Bảo vệ g_telemetry
SemaphoreHandle_t xMutexSerial        = NULL; // Bảo vệ cổng in Serial
SemaphoreHandle_t xSemaphoreOverheat  = NULL; // Binary Semaphore báo hiệu khẩn cấp

// --- ISR: ROTARY ENCODER ---
void IRAM_ATTR isr_encoder_change() {
    static uint32_t last_time = 0;
    uint32_t now = millis();

    if (now - last_time > 5) {
        BaseType_t xHigherPriorityTaskWoken = pdFALSE;
        InputEvent_t ev = (digitalRead(PIN_ENCODER_CLK) != digitalRead(PIN_ENCODER_DT)) 
                          ? EVENT_ROTATE_CW : EVENT_ROTATE_CCW;

        xQueueSendFromISR(xQueueInputEvent, &ev, &xHigherPriorityTaskWoken);
        last_time = now;
        if (xHigherPriorityTaskWoken) portYIELD_FROM_ISR();
    }
}

// --- ISR: NÚT NHẤN (SW) ---
void IRAM_ATTR isr_button_press() {
    static uint32_t last_time = 0;
    uint32_t now = millis();

    if (now - last_time > 200) {
        BaseType_t xHigherPriorityTaskWoken = pdFALSE;
        InputEvent_t ev = EVENT_BUTTON_PRESS;
        xQueueSendFromISR(xQueueInputEvent, &ev, &xHigherPriorityTaskWoken);
        last_time = now;
        if (xHigherPriorityTaskWoken) portYIELD_FROM_ISR();
    }
}

// =============================================================================
// TASK 1: ĐỌC CẢM BIẾN MÔI TRƯỜNG (Chu kỳ chậm: 2000ms)
// =============================================================================
void vTaskSensorSampler(void *pvParameters) {
    TickType_t xLastWakeTime = xTaskGetTickCount();
    const TickType_t xPeriod = pdMS_TO_TICKS(2000);

    for (;;) {
        vTaskDelayUntil(&xLastWakeTime, xPeriod);

        // Giả lập đọc cảm biến thực tế
        float temp = 26.0f + (float)(esp_random() % 150) / 10.0f; // 26.0 - 41.0 C
        float hum  = 60.0f + (float)(esp_random() % 200) / 10.0f;

        // DÙNG MUTEX ĐỂ CẬP NHẬT DỮ LIỆU CHUNG AN TOÀN
        if (xSemaphoreTake(xMutexData, pdMS_TO_TICKS(50)) == pdTRUE) {
            g_telemetry.temperature = temp;
            g_telemetry.humidity    = hum;
            xSemaphoreGive(xMutexData);
        }

        // PHÁT TÍN HIỆU KHẨN CẤP QUA SEMAPHORE NẾU VƯỢT NGƯỠNG
        if (temp > 35.0f) {
            xSemaphoreGive(xSemaphoreOverheat); // Đánh thức Task Alarm ngay lập tức
        }

        // Heartbeat LED báo hiệu Task còn sống
        digitalWrite(PIN_HEARTBEAT_LED, !digitalRead(PIN_HEARTBEAT_LED));
    }
}

// =============================================================================
// TASK 2: NHẬN DỮ LIỆU TELEMETRY TỪ MÁY TÍNH (Chu kỳ nhanh: 200ms)
// =============================================================================
void vTaskPcReceiver(void *pvParameters) {
    TickType_t xLastWakeTime = xTaskGetTickCount();
    const TickType_t xPeriod = pdMS_TO_TICKS(200);

    for (;;) {
        vTaskDelayUntil(&xLastWakeTime, xPeriod);

        // Giả lập bóc gói dữ liệu từ PC UART
        uint8_t dummy_cpu = (esp_random() % 100);
        uint8_t dummy_ram = 68;

        if (xSemaphoreTake(xMutexData, pdMS_TO_TICKS(30)) == pdTRUE) {
            g_telemetry.cpu_usage = dummy_cpu;
            g_telemetry.ram_usage = dummy_ram;
            g_telemetry.uptime_sec++;
            xSemaphoreGive(xMutexData);
        }

        // Nếu CPU quá tải > 90%, cũng phát tín hiệu cảnh báo
        if (dummy_cpu > 90) {
            xSemaphoreGive(xSemaphoreOverheat);
        }
    }
}

// =============================================================================
// TASK 3: GIÁM SÁT SỰ CỐ & BÁO ĐỘNG (Event-driven bằng Binary Semaphore)
// =============================================================================
void vTaskAlarmMonitor(void *pvParameters) {
    pinMode(PIN_ALARM_LED, OUTPUT);
    digitalWrite(PIN_ALARM_LED, LOW);

    for (;;) {
        // Task ngủ vô hạn (Block hoàn toàn, tốn 0% CPU) cho tới khi có ai Give Semaphore
        if (xSemaphoreTake(xSemaphoreOverheat, portMAX_DELAY) == pdTRUE) {
            if (xSemaphoreTake(xMutexSerial, pdMS_TO_TICKS(10)) == pdTRUE) {
                Serial.println(F(">>> [ALARM] PHÁT HIỆN QUÁ NHIỆT / QUÁ TẢI CPU! <<<"));
                xSemaphoreGive(xMutexSerial);
            }

            // Nháy LED cảnh báo 3 lần
            for (int i = 0; i < 3; i++) {
                digitalWrite(PIN_ALARM_LED, HIGH);
                vTaskDelay(pdMS_TO_TICKS(100));
                digitalWrite(PIN_ALARM_LED, LOW);
                vTaskDelay(pdMS_TO_TICKS(100));
            }
        }
    }
}

// =============================================================================
// TASK 4: QUẢN LÝ GIAO DIỆN & RENDER MÀN HÌNH (Chu kỳ 50ms ~ 20 FPS)
// =============================================================================
// Hàm phụ trợ vẽ thanh Progress Bar (Ví dụ: CPU 75%)
void drawProgressBar(int x, int y, int width, int height, uint8_t percent) {
    u8g2.drawFrame(x, y, width, height); // Vẽ viền ngoài
    int fill_width = (width - 4) * percent / 100; // Tính độ dài ruột theo %
    if (fill_width > 0) {
        u8g2.drawBox(x + 2, y + 2, fill_width, height - 4); // Tô đặc ruột
    }
}

// BÊN TRONG TASK UI:
void vTaskUIManager(void *pvParameters) {
    u8g2.begin(); // Khởi động màn hình OLED
    
    InputEvent_t ev = EVENT_NONE;
    uint8_t current_page = 0;
    SystemData_t local_data;

    for (;;) {
        // 1. Nhận sự kiện chuyển trang từ Encoder
        if (xQueueReceive(xQueueInputEvent, &ev, 0) == pdPASS) {
            if (ev == EVENT_ROTATE_CW) current_page = (current_page + 1) % 3;
            else if (ev == EVENT_ROTATE_CCW) current_page = (current_page == 0) ? 2 : current_page - 1;
        }

        // 2. Chụp snapshot dữ liệu từ biến chung có Mutex bảo vệ
        if (xSemaphoreTake(xMutexData, pdMS_TO_TICKS(20)) == pdTRUE) {
            local_data = g_telemetry;
            xSemaphoreGive(xMutexData);
        }

        // 3. TIẾN HÀNH "VẼ" GIAO DIỆN LÊN FRAMEBUFFER
        u8g2.clearBuffer(); // Bắt buộc xóa sạch frame cũ trước khi vẽ frame mới

        // Header chung cho tất cả các trang
        u8g2.setFont(u8g2_font_6x10_tr);
        u8g2.drawStr(0, 10, "SMART MONITOR");
        u8g2.drawLine(0, 12, 128, 12); // Kẻ đường kẻ ngang phân cách

        // Render nội dung theo trang hiện tại (FSM)
        if (current_page == 0) {
            // --- TRANG 0: PC TELEMETRY (CÓ THANH THANH GRAPHIC) ---
            char buf[16];
            
            // Vẽ dòng CPU
            snprintf(buf, sizeof(buf), "CPU: %d%%", local_data.cpu_usage);
            u8g2.drawStr(0, 26, buf);
            drawProgressBar(55, 18, 70, 10, local_data.cpu_usage);

            // Vẽ dòng RAM
            snprintf(buf, sizeof(buf), "RAM: %d%%", local_data.ram_usage);
            u8g2.drawStr(0, 42, buf);
            drawProgressBar(55, 34, 70, 10, local_data.ram_usage);

        } else if (current_page == 1) {
            // --- TRANG 1: MÔI TRƯỜNG PHÒNG ---
            char buf[20];
            snprintf(buf, sizeof(buf), "Temp: %.1f C", local_data.temperature);
            u8g2.drawStr(10, 30, buf);

            snprintf(buf, sizeof(buf), "Humidity: %.1f%%", local_data.humidity);
            u8g2.drawStr(10, 48, buf);

        } else if (current_page == 2) {
            // --- TRANG 2: THÔNG TIN HỆ THỐNG ---
            char buf[20];
            snprintf(buf, sizeof(buf), "Uptime: %lu s", local_data.uptime_sec);
            u8g2.drawStr(10, 30, buf);

            snprintf(buf, sizeof(buf), "Free RAM: %u B", esp_get_free_heap_size());
            u8g2.drawStr(10, 48, buf);
        }

        // Footer: Hiển thị chấm tròn chỉ thị trang hiện tại
        for (int i = 0; i < 3; i++) {
            if (i == current_page) u8g2.drawDisc(58 + i * 8, 60, 2); // Chấm đặc
            else u8g2.drawCircle(58 + i * 8, 60, 2);                // Vòng tròn rỗng
        }

        // 4. BẮN TOÀN BỘ 1KB TỪ RAM XUỐNG MÀN HÌNH VẬT LÝ QUA I2C
        u8g2.sendBuffer();

        // Chu kỳ quét khung hình (~20 FPS)
        vTaskDelay(pdMS_TO_TICKS(50));
    }
}

// --- SETUP ---
void setup() {
    Serial.begin(115200);

    pinMode(PIN_HEARTBEAT_LED, OUTPUT);
    pinMode(PIN_ENCODER_CLK, INPUT_PULLUP);
    pinMode(PIN_ENCODER_DT, INPUT_PULLUP);
    pinMode(PIN_ENCODER_SW, INPUT_PULLUP);

    attachInterrupt(digitalPinToInterrupt(PIN_ENCODER_CLK), isr_encoder_change, FALLING);
    attachInterrupt(digitalPinToInterrupt(PIN_ENCODER_SW), isr_button_press, FALLING);

    // Khởi tạo các RTOS Primitives
    xQueueInputEvent   = xQueueCreate(10, sizeof(InputEvent_t));
    xMutexData         = xSemaphoreCreateMutex();
    xMutexSerial       = xSemaphoreCreateMutex();
    xSemaphoreOverheat = xSemaphoreCreateBinary();

    // Tạo 4 Task song song
    xTaskCreate(vTaskSensorSampler, "SensorTask", 2048, NULL, 1, NULL);
    xTaskCreate(vTaskPcReceiver,    "PcTask",     2048, NULL, 1, NULL);
    xTaskCreate(vTaskAlarmMonitor,  "AlarmTask",  2048, NULL, 3, NULL); // Ưu tiên 3 (Cao nhất)
    xTaskCreate(vTaskUIManager,     "UITask",     3072, NULL, 2, NULL); // Ưu tiên 2

    Serial.println(F("[SYSTEM] Hệ thống 4 Task + Mutex/Semaphore đã khởi động."));
}

void loop() {
    vTaskDelete(NULL);
}