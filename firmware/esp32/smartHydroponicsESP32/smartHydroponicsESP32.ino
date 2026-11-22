#include <Arduino.h>
#include <string.h>

#define RXD2 16
#define TXD2 17

// Reads existing Mega DATA/ALERT/INFO messages, plus new dosing events.
constexpr size_t BUFFER_SIZE = 200;
char rxBuffer[BUFFER_SIZE];
size_t rxIndex = 0;
bool discardingLine = false;

// Messages produced by the companion Mega sketch:
// INFO,DOSER,DISABLED
// INFO,DOSER,WAIT_FLOW
// INFO,DOSER,FLOW_OK
// ALERT,DOSER,FLOW_LOST
// INFO,DOSER,PHASE,TDS|PH
// INFO,DOSER,PUMP_ON,NUTRIENT|PH_UP|PH_DOWN,150
// INFO,DOSER,PUMP_OFF,NUTRIENT|PH_UP|PH_DOWN
// INFO,DOSER,MIXING,60
// INFO,DOSER,MONITORING
// ALERT,DOSER,LOCKOUT,FLOW_UNSAFE|BAD_SENSOR|TDS_HIGH|DOSE_LIMIT
// INFO,DOSER,RESET

const char* humanPump(const char* code) {
  if (strcmp(code, "NUTRIENT") == 0) return "Nutrient dosing";
  if (strcmp(code, "PH_UP") == 0) return "pH UP dosing";
  if (strcmp(code, "PH_DOWN") == 0) return "pH DOWN dosing";
  return "Unknown pump";
}

void displayDosingMessage(const char* message) {
  // Parse a copy: the incoming line is preserved for log/debug printing.
  char temp[BUFFER_SIZE];
  strncpy(temp, message, sizeof(temp) - 1);
  temp[sizeof(temp) - 1] = '\0';

  char* fields[6] = {nullptr};
  size_t count = 0;
  char* p = temp;
  while (count < 6) {
    fields[count++] = p;
    char* comma = strchr(p, ',');
    if (!comma) break;
    *comma = '\0';
    p = comma + 1;
  }

  if (count < 3) { Serial.println(message); return; }
  const char* event = fields[2];
  const char* arg = count > 3 ? fields[3] : "";
  const char* extra = count > 4 ? fields[4] : "";

  Serial.println();
  Serial.println(F("===== DOSING STATUS ====="));

  if (strcmp(event, "DISABLED") == 0) {
    Serial.println(F("Automatic dosing DISABLED: commissioning required."));
  } else if (strcmp(event, "WAIT_FLOW") == 0) {
    Serial.println(F("Waiting for confirmed circulation flow; dosing OFF."));
  } else if (strcmp(event, "FLOW_OK") == 0) {
    Serial.println(F("Flow confirmed (Mega threshold >= 0.50 L/min)."));
  } else if (strcmp(event, "FLOW_LOST") == 0) {
    Serial.println(F("WARNING: circulation flow no longer confirmed."));
  } else if (strcmp(event, "PHASE") == 0) {
    if (strcmp(arg, "TDS") == 0)
      Serial.println(F("Correction priority: TDS (target 900-1000 ppm)."));
    else if (strcmp(arg, "PH") == 0)
      Serial.println(F("TDS accepted; now checking pH (target 5.8-6.0)."));
    else Serial.println(message);
  } else if (strcmp(event, "PUMP_ON") == 0) {
    Serial.print(humanPump(arg));
    Serial.print(F(" pump ON, requested pulse "));
    Serial.print(*extra ? extra : "?");
    Serial.println(F(" ms."));
  } else if (strcmp(event, "PUMP_OFF") == 0) {
    Serial.print(humanPump(arg));
    Serial.println(F(" pump OFF."));
  } else if (strcmp(event, "MIXING") == 0) {
    Serial.print(F("Mixing interval started: "));
    Serial.print(*arg ? arg : "?");
    Serial.println(F(" seconds; dosing pumps OFF."));
  } else if (strcmp(event, "MONITORING") == 0) {
    Serial.println(F("Initial mixing complete: evaluating sensor readings."));
  } else if (strcmp(event, "RESET") == 0) {
    Serial.println(F("Mega received manual doser reset command."));
  } else if (strcmp(event, "LOCKOUT") == 0) {
    Serial.print(F("DOSING LOCKED OUT: "));
    if (strcmp(arg, "FLOW_UNSAFE") == 0)
      Serial.println(F("insufficient or unconfirmed circulation flow."));
    else if (strcmp(arg, "BAD_SENSOR") == 0)
      Serial.println(F("invalid pH or TDS measurement."));
    else if (strcmp(arg, "TDS_HIGH") == 0)
      Serial.println(F("TDS above 1000 ppm: stop adding nutrients."));
    else if (strcmp(arg, "DOSE_LIMIT") == 0)
      Serial.println(F("maximum correction pulses reached."));
    else Serial.println(arg);
    Serial.println(F("All dosing OFF. Inspect the system before manual reset."));
  } else {
    Serial.println(message);
  }
  Serial.println(F("========================="));
}

void processMessage(const char* message) {
  // Dosing messages are a subset of the existing INFO/ALERT protocol.
  if (strncmp(message, "INFO,DOSER,", 11) == 0 ||
      strncmp(message, "ALERT,DOSER,", 12) == 0) {
    displayDosingMessage(message);
  } else if (strncmp(message, "DATA,", 5) == 0) {
    Serial.println(F("----- SENSOR DATA -----"));
    Serial.println(message);
  } else if (strncmp(message, "ALERT,", 6) == 0) {
    Serial.println(F("!!!!! ALERT !!!!!"));
    Serial.println(message);
  } else if (strncmp(message, "INFO,", 5) == 0) {
    Serial.println(F("----- INFO -----"));
    Serial.println(message);
  } else {
    Serial.print(F("Unknown message: "));
    Serial.println(message);
  }
}

void setup() {
  Serial.begin(115200);
  Serial2.begin(9600, SERIAL_8N1, RXD2, TXD2);
  Serial.println(F("ESP32 Hydroponics Ready"));
}

void loop() {
  while (Serial2.available() > 0) {
    const char c = (char)Serial2.read();
    if (c == '\n') {
      if (!discardingLine) {
        rxBuffer[rxIndex] = '\0';
        if (rxIndex > 0 && rxBuffer[rxIndex - 1] == '\r')
          rxBuffer[rxIndex - 1] = '\0';
        if (rxBuffer[0] != '\0') processMessage(rxBuffer);
      }
      rxIndex = 0;
      discardingLine = false;
    } else if (!discardingLine) {
      if (rxIndex < BUFFER_SIZE - 1) rxBuffer[rxIndex++] = c;
      else {
        Serial.println(F("UART message too long; discarded"));
        rxIndex = 0;
        discardingLine = true;
      }
    }
  }
}
