#include "InputManager.h"
#include "config.h"

// Bảng tra chuyển đổi trạng thái hợp lệ của Gray Code (4 bước = 1 nấc xoay)
// Index = (trạng_thái_cũ << 2) | trạng_thái_mới
static const int8_t ENCODER_TABLE[16] = {
     0, -1,  1,  0,
     1,  0,  0, -1,
    -1,  0,  0,  1,
     0,  1, -1,  0
};

void IRAM_ATTR isr_encoder_rotate() {
    static uint8_t old_state = 0b11; // Trạng thái nghỉ mặc định (CLK=1, DT=1)
    static int8_t  step_acc  = 0;    // Bộ tích lũy vi bước

    // Đọc đồng thời 2 chân bằng hàm mức thấp của ESP-IDF (nhanh hơn digitalRead trong ISR)
    uint8_t clk = gpio_get_level((gpio_num_t)PIN_ENCODER_CLK);
    uint8_t dt  = gpio_get_level((gpio_num_t)PIN_ENCODER_DT);
    uint8_t new_state = (clk << 1) | dt;

    // Tra bảng hướng di chuyển (+1, -1 hoặc 0 nếu nhiễu)
    int8_t delta = ENCODER_TABLE[(old_state << 2) | new_state];
    old_state = new_state;

    if (delta != 0) {
        step_acc += delta;

        // Đủ 4 vi bước (1 nấc khấc cơ học trên KY-040) mới đẩy 1 sự kiện vào Queue
        if (step_acc >= 2 || step_acc <= -2) {
            BaseType_t xHigherPriorityTaskWoken = pdFALSE;
            InputEvent_t event = (step_acc > 0) ? EVENT_ROTATE_CW : EVENT_ROTATE_CCW;
            step_acc = 0; // Reset bộ đếm cho nấc tiếp theo

            xQueueSendFromISR(xQueueInputEvent, &event, &xHigherPriorityTaskWoken);
            if (xHigherPriorityTaskWoken) {
                portYIELD_FROM_ISR();
            }
        }
    }
}

void IRAM_ATTR isr_encoder_press() {
    static TickType_t xLastTime = 0;
    TickType_t xNow = xTaskGetTickCountFromISR();

    if (xNow - xLastTime > pdMS_TO_TICKS(200)) {
        BaseType_t xHigherPriorityTaskWoken = pdFALSE;
        InputEvent_t event = EVENT_BUTTON_PRESS;
        xQueueSendFromISR(xQueueInputEvent, &event, &xHigherPriorityTaskWoken);
        xLastTime = xNow;
        if (xHigherPriorityTaskWoken) {
            portYIELD_FROM_ISR();
        }
    }
}