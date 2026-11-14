#include "DisplayTask.h"

U8G2_SSD1306_128X64_NONAME_F_HW_I2C u8g2(U8G2_R0, /* reset=*/ U8X8_PIN_NONE);

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