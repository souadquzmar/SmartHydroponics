#ifndef UART_MEGA_H
#define UART_MEGA_H

#include <Arduino.h>

struct HydroTelemetry {
  float ph;
  float tds;
  float waterTemp;
  float humidity;
  float airTemp;
  float flowLpm;
  int ldr;
  uint8_t ledPwm;
  bool heaterOn;
  bool lightOn;
  bool fanOn;
  bool pumpOn;
  // There is no water-level sensor in the supplied firmware yet.
  // Never infer water level from flow rate.
  bool waterLevelAvailable;
  bool waterLow;
  const char* lightingLevel;
};

void uartSetup();
void uartUpdate(const HydroTelemetry &data);
void uartNotifyPumpOn(const char* pumpName);

#endif
