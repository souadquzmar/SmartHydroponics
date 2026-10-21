
#include <OneWire.h>
#include <DallasTemperature.h>

// DS18B20 data pin
const byte DS18B20_PIN = 2;

OneWire oneWire(DS18B20_PIN);
DallasTemperature sensors(&oneWire);

const unsigned long SAMPLE_INTERVAL_MS = 1000UL;
unsigned long previousSampleMs = 0;

void setup() {
  Serial.begin(9600);

  sensors.begin();

  Serial.println("Smart Hydroponic Greenhouse");
  Serial.println("DS18B20 Water Temperature Monitor");

  Serial.print("Detected sensors: ");
  Serial.println(sensors.getDeviceCount());

  sensors.setResolution(12);
}

void loop() {
  unsigned long now = millis();

  if (now - previousSampleMs < SAMPLE_INTERVAL_MS) {
    return;
  }

  previousSampleMs = now;

  sensors.requestTemperatures();

  float temperatureC =
    sensors.getTempCByIndex(0);

  if (temperatureC == DEVICE_DISCONNECTED_C) {
    Serial.println("ERROR: DS18B20 not detected!");
    return;
  }

  float temperatureF =
    temperatureC * 9.0 / 5.0 + 32.0;

  Serial.print("Water Temperature: ");
  Serial.print(temperatureC, 2);
  Serial.print(" °C | ");

  Serial.print(temperatureF, 2);
  Serial.println(" °F");
}
