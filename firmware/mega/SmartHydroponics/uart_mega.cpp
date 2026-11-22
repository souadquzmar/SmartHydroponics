#include "uart_mega.h"

static const unsigned long UART_INTERVAL_MS = 2000UL;
static unsigned long lastSend = 0;
static bool initialized = false;
static bool previousFan = false;
static bool previousWaterKnown = false;
static bool previousWaterLow = false;

static const char* onOff(bool value) { return value ? "ON" : "OFF"; }

void uartSetup() {
  Serial1.begin(9600); // Mega pin 18 TX1, pin 19 RX1
  Serial.println(F("Mega UART initialized"));
}

static void sendTelemetry(const HydroTelemetry &d) {
  Serial1.print(F("DATA,ph="));         Serial1.print(d.ph, 2);
  Serial1.print(F(",tds="));          Serial1.print(d.tds, 1);
  Serial1.print(F(",waterTemp="));    Serial1.print(d.waterTemp, 2);
  Serial1.print(F(",humidity="));     Serial1.print(d.humidity, 1);
  Serial1.print(F(",airTemp="));      Serial1.print(d.airTemp, 2);
  Serial1.print(F(",heater="));       Serial1.print(onOff(d.heaterOn));
  Serial1.print(F(",light="));        Serial1.print(onOff(d.lightOn));
  Serial1.print(F(",lighting="));     Serial1.print(d.lightingLevel ? d.lightingLevel : "UNKNOWN");
  Serial1.print(F(",fan="));          Serial1.print(onOff(d.fanOn));
  Serial1.print(F(",pump="));         Serial1.print(onOff(d.pumpOn));
  Serial1.print(F(",flowLpm="));      Serial1.print(d.flowLpm, 2);
  Serial1.print(F(",ldr="));          Serial1.print(d.ldr);
  Serial1.print(F(",ledPwm="));       Serial1.print(d.ledPwm);
  Serial1.print(F(",water="));        Serial1.println(d.waterLevelAvailable ?
                                                   (d.waterLow ? "LOW" : "NORMAL") : "UNKNOWN");
}

void uartNotifyPumpOn(const char* pumpName) {
  Serial1.print(F("ALERT,pump="));
  Serial1.print(pumpName ? pumpName : "unknown");
  Serial1.println(F(",state=ON"));
  Serial.println(F("UART alert: pump ON"));
}

void uartUpdate(const HydroTelemetry &d) {
  if (!initialized) {
    previousFan = d.fanOn;
    previousWaterKnown = d.waterLevelAvailable;
    previousWaterLow = d.waterLow;
    if (d.waterLevelAvailable && d.waterLow) {
      Serial1.println(F("ALERT,water=LOW"));
    }
    initialized = true;
  } else {
    if (d.fanOn != previousFan) {
      Serial1.print(F("ALERT,fan="));
      Serial1.println(onOff(d.fanOn));
      previousFan = d.fanOn;
    }
    // Only emit water alerts when an actual sensor is configured.
    if (d.waterLevelAvailable &&
        (!previousWaterKnown || d.waterLow != previousWaterLow)) {
      Serial1.println(d.waterLow ? F("ALERT,water=LOW") : F("INFO,water=NORMAL"));
    }
    previousWaterKnown = d.waterLevelAvailable;
    previousWaterLow = d.waterLow;
  }

  unsigned long now = millis();
  if (now - lastSend >= UART_INTERVAL_MS) {
    lastSend = now;
    sendTelemetry(d);
  }
}
