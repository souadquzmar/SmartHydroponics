
const int LDR_PIN = A0;
const int LED_PIN = 10;  // TIP120 through 1k resistor

// LDR thresholds
const int BRIGHT_THRESHOLD = 100;
const int DARK_THRESHOLD = 150;

// true: LDR reading increases in darkness
// false: LDR reading decreases in darkness
const bool HIGH_VALUE_IS_DARK = true;

// Light and rest durations
const unsigned long LIGHT_DURATION =
    14UL * 60UL * 60UL * 1000UL;

const unsigned long DARK_DURATION =
    10UL * 60UL * 60UL * 1000UL;

// Cycle tracking
bool lightPhase = true;
unsigned long phaseStartTime = 0;

// Serial output interval
unsigned long lastPrintTime = 0;
const unsigned long PRINT_INTERVAL = 1000UL;

void setup() {
  Serial.begin(9600);

  pinMode(LDR_PIN, INPUT);
  pinMode(LED_PIN, OUTPUT);

  analogWrite(LED_PIN, 0);

  phaseStartTime = millis();

  Serial.println("==============================");
  Serial.println("Hydroponic Lighting Controller");
  Serial.println("14h LIGHT / 10h DARK");
  Serial.println("Automatic 24h Cycle");
  Serial.println("==============================");
}

void loop() {

  unsigned long currentMillis = millis();
  unsigned long elapsed = currentMillis - phaseStartTime;

  // Automatically switch phases
  if (lightPhase && elapsed >= LIGHT_DURATION) {

    lightPhase = false;
    phaseStartTime += LIGHT_DURATION;

    analogWrite(LED_PIN, 0);

    Serial.println("==============================");
    Serial.println("14 HOURS COMPLETED");
    Serial.println("STARTING 10-HOUR DARK PHASE");
    Serial.println("==============================");
  }

  else if (!lightPhase && elapsed >= DARK_DURATION) {

    lightPhase = true;
    phaseStartTime += DARK_DURATION;

    Serial.println("==============================");
    Serial.println("10 HOURS COMPLETED");
    Serial.println("STARTING NEW 14-HOUR LIGHT PHASE");
    Serial.println("==============================");
  }

  // Average 10 LDR readings
  long total = 0;

  for (int i = 0; i < 10; i++) {
    total += analogRead(LDR_PIN);
    delay(5);
  }

  int ldrValue = total / 10;

  // Normalize LDR direction
  int lightValue = HIGH_VALUE_IS_DARK
                   ? ldrValue
                   : 1023 - ldrValue;

  int brightness = 0;
  const char* status;

  if (lightPhase) {

    // Three light intervals
    if (lightValue < BRIGHT_THRESHOLD) {

      brightness = 0;
      status = "BRIGHT - LED OFF (0%)";

    }
    else if (lightValue < DARK_THRESHOLD) {

      brightness = 64;
      status = "MODERATE - LED 25%";

    }
    else {

      brightness = 255;
      status = "DARK - LED 100%";

    }

  }
  else {

    // Mandatory 10-hour rest period
    brightness = 0;
    status = "REST PHASE - LED OFF";
  }

  // Apply PWM brightness
  analogWrite(LED_PIN, brightness);

  // Calculate remaining phase time
  unsigned long phaseDuration =
      lightPhase ? LIGHT_DURATION : DARK_DURATION;

  unsigned long phaseElapsed =
      millis() - phaseStartTime;

  unsigned long remaining =
      phaseElapsed >= phaseDuration
      ? 0
      : phaseDuration - phaseElapsed;

  // Print every second
  if (millis() - lastPrintTime >= PRINT_INTERVAL) {

    lastPrintTime = millis();

    Serial.print("Phase: ");
    Serial.print(lightPhase ? "LIGHT" : "DARK");

    Serial.print(" | LDR: ");
    Serial.print(ldrValue);

    Serial.print(" | PWM: ");
    Serial.print(brightness);

    Serial.print(" | Status: ");
    Serial.print(status);

    Serial.print(" | Remaining: ");

    unsigned long remainingSeconds = remaining / 1000UL;

    Serial.print(remainingSeconds / 3600UL);
    Serial.print("h ");

    Serial.print((remainingSeconds % 3600UL) / 60UL);
    Serial.print("m ");

    Serial.print(remainingSeconds % 60UL);
    Serial.println("s");
  }
}
