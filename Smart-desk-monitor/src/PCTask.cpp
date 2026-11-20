#include "PCTask.h"
#include "AppContext.h"

WiFiMulti wifiMulti;
WiFiUDP   udp;

void vPCTaskReciver(void *parameter){
    WiFi.mode(WIFI_STA);
    WiFi.setTxPower(WIFI_POWER_8_5dBm);

    wifiMulti.addAP("Wifi Cot Nha", "uiterk20");

    bool udp_Started = false;
    char packet_buff[32];

    for(;;){
        if(wifiMulti.run() == WL_CONNECTED){
            if(udp_Started == false){
                udp.begin(NetConfig::UDP_PORT);
                MDNS.begin(NetConfig::MDNS_HOST);
                udp_Started = true;

                if (xSemaphoreTake(xMutexSerial, pdMS_TO_TICKS(20)) == pdTRUE) {
                    Serial.printf("[WIFI] Da ket noi! IP: %s | Host: %s.local:%d\n",
                                  WiFi.localIP().toString().c_str(),
                                  NetConfig::MDNS_HOST,
                                  NetConfig::UDP_PORT);
                    xSemaphoreGive(xMutexSerial);
                }
                //Thêm thông báo "Đã kết nối thành công"

            }

            int packet_size = udp.parsePacket();
            if(packet_size > 0){
                int len = udp.read(packet_buff, sizeof(packet_buff) - 1);
                if(len>0){
                    packet_buff[len] = '\0'; // Kết thúc chuỗi chuẩn C

                    int8_t rx_cpu = 0, rx_ram = 0, rx_gpu = 0;
                    float rx_gpu_temp = 0.0;

                    if(sscanf(packet_buff, "%hhu,%hhu,%hhu,%f", &rx_cpu, &rx_ram, &rx_gpu, &rx_gpu_temp) == 4){
                        if(xSemaphoreTake(xMutexData, pdMS_TO_TICKS(20)) == pdTRUE){
                            g_telemetry.cpu_usage = rx_cpu;
                            g_telemetry.ram_usage = rx_ram;
                            g_telemetry.gpu_usage = rx_gpu;
                            g_telemetry.gpu_temp = rx_gpu_temp;
                            g_telemetry.uptime_sec = millis()/1000;
                            g_telemetry.wifi_connected = true;
                            xSemaphoreGive(xMutexData);
                        }
                        if (xSemaphoreTake(xMutexSerial, pdMS_TO_TICKS(20)) == pdTRUE) {
                        Serial.printf("[WIFI] Da nhan goi tin! CPU: %hhu | RAM: %hhu| GPU:%hhu\n",
                                  g_telemetry.cpu_usage = rx_cpu,
                                    g_telemetry.ram_usage = rx_ram,
                                    g_telemetry.gpu_usage = rx_gpu);
                        xSemaphoreGive(xMutexSerial);
                        }
                        /// .... báo động CPU/GPU
                    }
                }
            }
        }
        else{
            udp_Started = false;
            if(xSemaphoreTake(xMutexData, pdMS_TO_TICKS(20))==pdTRUE){
                g_telemetry.wifi_connected = false;
                xSemaphoreGive(xMutexData);
            }
            vTaskDelay(pdMS_TO_TICKS(1000));
            continue;
        }

        vTaskDelay(pdMS_TO_TICKS(100));
    }
}