#include <Arduino.h>
#include "lighting.h"

// A0 is used by the TDS/EC module, so the LDR uses A2.
static const uint8_t LDR_PIN = A2;
static const uint8_t LED_PIN = 10; // TIP120 base through 1k resistor

static const int BRIGHT_THRESHOLD = 100;
static const int DARK_THRESHOLD = 150;
static const bool HIGH_VALUE_IS_DARK = true;

static const unsigned long LIGHT_DURATION_MS = 14UL * 60UL * 60UL * 1000UL;
static const unsigned long REST_DURATION_MS  = 10UL * 60UL * 60UL * 1000UL;
static const unsigned long READ_INTERVAL_MS = 500UL;
static const unsigned long PRINT_INTERVAL_MS = 1000UL;

static unsigned long phaseStartMs = 0;
static unsigned long lastReadMs = 0;
static unsigned long lastPrintMs = 0;
static bool lightPhase = true;
static int ldrValue = -1;
static uint8_t ledBrightness = 0;
static const char* lightStatus = "STARTING";

void initLighting() {
  pinMode(LDR_PIN, INPUT);
  pinMode(LED_PIN, OUTPUT);
  analogWrite(LED_PIN, 0);
  phaseStartMs = millis();
  lastReadMs = phaseStartMs - READ_INTERVAL_MS;
  lastPrintMs = phaseStartMs;
  lightPhase = true;
  Serial.println("Lighting initialized (LDR A2, LED PWM 10)");
  Serial.println("Automatic 14h light / 10h rest cycle");
}

void updateLighting() {
  unsigned long now = millis();

  // Advance through phase boundaries without losing elapsed time.
  // Subtraction is rollover-safe for these durations.
  while (true) {
    unsigned long duration = lightPhase ? LIGHT_DURATION_MS : REST_DURATION_MS;
    if (now - phaseStartMs < duration) break;
    phaseStartMs += duration;
    lightPhase = !lightPhase;
    if (!lightPhase) {
      ledBrightness = 0;
      analogWrite(LED_PIN, 0);
      Serial.println("Lighting: 14h complete; 10h rest started");
    } else {
      Serial.println("Lighting: 10h rest complete; 14h light started");
    }
  }

  if (now - lastReadMs >= READ_INTERVAL_MS) {
    lastReadMs = now;
    unsigned long sum = 0;
    for (uint8_t i = 0; i < 10; ++i) {
      sum += analogRead(LDR_PIN);
      delay(5);
    }
    ldrValue = sum / 10;
    int normalized = HIGH_VALUE_IS_DARK ? ldrValue : 1023 - ldrValue;

    if (!lightPhase) {
      ledBrightness = 0;
      lightStatus = "REST - LED OFF";
    } else if (normalized < BRIGHT_THRESHOLD) {
      ledBrightness = 0;
      lightStatus = "BRIGHT - LED OFF";
    } else if (normalized < DARK_THRESHOLD) {
      ledBrightness = 64;
      lightStatus = "MODERATE - LED 25%";
    } else {
      ledBrightness = 255;
      lightStatus = "DARK - LED 100%";
    }
    analogWrite(LED_PIN, ledBrightness);
  }

  if (now - lastPrintMs >= PRINT_INTERVAL_MS) {
    lastPrintMs = now;
    Serial.print("Lighting | Phase: ");
    Serial.print(lightPhase ? "LIGHT" : "REST");
    Serial.print(" | LDR: ");
    Serial.print(ldrValue);
    Serial.print(" | PWM: ");
    Serial.print(ledBrightness);
    Serial.print(" | Status: ");
    Serial.print(lightStatus);
    Serial.print(" | Remaining: ");
    Serial.print(getLightingPhaseRemainingSeconds());
    Serial.println(" s");
  }
}

int getLDRValue() { return ldrValue; }
uint8_t getLEDBrightness() { return ledBrightness; }
bool isLightingPhase() { return lightPhase; }
unsigned long getLightingPhaseRemainingSeconds() {
  unsigned long duration = lightPhase ? LIGHT_DURATION_MS : REST_DURATION_MS;
  unsigned long elapsed = millis() - phaseStartMs;
  return (elapsed >= duration) ? 0 : (duration - elapsed) / 1000UL;
}
