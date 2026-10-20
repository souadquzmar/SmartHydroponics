
/*
  Smart Hydroponic Greenhouse
  Controller: Arduino Mega 2560

  Current stage:
  Project configuration only.
*/

#include <Arduino.h>

// ========================================
// 1. SENSOR PINS
// ========================================

const byte FLOW_PIN = 2;
const byte DS18B20_PIN = 4;
const byte DHT_PIN = 5;

const byte LDR_PIN = A0;
const byte PH_PIN  = A1;
const byte TDS_PIN = A2;

// ========================================
// 2. RELAY PINS
// ========================================

const byte RELAY_PINS[] = {
  22, 23, 24, 25, 26, 27, 28, 29
};

const byte RELAY_COUNT = 8;

// Most relay modules with this interface
// use active-LOW control. Verify yours.

const byte RELAY_ON  = LOW;
const byte RELAY_OFF = HIGH;

// ========================================
// 3. TIMING
// ========================================

const unsigned long SERIAL_BAUD_RATE = 115200;

const unsigned long SENSOR_PRINT_INTERVAL_MS = 2000;
const unsigned long FLOW_SAMPLE_INTERVAL_MS = 1000;

// ========================================
// 4. FLOW SENSOR CALIBRATION
// ========================================

// Based on your experiment:
// 2485 pulses for 1 liter in 43.27 seconds.

const float FLOW_CALIBRATION_FACTOR = 41.42f;

// ========================================
// 5. INITIALIZATION
// ========================================

void setup() {
  Serial.begin(SERIAL_BAUD_RATE);

  Serial.println(
    F("Smart Hydroponic Greenhouse")
  );

  Serial.println(
    F("Configuration initialized.")
  );
}

void loop() {
  // Sensor reading and control logic
  // will be added in later steps.
}
