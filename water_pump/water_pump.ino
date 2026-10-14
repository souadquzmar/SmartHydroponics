const int RELAY_PIN = 3;

void setup() {
  pinMode(RELAY_PIN, OUTPUT);
  digitalWrite(RELAY_PIN, HIGH);  // OFF for active-low relay
}

void loop() {
  digitalWrite(RELAY_PIN, LOW);   // Pump ON
  delay(5000);

  digitalWrite(RELAY_PIN, HIGH);  // Pump OFF
  delay(5000);
}