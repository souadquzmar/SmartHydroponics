#include "flow_pump.h"

namespace {
constexpr byte FLOW_SENSOR_PIN = 2;
constexpr byte PUMP_RELAY_PIN = 5;
constexpr float CALIBRATION_FACTOR = 41.42f; // pulses per liter: calibrate for your sensor
constexpr float MINIMUM_FLOW_LPM = 0.50f;
constexpr unsigned long SAMPLE_INTERVAL_MS = 1000UL;
constexpr unsigned long STARTUP_GRACE_MS = 30000UL;
constexpr unsigned long RESTART_DELAY_MS = 60000UL;
constexpr byte RELAY_ON = LOW;
constexpr byte RELAY_OFF = HIGH;

volatile unsigned long pulseCount = 0;
bool pumpRunning = false;
bool flowConfirmed = false;
float flowRateLPM = 0.0f;
unsigned long pumpStartTime = 0;
unsigned long pumpStopTime = 0;
unsigned long lastSampleTime = 0;

void countPulse() { ++pulseCount; }

void startPump() {
  // Reset state *before* energizing the circulation pump
  noInterrupts();
  pulseCount = 0;
  interrupts();
  flowRateLPM = 0.0f;
  flowConfirmed = false;
  pumpStartTime = millis();
  lastSampleTime = pumpStartTime;
  pumpRunning = true;
  digitalWrite(PUMP_RELAY_PIN, RELAY_ON);
  Serial.println(F("Pump ON"));
}

void stopPump(const __FlashStringHelper *reason) {
  digitalWrite(PUMP_RELAY_PIN, RELAY_OFF);
  pumpRunning = false;
  flowConfirmed = false;
  flowRateLPM = 0.0f;
  pumpStopTime = millis();
  Serial.print(F("Pump OFF: "));
  Serial.println(reason);
  Serial.println(F("Retrying in 60 seconds."));
}
} // namespace

void initFlowPump() {
  pinMode(FLOW_SENSOR_PIN, INPUT_PULLUP);
  digitalWrite(PUMP_RELAY_PIN, RELAY_OFF);
  pinMode(PUMP_RELAY_PIN, OUTPUT);
  attachInterrupt(digitalPinToInterrupt(FLOW_SENSOR_PIN), countPulse, FALLING);
  startPump();
}

void updateFlowPump() {
  unsigned long now = millis();
  if (!pumpRunning) {
    if (now - pumpStopTime >= RESTART_DELAY_MS) startPump();
    return;
  }
  if (now - lastSampleTime < SAMPLE_INTERVAL_MS) return;
  const unsigned long elapsed = now - lastSampleTime;
  lastSampleTime = now;
  noInterrupts();
  const unsigned long pulses = pulseCount;
  pulseCount = 0;
  interrupts();
  flowRateLPM = (pulses * 60000.0f) / (CALIBRATION_FACTOR * elapsed);
  Serial.print(F("Flow: "));
  Serial.print(flowRateLPM, 2);
  Serial.println(F(" L/min"));

  // A reading becomes dosing-safe only after startup grace expires.
  flowConfirmed = (now - pumpStartTime >= STARTUP_GRACE_MS) &&
                  (flowRateLPM >= MINIMUM_FLOW_LPM);
  if (now - pumpStartTime < STARTUP_GRACE_MS) {
    Serial.println(F("Startup grace period..."));
    return;
  }
  if (!flowConfirmed) {
    stopPump(pulses == 0 ? F("No water flow detected")
                         : F("Water flow below 0.5 L/min"));
  } else {
    Serial.println(F("Flow normal - pump continues running."));
  }
}

float getFlowRateLPM() { return flowRateLPM; }
bool isPumpRunning() { return pumpRunning; }
bool isFlowConfirmedForDosing() { return pumpRunning && flowConfirmed; }
