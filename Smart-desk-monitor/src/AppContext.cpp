#include <AppContext.h>

SystemData_t g_telemetry = {0, 0, 25.0f, 50.0f, 0};

SemaphoreHandle_t xMutexData = NULL;
SemaphoreHandle_t xMutexSerial = NULL;
SemaphoreHandle_t xSemaphoreOverheat = NULL;