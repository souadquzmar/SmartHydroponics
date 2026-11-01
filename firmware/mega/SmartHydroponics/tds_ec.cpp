#include <Arduino.h>
#include <math.h>
#include "tds_ec.h"
#include "ds18b20.h"

// Assumes a DFRobot-style analog TDS module, 0-2.3V output.
// Calibration is REQUIRED for accurate EC/TDS.
static const uint8_t TDS_PIN = A0;
static const float ADC_REFERENCE_V = 5.0f; // Measure actual Mega AREF/5V
static const float EC_CALIBRATION = 1.0f; // Adjust using standard solution
static const unsigned long INTERVAL_MS = 1000UL;
static unsigned long lastRead = 0;
static float ecValue = NAN, tdsValue = NAN;

void initTDSEC() { pinMode(TDS_PIN, INPUT); Serial.println("TDS/EC initialized (A1)"); }

void updateTDSEC() {
  unsigned long now = millis();
  if (now - lastRead < INTERVAL_MS) return;
  lastRead = now;
  float tempC = getWaterTemperatureC();
  if (isnan(tempC)) { ecValue = tdsValue = NAN; Serial.println("TDS/EC: waiting for valid water temperature"); return; }
  // Average samples to reduce noise.
  unsigned long sum = 0;
  for (uint8_t i = 0; i < 20; ++i) sum += analogRead(TDS_PIN);
  float voltage = (sum / 20.0f) * ADC_REFERENCE_V / 1023.0f;
  if (voltage < 0.0f || voltage > 2.3f) { ecValue = tdsValue = NAN; Serial.println("TDS/EC: voltage out of expected range"); return; }
  float coefficient = 1.0f + 0.02f * (tempC - 25.0f);
  float v25 = voltage / coefficient;
  // Common DFRobot analog TDS polynomial (ppm, 500-scale).
  float ppm = (133.42f*v25*v25*v25 - 255.86f*v25*v25 + 857.39f*v25) * 0.5f;
  tdsValue = max(0.0f, ppm * EC_CALIBRATION);
  ecValue = tdsValue / 500.0f; // approximate mS/cm; NOT independently measured EC
  Serial.print("EC (estimated): "); Serial.print(ecValue, 3); Serial.print(" mS/cm | TDS: ");
  Serial.print(tdsValue, 0); Serial.println(" ppm");
}
float getEC() { return ecValue; }
float getTDS() { return tdsValue; }
