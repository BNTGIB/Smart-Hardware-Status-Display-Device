#include "DisplayTask.h"

U8G2_SSD1306_128X64_NONAME_F_HW_I2C u8g2(U8G2_R0, U8X8_PIN_NONE);


unsigned char temperature_bits[] U8X8_PROGMEM = {0x70,0x00,0x00,0x8c,0x00,0x02,0xac,0x01,0x05,0xac,0x01,0x05,0xac,0x00,0x02,0xac,0x71,0x00,0xac,0x18,0x00,0xac,0x09,0x00,0xac,0x09,0x00,0xac,0x18,0x00,0xa4,0x18,0x00,0x22,0x73,0x00,0x71,0x06,0x00,0xed,0x06,0x00,0xfd,0x06,0x00,0xf9,0x06,0x00,0x71,0x02,0x00,0x02,0x01,0x00,0xfc,0x00,0x00};
// Temperature Icon
unsigned char humidity_bits[] U8X8_PROGMEM = {0x40,0x00,0x40,0x00,0x60,0x00,0x60,0x00,0xe0,0x00,0xf0,0x00,0xf0,0x01,0xfc,0x07,0xfc,0x07,0xfc,0x07,0xfe,0x0f,0xfe,0x09,0xff,0x19,0xff,0x1f,0xfe,0x0f,0xfe,0x0f,0xfe,0x0f,0xfc,0x07,0xe0,0x01};
// Humidity Icon

void vTaskUIManager(void *pvParameters) {
    u8g2.begin(); // Khởi động màn hình OLED
    
    SystemData_t local_data;

    for (;;) {
        uint8_t Tab_number = 1;

        // 2. Chụp snapshot dữ liệu từ biến chung có Mutex bảo vệ
        if (xSemaphoreTake(xMutexData, pdMS_TO_TICKS(20)) == pdTRUE) {
            local_data = g_telemetry;
            xSemaphoreGive(xMutexData);
        }
        u8g2.setBitmapMode(1);
        u8g2.setFontMode(1);
        // 3. TIẾN HÀNH "VẼ" GIAO DIỆN LÊN FRAMEBUFFER
        u8g2.clearBuffer(); // Bắt buộc xóa sạch frame cũ trước khi vẽ frame mới
        

        if(Tab_number == 0){
            u8g2.setFont(u8g2_font_helvB10_tr);

            u8g2.drawStr(8, 21, "CPU");
            u8g2.drawStr(48, 21, "RAM");
            u8g2.drawStr(90, 21, "GPU");

            u8g2.setFont(u8g2_font_helvB12_tr);

            u8g2.drawStr(10, 45, "90%");
            u8g2.drawStr(50, 45, "90%");
            u8g2.drawStr(92, 45, "90%");

        }
        else if(Tab_number == 1){
            u8g2.drawXBM(28, 10, 19, 19, temperature_bits);
            u8g2.drawXBM(89, 10, 13, 19, humidity_bits);


            u8g2.setFont(u8g2_font_helvB12_tr);
    
            char buf[20];
            snprintf(buf, sizeof(buf), "%.1f C", local_data.temperature);
            u8g2.drawStr(10, 50, buf);

            snprintf(buf, sizeof(buf), "%.1f%%", local_data.humidity);
            u8g2.drawStr(80, 50, buf);
        }
            
        
        
        // 4. BẮN TOÀN BỘ 1KB TỪ RAM XUỐNG MÀN HÌNH VẬT LÝ QUA I2C
        u8g2.sendBuffer();

        // Chu kỳ quét khung hình (~20 FPS)
        vTaskDelay(pdMS_TO_TICKS(50));
    }
}