
#include <Arduino.h>

// Mega TX1 (pin 18) sends messages to ESP32 RX2 (GPIO 16).

unsigned long lastMessageTime = 0;
const unsigned long MESSAGE_INTERVAL = 2000;

int testMessage = 0;

void setup() {
  Serial.begin(115200);
  Serial1.begin(9600);

  Serial.println("Mega UART simulation started.");
  Serial.println("No sensors or actuators are being controlled.");
}

void loop() {
  if (millis() - lastMessageTime >= MESSAGE_INTERVAL) {
    lastMessageTime = millis();

    switch (testMessage) {
      case 0:
        Serial1.println(
          "DATA,ph=6.2,tds=850,waterTemp=23.5,"
          "humidity=55,airTemp=24.0,heater=OFF,"
          "light=OFF,lighting=DAY,fan=OFF,pump=OFF,"
          "flowLpm=0.00,ldr=500,ledPwm=0,water=UNKNOWN"
        );
        Serial.println("Sent simulated telemetry.");
        break;

      case 1:
        Serial1.println("INFO,DOSER,MONITORING");
        Serial.println("Sent dosing information.");
        break;

      case 2:
        Serial1.println("ALERT,water=LOW");
        Serial.println("Sent water alert.");
        break;

      case 3:
        Serial1.println("INFO,DOSER,FLOW_OK");
        Serial.println("Sent flow information.");
        break;
    }

    testMessage = (testMessage + 1) % 4;
  }
}
