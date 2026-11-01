#include "ds18b20.h"
#include <math.h>
#include <OneWire.h>
#include <DallasTemperature.h>

const byte DS18B20_PIN = 5;

OneWire oneWire(DS18B20_PIN);
DallasTemperature ds18b20(&oneWire);

unsigned long lastTempRead = 0;
static float waterTemperatureC = NAN;
const unsigned long TEMP_INTERVAL = 1000;

void initDS18B20() {
  ds18b20.begin();

  Serial.println("DS18B20 initialized");
  Serial.print("Detected sensors: ");
  Serial.println(ds18b20.getDeviceCount());
}

void updateDS18B20() {
  unsigned long now = millis();

  if (now - lastTempRead < TEMP_INTERVAL) {
    return;
  }

  lastTempRead = now;

  ds18b20.requestTemperatures();
  float temperature = ds18b20.getTempCByIndex(0);

  if (temperature == DEVICE_DISCONNECTED_C) {
    waterTemperatureC = NAN;
    Serial.println("DS18B20 error: sensor disconnected!");
    return;
  }

  if (temperature < -55.0f || temperature > 125.0f || temperature == 85.0f) {
    waterTemperatureC = NAN;
    return;
  }
  waterTemperatureC = temperature;
  Serial.print("Nutrient solution temperature: ");
  Serial.print(temperature, 2);
  Serial.println(" °C");
}
float getWaterTemperatureC() { return waterTemperatureC; }
