
#ifndef FLOW_PUMP_H
#define FLOW_PUMP_H

void initFlowPump();
void updateFlowPump();

float getFlowRateLPM();
bool isPumpRunning();
// True only after the 60-second grace period and recent >=0.50 L/min measurement.
bool isFlowConfirmedForDosing();

#endif
