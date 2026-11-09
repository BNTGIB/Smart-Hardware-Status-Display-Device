#include "header.h"


void taskTestBuzzer(void *pvParameters){
    for(;;){
        digitalWrite(BUZZER_PIN, HIGH);
        vTaskDelay(pdMS_TO_TICKS(200));
        digitalWrite(BUZZER_PIN, LOW);
        vTaskDelay(pdMS_TO_TICKS(200));
    }
}

void setup(){
    Serial.begin(115200);
    pinMode(BUZZER_PIN, OUTPUT);
    xTaskCreate(taskTestBuzzer, "testBuzzer", 2048, NULL, 1, NULL);
}
void loop(){

}