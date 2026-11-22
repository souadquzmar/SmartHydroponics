
void setup() {
  Serial.begin(115200);
  Serial1.begin(9600);

  Serial.println("Arduino Mega UART Test");
}

void loop() {
  // Send message to ESP32
  Serial1.println("PING");
  Serial.println("Mega sent: PING");

  // Receive response from ESP32
  unsigned long start = millis();

  while (millis() - start < 1000) {
    if (Serial1.available()) {
      String msg = Serial1.readStringUntil('\n');
      msg.trim();

      Serial.print("Mega received: ");
      Serial.println(msg);
      break;
    }
  }

  delay(1000);
}
