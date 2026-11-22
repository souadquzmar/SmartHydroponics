#include <Arduino.h>
#include <math.h>
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
static bool previousPumpRunning = false;
static bool flowHasBeenConfirmed = false;
AutomaticDoser doser;

// For initial hardware testing this MUST stay false.
// Set true ONLY after confirming the relay pins, flow sensor, calibrated sensors,
// real 150 ms pump output and chemical safety procedures.
constexpr bool DOSING_COMMISSIONED = true;

bool dosingSafetyOK() {
  return DOSING_COMMISSIONED && isFlowConfirmedForDosing();
}


// Dosing events for the ESP32. UART remains 9600 baud via uartSetup().
// These small messages are sent only on state changes, not on every loop.
const char* dosingPumpName(DoserPump p) {
  switch (p) {
    case DP_PH_UP: return "PH_UP";
    case DP_PH_DOWN: return "PH_DOWN";
    case DP_NUTRIENT: return "NUTRIENT";
    default: return "NONE";
  }
}

const char* dosingFaultName(DoserFault f) {
  switch (f) {
    case DF_SAFETY: return "FLOW_UNSAFE";
    case DF_BAD_SENSOR: return "BAD_SENSOR";
    case DF_TDS_HIGH: return "TDS_HIGH";
    case DF_DOSE_LIMIT: return "DOSE_LIMIT";
    default: return "UNKNOWN";
  }
}

void reportDosingEvents(bool dosingEnabled, bool flowOK) {
  static bool initialized = false;
  static DoserState previousState = DS_LOCKOUT;
  static DoserPhase previousPhase = DPH_TDS;
  static DoserPump previousPump = DP_NONE;
  static bool previousFlowOK = false;

  const DoserState currentState = doser.state();
  const DoserPhase currentPhase = doser.phase();
  const DoserPump currentPump = doser.pump();

  if (!initialized) {
    Serial1.println(dosingEnabled ? F("INFO,DOSER,WAIT_FLOW")
                                  : F("INFO,DOSER,DISABLED"));
    initialized = true;
  }

  // Report confirmed circulation and later loss of circulation.
  if (dosingEnabled && flowOK != previousFlowOK) {
    Serial1.println(flowOK ? F("INFO,DOSER,FLOW_OK")
                           : F("ALERT,DOSER,FLOW_LOST"));
  }
  previousFlowOK = flowOK;

  if (previousPump != DP_NONE && previousPump != currentPump) {
    Serial1.print(F("INFO,DOSER,PUMP_OFF,"));
    Serial1.println(dosingPumpName(previousPump));
  }
  if (currentState == DS_LOCKOUT && previousState != DS_LOCKOUT) {
    Serial1.print(F("ALERT,DOSER,LOCKOUT,"));
    Serial1.println(dosingFaultName(doser.fault()));
  }
  if (currentPhase != previousPhase && currentState != DS_LOCKOUT) {
    Serial1.print(F("INFO,DOSER,PHASE,"));
    Serial1.println(currentPhase == DPH_TDS ? F("TDS") : F("PH"));
  }
  if (currentPump != DP_NONE && currentPump != previousPump) {
    Serial1.print(F("INFO,DOSER,PUMP_ON,"));
    Serial1.print(dosingPumpName(currentPump));
    Serial1.println(F(",150"));
  }
  if (currentState == DS_MIX && previousState != DS_MIX) {
    Serial1.println(F("INFO,DOSER,MIXING,60"));
  }
  if (currentState == DS_CHECK && previousState != DS_CHECK &&
      previousState == DS_STARTUP) {
    Serial1.println(F("INFO,DOSER,MONITORING"));
  }

  previousState = currentState;
  previousPhase = currentPhase;
  previousPump = currentPump;
}

void setup() {
  Serial.begin(9600); // Existing USB debug speed
  // Initialize Serial1 before pump starts, so startup can be reported.
  uartSetup();
  initFlowPump();
  initDS18B20();
  initTDSEC();
  initPHSensor();
  initLighting();
  initDHTFan();
  initWaterHeater();
  doser.begin();
  Serial.println(F("DOSER: TDS first (900-1000 ppm), then pH (5.8-6.0)"));
  Serial.println(F("DOSER: 150 ms pulse, 60 s circulation/mixing wait"));
  if (!DOSING_COMMISSIONED) Serial.println(F("DOSER DISABLED until commissioning"));
}

void loop() {
  updateFlowPump();
  updateDS18B20(); // Must precede TDS/EC temperature compensation
  updateWaterHeater();
  updateTDSEC();
  updatePHSensor();
  updateLighting();
  updateDHTFan();

  // Read real values from the uploaded sensor/control modules.
  telemetry.ph = getPH();
  telemetry.tds = getTDS();
  telemetry.waterTemp = getWaterTemperatureC();
  telemetry.humidity = getAirHumidity();
  telemetry.airTemp = getAirTemperature();
  telemetry.flowLpm = getFlowRateLPM();
  telemetry.ldr = getLDRValue();
  telemetry.ledPwm = getLEDBrightness();
  telemetry.heaterOn = isWaterHeaterOn();
  telemetry.lightOn = (getLEDBrightness() > 0);
  telemetry.lightingLevel = getLightingCategory();
  telemetry.fanOn = getFanState();
  telemetry.pumpOn = isPumpRunning();

  // No water level sensor is installed. A flow sensor does NOT measure water level.
  telemetry.waterLevelAvailable = false;
  telemetry.waterLow = false; // Ignored until waterLevelAvailable == true

  // During initial 60 s pump startup, flow will not yet be confirmed.
  // Keep all dosing pumps OFF without entering a permanent lockout.
  // After flow has been established ONCE, a subsequent unsafe flow condition
  // triggers a lockout and cannot automatically restart chemical dosing.
  const bool flowOK = isFlowConfirmedForDosing();
  if (DOSING_COMMISSIONED && flowOK) flowHasBeenConfirmed = true;

  if (!DOSING_COMMISSIONED || (!flowHasBeenConfirmed && !flowOK)) {
    doser.holdUntilFlowConfirmed();
  } else {
    doser.update(telemetry.ph, telemetry.tds);
  }

  reportDosingEvents(DOSING_COMMISSIONED, flowOK);

  // Send the pump ON alert only on an OFF -> ON transition.
  // Also reports the pump started inside initFlowPump() on first loop.
  if (telemetry.pumpOn && !previousPumpRunning) {
    uartNotifyPumpOn("circulation");
  }
  previousPumpRunning = telemetry.pumpOn;

  // Send event alerts on changes and DATA every 2 seconds.
  uartUpdate(telemetry);
  // Manual dosing fault reset after inspecting and correcting the fault.
  // Select Newline or Both NL & CR in USB Serial Monitor.
  static char command[20];
  static uint8_t commandLength = 0;
  while (Serial.available() > 0) {
    const char c = (char)Serial.read();
    if (c == '\n' || c == '\r') {
      command[commandLength] = '\0';
      if (strcmp(command, "RESET_DOSER") == 0 && DOSING_COMMISSIONED) {
        // A reset does not bypass a missing-flow fault.
        doser.resetAfterInspection();
        Serial1.println(F("INFO,DOSER,RESET"));
        flowHasBeenConfirmed = false;
        Serial.println(F("DOSER reset: flow must be reconfirmed"));
      }
      commandLength = 0;
    } else if (commandLength < sizeof(command) - 1) {
      command[commandLength++] = c;
    } else {
      commandLength = 0;
    }
  }
}
