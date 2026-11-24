// ===========================================================================
//  SmartHydroponics - Arduino Mega main (full system)
//  Circulation pump + flow, DS18B20, TDS, pH, automatic dosing (3 pumps),
//  LED lighting, DHT22 + fan, water heater, and UART telemetry to the ESP32.
//
//  Mega TX1 (pin 18) -> ESP32 RX2 (GPIO16)   [5 V -> 3.3 V: use a divider!]
//  Common GND between the two boards.
// ===========================================================================
#include <Arduino.h>
#include <math.h>
#include <string.h>
#include <stdio.h>
#include "flow_pump.h"
#include "ds18b20.h"
#include "tds_ec.h"
#include "ph_sensor.h"
#include "lighting.h"
#include "TempHumidity.h"
#include "water_heater.h"
#include "uart_mega.h"
#include "automatic_dosing.h"

HydroTelemetry telemetry = {};
AutomaticDoser doser;

// true  = automatic dosing runs (nutrient / pH Up / pH Down pumps can turn ON)
// false = dosing pumps always OFF; everything else still runs
constexpr bool DOSING_ENABLED = true;
constexpr unsigned long STATUS_MS = 2000UL;

static bool previousPumpRunning = false;
static bool prevFlowOK = false;
static bool flowEverConfirmed = false;

// ---------------------------------------------------------------------------
// Dosing interlock (required by automatic_dosing.h)
// ---------------------------------------------------------------------------
bool dosingSafetyOK() {
  return DOSING_ENABLED && isPumpRunning() && isFlowConfirmedForDosing();
}

// ---------------------------------------------------------------------------
// Names
// ---------------------------------------------------------------------------
const char* pumpName(DoserPump p) {
  switch (p) {
    case DP_PH_UP: return "PH_UP";
    case DP_PH_DOWN: return "PH_DOWN";
    case DP_NUTRIENT: return "NUTRIENT";
    default: return "NONE";
  }
}

const char* faultName(DoserFault f) {
  switch (f) {
    case DF_NONE: return "NONE";
    case DF_SAFETY: return "FLOW_UNSAFE";
    case DF_BAD_SENSOR: return "BAD_SENSOR";
    case DF_TDS_HIGH: return "TDS_HIGH";
    case DF_DOSE_LIMIT: return "DOSE_LIMIT";
    default: return "UNKNOWN";
  }
}

const char* stateName(DoserState s) {
  switch (s) {
    case DS_STARTUP: return "STARTUP";
    case DS_CHECK: return "CHECK";
    case DS_PULSE: return "PULSE";
    case DS_MIX: return "MIX";
    case DS_LOCKOUT: return "LOCKOUT";
    default: return "UNKNOWN";
  }
}

// ---------------------------------------------------------------------------
// ESP32 messages (format expected by smartHydroponicsESP32.ino):
//   LEVEL,DOSER,EVENT[,ARG[,EXTRA]]      LEVEL = INFO or ALERT
// ---------------------------------------------------------------------------
void uartDoser(const char* level, const char* event,
               const char* arg = nullptr, const char* extra = nullptr) {
  Serial1.print(level);
  Serial1.print(F(",DOSER,"));
  Serial1.print(event);
  if (arg)   { Serial1.print(','); Serial1.print(arg); }
  if (extra) { Serial1.print(','); Serial1.print(extra); }
  Serial1.println();
}

// Sends an event whenever the doser changes pump, state or phase.
void reportDoserChanges() {
  static DoserState prevState = DS_STARTUP;
  static DoserPhase prevPhase = DPH_TDS;
  static DoserPump prevPump = DP_NONE;
  char num[12];

  const DoserState st = doser.state();
  const DoserPhase ph = doser.phase();
  const DoserPump pu = doser.pump();

  if (pu != prevPump) {
    if (prevPump != DP_NONE) uartDoser("INFO", "PUMP_OFF", pumpName(prevPump));
    if (pu != DP_NONE) {
      snprintf(num, sizeof(num), "%lu", (unsigned long)DOSER_PULSE_MS);
      uartDoser("INFO", "PUMP_ON", pumpName(pu), num);
    }
  }
  if (st != prevState) {
    if (prevState == DS_STARTUP && st == DS_CHECK) {
      uartDoser("INFO", "MONITORING");
    } else if (st == DS_MIX) {
      snprintf(num, sizeof(num), "%lu", (unsigned long)(DOSER_MIX_MS / 1000UL));
      uartDoser("INFO", "MIXING", num);
    } else if (st == DS_LOCKOUT) {
      uartDoser("ALERT", "LOCKOUT", faultName(doser.fault()));
    }
  }
  if (ph != prevPhase && st != DS_STARTUP && st != DS_LOCKOUT) {
    uartDoser("INFO", "PHASE", ph == DPH_TDS ? "TDS" : "PH");
  }
  prevPump = pu;
  prevState = st;
  prevPhase = ph;
}

// ---------------------------------------------------------------------------
// USB serial command: RESET_DOSER (only after inspecting the cause of a lockout)
// ---------------------------------------------------------------------------
void readCommands() {
  static char buf[24];
  static uint8_t len = 0;
  while (Serial.available()) {
    char c = (char)Serial.read();
    if (c == '\n' || c == '\r') {
      buf[len] = '\0';
      if (DOSING_ENABLED && strcmp(buf, "RESET_DOSER") == 0) {
        doser.resetAfterInspection();
        flowEverConfirmed = false;   // wait for confirmed flow again
        prevFlowOK = false;
        uartDoser("INFO", "RESET");
        Serial.println(F("DOSER RESET"));
      }
      len = 0;
    } else if (len < sizeof(buf) - 1) buf[len++] = c;
    else len = 0;
  }
}

// ---------------------------------------------------------------------------
// ONE status line on USB: values + everything that is currently ON
// ---------------------------------------------------------------------------
static void addOn(bool cond, const char* name, bool &any) {
  if (!cond) return;
  if (any) Serial.print('+');
  Serial.print(name);
  any = true;
}

void printStatus() {
  static unsigned long last = 0;
  static DoserPump lastDose = DP_NONE;
  const DoserPump dose = doser.pump();
  if (dose == lastDose && millis() - last < STATUS_MS) return;
  last = millis();
  lastDose = dose;

  Serial.print(F("pH:")); Serial.print(telemetry.ph, 2);
  Serial.print(F(" TDS:")); Serial.print(telemetry.tds, 0);
  Serial.print(F("ppm WT:")); Serial.print(telemetry.waterTemp, 1);
  Serial.print(F("C Flow:")); Serial.print(telemetry.flowLpm, 2);
  Serial.print(F("L/min Air:")); Serial.print(telemetry.airTemp, 1);
  Serial.print(F("C Hum:")); Serial.print(telemetry.humidity, 0);
  Serial.print(F("% LDR:")); Serial.print(telemetry.ldr);
  Serial.print(F(" PWM:")); Serial.print(telemetry.ledPwm);
  Serial.print(F(" ON:"));
  bool any = false;
  addOn(telemetry.pumpOn, "CIRC", any);
  addOn(dose != DP_NONE, pumpName(dose), any);
  addOn(telemetry.heaterOn, "HEATER", any);
  addOn(telemetry.fanOn, "FAN", any);
  addOn(telemetry.lightOn, "LED", any);
  if (!any) Serial.print(F("NONE"));

  if (DOSING_ENABLED) {
    Serial.print(F(" Doser:")); Serial.print(stateName(doser.state()));
    Serial.print(F("/")); Serial.print(doser.phase() == DPH_TDS ? F("TDS") : F("PH"));
    if (doser.fault() != DF_NONE) {
      Serial.print('('); Serial.print(faultName(doser.fault())); Serial.print(')');
    }
  } else {
    Serial.print(F(" Doser:DISABLED"));
  }
  Serial.println();
}

// ---------------------------------------------------------------------------
void setup() {
  Serial.begin(9600);
  doser.begin();          // dosing relays OFF as early as possible
  uartSetup();
  initFlowPump();         // starts circulation pump
  initDS18B20();
  initTDSEC();
  initPHSensor();
  initLighting();
  initDHTFan();
  initWaterHeater();

  if (DOSING_ENABLED) {
    uartDoser("INFO", "WAIT_FLOW");
    Serial.println(F("HYDROPONICS AUTO MODE"));
  } else {
    doser.stopForDisabledMode();
    uartDoser("INFO", "DISABLED");
    Serial.println(F("HYDROPONICS: DOSING DISABLED"));
  }
}

void loop() {
  // --- sensors and non-dosing actuators ---
  updateFlowPump();
  updateDS18B20();
  updateWaterHeater();
  updateTDSEC();
  updatePHSensor();
  updateLighting();
  updateDHTFan();

  // --- telemetry snapshot ---
  telemetry.ph = getPH();
  telemetry.tds = getTDS();
  telemetry.waterTemp = getWaterTemperatureC();
  telemetry.humidity = getAirHumidity();
  telemetry.airTemp = getAirTemperature();
  telemetry.flowLpm = getFlowRateLPM();
  telemetry.ldr = getLDRValue();
  telemetry.ledPwm = getLEDBrightness();
  telemetry.heaterOn = isWaterHeaterOn();
  telemetry.lightOn = (telemetry.ledPwm > 0);
  telemetry.lightingLevel = getLightingCategory();
  telemetry.fanOn = getFanState();
  telemetry.pumpOn = isPumpRunning();
  telemetry.waterLevelAvailable = false;   // no level sensor fitted
  telemetry.waterLow = false;

  // --- dosing ---
  const bool flowOK = isPumpRunning() && isFlowConfirmedForDosing();
  if (DOSING_ENABLED) {
    if (flowOK && !prevFlowOK) { flowEverConfirmed = true; uartDoser("INFO", "FLOW_OK"); }
    if (!flowOK && prevFlowOK) uartDoser("ALERT", "FLOW_LOST");
    prevFlowOK = flowOK;
  }
  if (!DOSING_ENABLED) {
    doser.stopForDisabledMode();
  } else if (flowOK || flowEverConfirmed) {
    // After flow was confirmed once, losing it makes update() latch a lockout.
    doser.update(telemetry.ph, telemetry.tds);
  } else {
    doser.holdUntilFlowConfirmed();        // first startup: wait quietly
  }

  // --- ESP32 + USB ---
  if (telemetry.pumpOn && !previousPumpRunning) uartNotifyPumpOn("circulation");
  previousPumpRunning = telemetry.pumpOn;
  uartUpdate(telemetry);        // DATA line every 2 s + fan alerts
  reportDoserChanges();         // INFO/ALERT,DOSER,... events
  readCommands();
  printStatus();
}