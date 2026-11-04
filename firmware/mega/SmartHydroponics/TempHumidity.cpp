#include "TempHumidity.h"
#include <math.h>

static DHT dht(DHTPIN, DHTTYPE);
static bool fanState = false;
static float airTemperature = NAN;
static float airHumidity = NAN;
static unsigned long lastReadTime = 0;
static bool firstRead = true;
static const unsigned long READ_INTERVAL = 2000;

static void setFan(bool on) {
  fanState = on;
  digitalWrite(FAN_RELAY, on ? RELAY_ON : RELAY_OFF);
}

void initDHTFan() {
  digitalWrite(FAN_RELAY, RELAY_OFF);
  pinMode(FAN_RELAY, OUTPUT);
  fanState = false;
  dht.begin();
  Serial.println("DHT22 + Fan initialized");
}

void updateDHTFan() {
  unsigned long currentTime = millis();
  if (!firstRead && currentTime - lastReadTime < READ_INTERVAL) return;
  firstRead = false;
  lastReadTime = currentTime;

  airHumidity = dht.readHumidity();
  airTemperature = dht.readTemperature();

  if (isnan(airHumidity) || isnan(airTemperature)) {
    Serial.println("DHT22 reading failed!");
    setFan(false);
    return;
  }

  if (airTemperature > TEMP_ON || airHumidity > HUM_ON) {
    setFan(true);
  } else if (airTemperature <= TEMP_OFF && airHumidity <= HUM_OFF) {
    setFan(false);
  }

  Serial.print("Temperature: ");
  Serial.print(airTemperature);
  Serial.print(" C | Humidity: ");
  Serial.print(airHumidity);
  Serial.print(" % | Fan: ");
  Serial.println(fanState ? "ON" : "OFF");
}

float getAirTemperature() { return airTemperature; }
float getAirHumidity() { return airHumidity; }
bool getFanState() { return fanState; }
