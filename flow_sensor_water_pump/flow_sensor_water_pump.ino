
const byte FLOW_SENSOR_PIN = 2;
const byte PUMP_RELAY_PIN = 22;

const float CALIBRATION_FACTOR = 41.42f;
const float MINIMUM_FLOW_LPM = 1.00f;

const unsigned long SAMPLE_INTERVAL_MS = 1000UL;
const unsigned long STARTUP_GRACE_MS = 5000UL;
const unsigned long RESTART_DELAY_MS = 5000UL;

// Active-LOW relay
const byte RELAY_ON = LOW;
const byte RELAY_OFF = HIGH;

volatile unsigned long pulseCount = 0;

unsigned long previousSampleMs = 0;
unsigned long pumpStartMs = 0;
unsigned long pumpStopMs = 0;

bool pumpRunning = false;

void countFlowPulse() {
  pulseCount++;
}

void startPump() {
  noInterrupts();
  pulseCount = 0;
  interrupts();

  digitalWrite(PUMP_RELAY_PIN, RELAY_ON);

  pumpRunning = true;
  pumpStartMs = millis();
  previousSampleMs = pumpStartMs;

  Serial.println("Pump ON - startup grace period");
}

void stopPump(const char* reason) {
  digitalWrite(PUMP_RELAY_PIN, RELAY_OFF);

  pumpRunning = false;
  pumpStopMs = millis();

  Serial.print("Pump OFF: ");
  Serial.println(reason);
  Serial.println("Retrying in 5 seconds.");
}

void setup() {
  Serial.begin(9600);

  pinMode(PUMP_RELAY_PIN, OUTPUT);
  digitalWrite(PUMP_RELAY_PIN, RELAY_OFF);

  pinMode(FLOW_SENSOR_PIN, INPUT_PULLUP);

  attachInterrupt(
    digitalPinToInterrupt(FLOW_SENSOR_PIN),
    countFlowPulse,
    FALLING
  );

  Serial.println("YF-S401 + Pump 1 Monitor");
  startPump();
}

void loop() {
  unsigned long now = millis();

  // Pump is OFF: wait 5 seconds, then retry.
  if (!pumpRunning) {
    if (now - pumpStopMs >= RESTART_DELAY_MS) {
      startPump();
    }
    return;
  }

  // Measure flow every second.
  unsigned long elapsedMs = now - previousSampleMs;

  if (elapsedMs < SAMPLE_INTERVAL_MS) {
    return;
  }

  previousSampleMs = now;

  noInterrupts();
  unsigned long pulses = pulseCount;
  pulseCount = 0;
  interrupts();

  float frequencyHz =
    pulses * 1000.0f / elapsedMs;

  float flowLpm =
    frequencyHz / CALIBRATION_FACTOR;

  Serial.print("Pulses: ");
  Serial.print(pulses);
  Serial.print(" | Flow: ");
  Serial.print(flowLpm, 2);
  Serial.println(" L/min");

  // Allow the water to reach the sensor after each start.
  if (now - pumpStartMs < STARTUP_GRACE_MS) {
    Serial.println("Starting: waiting for flow.");
    return;
  }

  // Stop only when flow is below the required threshold.
  if (flowLpm < MINIMUM_FLOW_LPM) {
    stopPump(
      pulses == 0
        ? "No water flow detected"
        : "Water flow below 1.00 L/min"
    );
  } else {
    // Normal flow: keep the pump running continuously.
    Serial.println("Flow normal - pump continues running.");
  }

  Serial.println("--------------------------");
}
