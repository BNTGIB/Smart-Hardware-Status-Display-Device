#include <AppContext.h>

SystemData_t g_telemetry = {0, 0, 0, 0, 0.0f, 0.0f, 0, false};

QueueHandle_t xQueueInputEvent    = NULL;

SemaphoreHandle_t xMutexData = NULL;
SemaphoreHandle_t xMutexSerial = NULL;
SemaphoreHandle_t xSemaphoreOverheat = NULL;