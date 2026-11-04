
#include <DHT.h>

#define DHTPIN 6
#define DHTTYPE DHT22
#define FAN_RELAY 7

DHT dht(DHTPIN, DHTTYPE);

// Desired greenhouse conditions
const float TEMP_ON = 26.1;
const float TEMP_OFF = 26.0;

const float HUM_ON = 65.0;
const float HUM_OFF = 62.0;

// Most relay modules are active LOW
const int RELAY_ON = LOW;
const int RELAY_OFF = HIGH;

bool fanState = false;

void setFan(bool on) {
  fanState = on;
  digitalWrite(FAN_RELAY,
               on ? RELAY_ON : RELAY_OFF);
}

void setup() {
  Serial.begin(9600);

  // Set OFF level before enabling output
  digitalWrite(FAN_RELAY, RELAY_OFF);
  pinMode(FAN_RELAY, OUTPUT);

  dht.begin();

  Serial.println("Hydroponic Greenhouse");
  Serial.println("DHT22 + Fan Control");
}

void loop() {
  float humidity = dht.readHumidity();
  float temperature = dht.readTemperature();

  if (isnan(humidity) || isnan(temperature)) {
    Serial.println("DHT22 reading failed!");
    setFan(false);
    delay(2000);
    return;
  }

  // Turn fan ON if either upper limit is exceeded
  if (temperature > TEMP_ON || humidity > HUM_ON) {
    setFan(true);
  }

  // Turn fan OFF only when both values recover
  else if (temperature <= TEMP_OFF &&
           humidity <= HUM_OFF) {
    setFan(false);
  }

  Serial.print("Temperature: ");
  Serial.print(temperature);
  Serial.print(" C | Humidity: ");
  Serial.print(humidity);
  Serial.print(" % | Fan: ");

  Serial.println(fanState ? "ON" : "OFF");

  delay(2000);
}
