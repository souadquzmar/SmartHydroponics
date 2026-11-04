#ifndef DHT_FAN_H
#define DHT_FAN_H

#include <Arduino.h>
#include <DHT.h>

#define DHTPIN 3
#define DHTTYPE DHT22
#define FAN_RELAY 6
constexpr float TEMP_ON = 26.1;
constexpr float TEMP_OFF = 26.0;
constexpr float HUM_ON = 65.0;
constexpr float HUM_OFF = 62.0;

constexpr uint8_t RELAY_ON = LOW;
constexpr uint8_t RELAY_OFF = HIGH;

void initDHTFan();
void updateDHTFan();
float getAirTemperature();
float getAirHumidity();
bool getFanState();

#endif
