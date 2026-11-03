#include <Arduino.h>
#include <stdint.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "freertos/semphr.h"

// --- ĐỊNH NGHĨA CHÂN NGOẠI VI (Hardware Mapping) ---
#define PIN_ENCODER_CLK     18
#define PIN_ENCODER_DT      19
#define PIN_ENCODER_SW      5
#define PIN_DHT_DATA        4
#define PIN_STATUS_LED      2

// --- CẤU TRÚC DỮ LIỆU (Data Structures) ---
typedef enum {
    EVENT_NONE = 0,
    EVENT_ROTATE_CW,
    EVENT_ROTATE_CCW,
    EVENT_BUTTON_PRESS
} InputEvent_t;

typedef struct {
    uint8_t  cpu_usage;    // 0 - 100 %
    uint8_t  ram_usage;    // 0 - 100 %
    float    temperature;  // Độ C
    float    humidity;     // %
} SystemTelemetry_t;

// --- FREE-RTOS HANDLES ---
QueueHandle_t xQueueInputEvent  = NULL;
QueueHandle_t xQueueTelemetry   = NULL;
SemaphoreHandle_t xMutexSerial  = NULL;

// --- ISR: BẮT NGẮT ROTARY ENCODER ---
void IRAM_ATTR isr_encoder_change() {
    static uint32_t last_interrupt_time = 0;
    uint32_t current_time = millis();

    // Debounce phần cứng tối giản bằng mốc thời gian (5ms)
    if (current_time - last_interrupt_time > 5) {
        BaseType_t xHigherPriorityTaskWoken = pdFALSE;
        InputEvent_t event = EVENT_NONE;

        int clk_state = digitalRead(PIN_ENCODER_CLK);
        int dt_state  = digitalRead(PIN_ENCODER_DT);

        if (clk_state != dt_state) {
            event = EVENT_ROTATE_CW;
        } else {
            event = EVENT_ROTATE_CCW;
        }

        // Đẩy sự kiện vào Queue an toàn từ trong ISR
        xQueueSendFromISR(xQueueInputEvent, &event, &xHigherPriorityTaskWoken);
        last_interrupt_time = current_time;

        if (xHigherPriorityTaskWoken) {
            portYIELD_FROM_ISR();
        }
    }
}

// --- ISR: BẮT NGẮT NÚT NHẤN (SW) ---
void IRAM_ATTR isr_button_press() {
    static uint32_t last_press_time = 0;
    uint32_t current_time = millis();

    // Debounce 200ms cho nút nhấn cơ
    if (current_time - last_press_time > 200) {
        BaseType_t xHigherPriorityTaskWoken = pdFALSE;
        InputEvent_t event = EVENT_BUTTON_PRESS;

        xQueueSendFromISR(xQueueInputEvent, &event, &xHigherPriorityTaskWoken);
        last_press_time = current_time;

        if (xHigherPriorityTaskWoken) {
            portYIELD_FROM_ISR();
        }
    }
}

// --- TASK 1: ĐỌC NGOẠI VI & THU THẬP TELEMETRY ---
void vTaskSensorSampler(void *pvParameters) {
    SystemTelemetry_t telemetry = {0, 0, 0.0f, 0.0f};
    TickType_t xLastWakeTime = xTaskGetTickCount();
    const TickType_t xFrequency = pdMS_TO_TICKS(500); // Chu kỳ lấy mẫu: 0.5 giây

    for (;;) {
        // Chờ chính xác 2 giây (Task ở trạng thái Blocked, nhường CPU)
        vTaskDelayUntil(&xLastWakeTime, xFrequency);

        // Giả lập đọc cảm biến hoặc phân tích gói UART từ PC
        telemetry.temperature = 28.5f; 
        telemetry.humidity    = 65.0f;
        telemetry.cpu_usage   = (telemetry.cpu_usage + 7) % 100;
        telemetry.ram_usage   = 62;

        // Ghi dữ liệu mới vào Queue (Ghi đè giá trị mới nhất)
        xQueueOverwrite(xQueueTelemetry, &telemetry);

        // Nhấp nháy LED trạng thái báo hiệu heartbeat
        digitalWrite(PIN_STATUS_LED, !digitalRead(PIN_STATUS_LED));
    }
}

// --- TASK 2: QUẢN LÝ GIAO DIỆN & MÁY TRẠNG THÁI (UI / FSM) ---
void vTaskUIManager(void *pvParameters) {
    InputEvent_t received_event = EVENT_NONE;
    SystemTelemetry_t current_data = {0, 0, 0.0f, 0.0f};
    uint8_t current_page = 0; // 0: PC Status, 1: Môi trường

    for (;;) {
        // 1. Kiểm tra sự kiện người dùng từ Queue (Timeout 50ms)
        if (xQueueReceive(xQueueInputEvent, &received_event, pdMS_TO_TICKS(50)) == pdPASS) {
            if (received_event == EVENT_ROTATE_CW) {
                current_page = (current_page + 1) % 2;
            } else if (received_event == EVENT_ROTATE_CCW) {
                current_page = (current_page == 0) ? 1 : 0;
            } else if (received_event == EVENT_BUTTON_PRESS) {
                // Xử lý action khi click nút
            }
        }

        // 2. Cập nhật dữ liệu đo lường mới nhất từ Queue Sensor
        xQueuePeek(xQueueTelemetry, &current_data, 0);

        // 3. Render log mô phỏng (Trên màn hình thật sẽ gọi hàm vẽ U8g2)
        if (xSemaphoreTake(xMutexSerial, pdMS_TO_TICKS(10)) == pdTRUE) {
            Serial.printf("[UI FSM] Trang: %d | CPU: %d%% | RAM: %d%% | Nhiệt độ: %.1f C\n",
                          current_page, current_data.cpu_usage, current_data.ram_usage, current_data.temperature);
            xSemaphoreGive(xMutexSerial);
        }

        // Delay chu kỳ làm mới khung hình (~20 FPS)
        vTaskDelay(pdMS_TO_TICKS(50));
    }
}

// --- SETUP & HARDWARE INIT ---
void setup() {
    Serial.begin(115200);

    pinMode(PIN_STATUS_LED, OUTPUT);
    pinMode(PIN_ENCODER_CLK, INPUT_PULLUP);
    pinMode(PIN_ENCODER_DT, INPUT_PULLUP);
    pinMode(PIN_ENCODER_SW, INPUT_PULLUP);

    // Gắn ngắt ngoài
    attachInterrupt(digitalPinToInterrupt(PIN_ENCODER_CLK), isr_encoder_change, CHANGE);
    attachInterrupt(digitalPinToInterrupt(PIN_ENCODER_SW), isr_button_press, FALLING);

    // Khởi tạo FreeRTOS Primitives
    xQueueInputEvent = xQueueCreate(10, sizeof(InputEvent_t));
    xQueueTelemetry  = xQueueCreate(1, sizeof(SystemTelemetry_t)); // Queue độ dài 1 (chứa state mới nhất)
    xMutexSerial     = xSemaphoreCreateMutex();

    // Tạo các Task chạy song song
    xTaskCreatePinnedToCore(
        vTaskSensorSampler,   // Hàm xử lý
        "SensorTask",         // Tên debug
        2048,                 // Stack depth (bytes)
        NULL,                 // Tham số truyền vào
        1,                    // Độ ưu tiên (Low)
        NULL,                 // Task handle
        0                     // Core ID (Core 0)
    );

    xTaskCreatePinnedToCore(
        vTaskUIManager,
        "UITask",
        3072,                 // Cần nhiều RAM hơn nếu render OLED
        NULL,
        2,                    // Độ ưu tiên cao hơn Sensor
        NULL,
        0
    );

    Serial.println(F("[SYSTEM] Hệ thống FreeRTOS khởi động hoàn tất."));
}

void loop() {
    // Để trống hoàn toàn - FreeRTOS Scheduler tự động điều phối các Task
    vTaskDelete(NULL);
}