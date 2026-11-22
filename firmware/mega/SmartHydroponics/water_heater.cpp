
#include "water_heater.h"
#include "ds18b20.h"
#include <Arduino.h>
#include <math.h>

const uint8_t HEATER_RELAY_PIN = 7;
const bool HEATER_RELAY_ACTIVE_LOW = true;
const float HEATER_TARGET_C = 21.0f;

const unsigned long HEATER_ON_MS = 120000UL;
const unsigned long HEATER_OFF_MS = 30000UL;

static bool heaterOn = false;
static bool cycling = false;
static unsigned long phaseStartedAt = 0;

static void setHeater(bool on) {

  // Print only when heater state changes
  if (heaterOn != on) {
    Serial.print("Heater is ");
    Serial.println(on ? "ON" : "OFF");
  }

  heaterOn = on;

  digitalWrite(HEATER_RELAY_PIN,
               (on == HEATER_RELAY_ACTIVE_LOW) ? LOW : HIGH);
}

void initWaterHeater() {
  // Set the OFF level before enabling the output
  digitalWrite(HEATER_RELAY_PIN, HEATER_RELAY_ACTIVE_LOW ? HIGH : LOW);
  pinMode(HEATER_RELAY_PIN, OUTPUT);

  setHeater(false);
  cycling = false;

  Serial.println("[WATER HEATER] Initialized - Heater OFF");
}

void updateWaterHeater() {
  const float temperatureC = getWaterTemperatureC();
  const unsigned long now = millis();

  // Invalid temperature or target reached
  if (isnan(temperatureC) || temperatureC >= HEATER_TARGET_C) {
    setHeater(false);
    cycling = false;
    return;
  }

  // Start heating cycle
  if (!cycling) {
    cycling = true;
    phaseStartedAt = now;
    setHeater(true);
    return;
  }

  // Switch between ON and OFF periods
  if (now - phaseStartedAt >=
      (heaterOn ? HEATER_ON_MS : HEATER_OFF_MS)) {

    phaseStartedAt = now;
    setHeater(!heaterOn);
  }
}

bool isWaterHeaterOn() {
  return heaterOn;
}
