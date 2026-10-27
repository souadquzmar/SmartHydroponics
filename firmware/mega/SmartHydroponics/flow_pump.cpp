
#include <Arduino.h>
#include "flow_pump.h"

const byte FLOW_SENSOR_PIN = 2;
const byte PUMP_RELAY_PIN = 22;

const float CALIBRATION_FACTOR = 41.42f;
const float MINIMUM_FLOW_LPM = 1.00f;

const unsigned long SAMPLE_INTERVAL_MS = 1000UL;
const unsigned long STARTUP_GRACE_MS = 5000UL;
const unsigned long RESTART_DELAY_MS = 5000UL;

const byte RELAY_ON = LOW;
const byte RELAY_OFF = HIGH;

volatile unsigned long pulseCount = 0;

unsigned long previousSampleMs = 0;
unsigned long pumpStartMs = 0;
unsigned long pumpStopMs = 0;

float flowRateLPM = 0.0f;

bool pumpRunning = false;

void countFlowPulse() {
  pulseCount++;
}

void startPump() {
  noInterrupts();
  pulseCount = 0;
  interrupts();

  digitalWrite(PUMP_RELAY_PIN, RELAY_ON);

  pumpRunning = true;
  pumpStartMs = millis();
  previousSampleMs = pumpStartMs;

  Serial.println("Pump ON - startup grace period");
}

void stopPump(const char* reason) {
  digitalWrite(PUMP_RELAY_PIN, RELAY_OFF);

  pumpRunning = false;
  pumpStopMs = millis();
  flowRateLPM = 0.0f;

  Serial.print("Pump OFF: ");
  Serial.println(reason);
  Serial.println("Retrying in 5 seconds.");
}

void initFlowPump() {
  pinMode(PUMP_RELAY_PIN, OUTPUT);
  digitalWrite(PUMP_RELAY_PIN, RELAY_OFF);

  pinMode(FLOW_SENSOR_PIN, INPUT_PULLUP);

  attachInterrupt(
    digitalPinToInterrupt(FLOW_SENSOR_PIN),
    countFlowPulse,
    FALLING
  );

  Serial.println("YF-S401 + Pump 1 Monitor");
  startPump();
}

void updateFlowPump() {
  unsigned long now = millis();

  if (!pumpRunning) {
    if (now - pumpStopMs >= RESTART_DELAY_MS) {
      startPump();
    }
    return;
  }

  unsigned long elapsedMs = now - previousSampleMs;

  if (elapsedMs < SAMPLE_INTERVAL_MS) {
    return;
  }

  previousSampleMs = now;

  noInterrupts();
  unsigned long pulses = pulseCount;
  pulseCount = 0;
  interrupts();

  float frequencyHz =
    pulses * 1000.0f / elapsedMs;

  flowRateLPM = frequencyHz / CALIBRATION_FACTOR;

  Serial.print("Pulses: ");
  Serial.print(pulses);
  Serial.print(" | Flow: ");
  Serial.print(flowRateLPM, 2);
  Serial.println(" L/min");

  if (now - pumpStartMs < STARTUP_GRACE_MS) {
    Serial.println("Starting: waiting for flow.");
    return;
  }

  if (flowRateLPM < MINIMUM_FLOW_LPM) {
    stopPump(
      pulses == 0
        ? "No water flow detected"
        : "Water flow below 1.00 L/min"
    );
  } else {
    Serial.println("Flow normal - pump continues running.");
  }

  Serial.println("--------------------------");
}

float getFlowRateLPM() {
  return flowRateLPM;
}

bool isPumpRunning() {
  return pumpRunning;
}
