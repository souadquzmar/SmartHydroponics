
const int LDR_PIN = A0;

// More sensitive thresholds
const int BRIGHT_THRESHOLD = 100;
const int DARK_THRESHOLD = 150;

void setup() {
  Serial.begin(9600);
  pinMode(LDR_PIN, INPUT);

  Serial.println("LDR Light Sensor Test Started");
}

void loop() {
  int ldrValue = analogRead(LDR_PIN);

  Serial.print("LDR Value: ");
  Serial.print(ldrValue);

  Serial.print(" | Light Status: ");

  // LOW value = BRIGHT
  // HIGH value = DARK

  if (ldrValue >= DARK_THRESHOLD) {
    Serial.println("DARK");
  }
  else if (ldrValue >= BRIGHT_THRESHOLD) {
    Serial.println("MODERATE LIGHT");
  }
  else {
    Serial.println("BRIGHT");
  }

  delay(700);  // Faster response (0.2 seconds)
}
