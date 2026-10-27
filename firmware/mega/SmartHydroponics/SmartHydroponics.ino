#include "flow_pump.h"
#include "ds18b20.h"

void setup() {
  Serial.begin(9600);

  initFlowPump();
  initDS18B20();
}

void loop() {
  updateFlowPump();
  updateDS18B20();

  // Other sensor updates will be added here later.
}