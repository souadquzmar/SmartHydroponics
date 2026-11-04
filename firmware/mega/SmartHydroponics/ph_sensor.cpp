
#include <Arduino.h>
#include <math.h>
#include "ph_sensor.h"

static const uint8_t PH_PIN = A0;
static const float ADC_REFERENCE_V = 5.0f;

static const unsigned long INTERVAL_MS = 1000UL;

static unsigned long lastRead = 0;
static float phValue = NAN;

void initPHSensor() {
  pinMode(PH_PIN, INPUT);
  Serial.println("pH initialized (A1)");
}

void updatePHSensor() {
  unsigned long now = millis();

  if (now - lastRead < INTERVAL_MS) return;
  lastRead = now;

  unsigned long sum = 0;

  for (uint8_t i = 0; i < 20; i++) {
    sum += analogRead(PH_PIN);
    delay(10);
  }

  float voltage =
      (sum / 20.0f) * ADC_REFERENCE_V / 1023.0f;

  // Temporary formula - calibrate before use
  phValue = 7.0f + ((2.5f - voltage) / 0.18f);

  if (!isfinite(phValue) ||
      phValue < 0.0f ||
      phValue > 14.0f) {
    phValue = NAN;
  }

  Serial.print("pH Voltage: ");
  Serial.print(voltage, 3);

  Serial.print(" | pH: ");

  if (isnan(phValue)) {
    Serial.println("invalid");
  } else {
    Serial.println(phValue, 2);
  }
}

float getPH() {
  return phValue;
}
