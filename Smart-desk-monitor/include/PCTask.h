#pragma once
 
#include <WiFi.h>
#include <WiFiMulti.h>
#include <WiFiUdp.h>
#include <ESPmDNS.h>

namespace NetConfig {
    constexpr uint16_t UDP_PORT    = 4210;
    constexpr char     MDNS_HOST[] = "smartmonitor";
}

extern WiFiMulti wifiMulti;
extern WiFiUDP udp;
extern bool udp_Started;

void vPCTaskReciver(void *parameter);
