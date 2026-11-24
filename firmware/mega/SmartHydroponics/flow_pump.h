#pragma once
#include <Arduino.h>

void initFlowPump();
void updateFlowPump();
float getFlowRateLPM();
bool isPumpRunning();
bool isFlowConfirmedForDosing();
