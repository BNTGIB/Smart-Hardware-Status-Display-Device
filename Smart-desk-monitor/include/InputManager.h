#pragma once

#include <Arduino.h>  

#include "AppContext.h"

void IRAM_ATTR isr_encoder_rotate();
void IRAM_ATTR isr_encoder_press();
