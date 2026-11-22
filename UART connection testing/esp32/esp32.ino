
#define RXD2 16
#define TXD2 17

void setup() {
  Serial.begin(115200);

  Serial2.begin(9600, SERIAL_8N1, RXD2, TXD2);

  Serial.println("ESP32 UART Test");
}

void loop() {
  if (Serial2.available()) {
    String msg = Serial2.readStringUntil('\n');
    msg.trim();

    Serial.print("ESP32 received: ");
    Serial.println(msg);

    // Send response to Arduino Mega
    if (msg == "PING") {
      Serial2.println("PONG");
      Serial.println("ESP32 sent: PONG");
    }
  }
}
