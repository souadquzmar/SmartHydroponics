#include <math.h>
#include "flow_pump.h"
#include "ds18b20.h"
#include "tds_ec.h"
#include "ph_sensor.h"
#include "lighting.h"
#include "TempHumidity.h"

void setup() {
  Serial.begin(9600);
  initFlowPump();
  initDS18B20();
  initTDSEC();
  initPHSensor();
  initLighting();
  initDHTFan();
}

void loop() {
  updateFlowPump();
  updateDS18B20(); // Must precede TDS/EC for temperature compensation
  updateTDSEC();
  updatePHSensor();
  updateLighting();
  updateDHTFan();

  // Available for future control logic:
  // getWaterTemperatureC(), getEC(), getTDS(), getPH()
  // getAirTemperature(), getAirHumidity(), getFanState()
  // getFlowRateLPM(), isPumpRunning()
  // getLDRValue(), getLEDBrightness(), isLightingPhase()
  // getLightingPhaseRemainingSeconds()
  // Check isnan(value) before using sensor values to control dosing.
}
