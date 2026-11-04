#ifndef LIGHTING_H
#define LIGHTING_H

#include <Arduino.h>

void initLighting();
void updateLighting();
int getLDRValue();
uint8_t getLEDBrightness();
bool isLightingPhase();
unsigned long getLightingPhaseRemainingSeconds();

#endif
