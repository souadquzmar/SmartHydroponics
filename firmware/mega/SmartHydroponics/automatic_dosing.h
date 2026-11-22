#ifndef AUTOMATIC_DOSING_H
#define AUTOMATIC_DOSING_H
#include <Arduino.h>
#include <math.h>

// Check against the rest of your project's pins before wiring.
constexpr uint8_t PH_UP_RELAY_PIN = 30;
constexpr uint8_t PH_DOWN_RELAY_PIN = 31;
constexpr uint8_t NUTRIENT_RELAY_PIN = 32;

// Change these ONLY if your relay board is active-HIGH.
constexpr uint8_t DOSER_RELAY_ON = LOW;
constexpr uint8_t DOSER_RELAY_OFF = HIGH;

constexpr float DOSER_PH_LOW = 5.8f;
constexpr float DOSER_PH_HIGH = 6.0f;
constexpr float DOSER_TDS_LOW_PPM = 900.0f;
constexpr float DOSER_TDS_HIGH_PPM = 1000.0f;

// At a measured 21 mL/s, 150 ms dispenses ~3.15 mL.
// Verify that a short pulse actually makes this pump/relay dispense consistently.
constexpr unsigned long DOSER_PULSE_MS = 150UL;
constexpr unsigned long DOSER_MIX_MS = 60000UL;
constexpr unsigned long DOSER_STARTUP_MIX_MS = 60000UL;
constexpr unsigned long DOSER_STABLE_MS = 5000UL;
constexpr uint8_t DOSER_MAX_TDS_PULSES = 5;
constexpr uint8_t DOSER_MAX_PH_PULSES = 5;

constexpr float DOSER_PH_SANITY_LOW = 3.0f;
constexpr float DOSER_PH_SANITY_HIGH = 10.0f;
constexpr float DOSER_TDS_SANITY_HIGH = 3000.0f;

// Defined in the main sketch; must confirm circulation and measured flow.
bool dosingSafetyOK();

enum DoserPump : uint8_t { DP_NONE, DP_PH_UP, DP_PH_DOWN, DP_NUTRIENT };
enum DoserState : uint8_t { DS_STARTUP, DS_CHECK, DS_PULSE, DS_MIX, DS_LOCKOUT };
enum DoserFault : uint8_t { DF_NONE, DF_SAFETY, DF_BAD_SENSOR, DF_TDS_HIGH, DF_DOSE_LIMIT };
enum DoserPhase : uint8_t { DPH_TDS, DPH_PH };

class AutomaticDoser {
public:
  void begin() {
    // Preload OFF before configuring OUTPUT to avoid active-LOW relay glitches.
    digitalWrite(PH_UP_RELAY_PIN, DOSER_RELAY_OFF);
    digitalWrite(PH_DOWN_RELAY_PIN, DOSER_RELAY_OFF);
    digitalWrite(NUTRIENT_RELAY_PIN, DOSER_RELAY_OFF);
    pinMode(PH_UP_RELAY_PIN, OUTPUT);
    pinMode(PH_DOWN_RELAY_PIN, OUTPUT);
    pinMode(NUTRIENT_RELAY_PIN, OUTPUT);
    allOff();
    state_ = DS_STARTUP;
    phase_ = DPH_TDS;
    fault_ = DF_NONE;
    stateSince_ = millis();
    validSince_ = millis();
    tdsPulses_ = 0;
    phPulses_ = 0;
    candidate_ = DP_NONE;
  }

  // During first circulation startup, hold every dosing output OFF without
  // treating the normal zero-flow startup period as a permanent fault.
  // Once flow has been established, a later flow loss causes a lockout instead.
  void holdUntilFlowConfirmed() {
    if (state_ == DS_LOCKOUT) { allOff(); return; }
    allOff();
    state_ = DS_STARTUP;
    stateSince_ = millis();
    candidate_ = DP_NONE;
    phase_ = DPH_TDS;
  }

  // Call frequently in loop() with the current sensor values.
  void update(float ph, float tdsPpm) {
    const unsigned long now = millis();
    if (state_ == DS_LOCKOUT) { allOff(); return; }
    if (!dosingSafetyOK()) { lock(DF_SAFETY); return; }
    if (!isfinite(ph) || !isfinite(tdsPpm) ||
        ph < DOSER_PH_SANITY_LOW || ph > DOSER_PH_SANITY_HIGH ||
        tdsPpm <= 0.0f || tdsPpm > DOSER_TDS_SANITY_HIGH) {
      lock(DF_BAD_SENSOR); return;
    }
    if (tdsPpm > DOSER_TDS_HIGH_PPM) { lock(DF_TDS_HIGH); return; }

    if (state_ == DS_PULSE) {
      if (now - stateSince_ >= DOSER_PULSE_MS) {
        allOff();
        state_ = DS_MIX;
        stateSince_ = now;
        Serial.println(F("DOSER: OFF; wait 60 seconds for mixing"));
      }
      return;
    }
    if (state_ == DS_STARTUP || state_ == DS_MIX) {
      const unsigned long waitMs = (state_ == DS_STARTUP) ? DOSER_STARTUP_MIX_MS : DOSER_MIX_MS;
      if (now - stateSince_ < waitMs) return;
      state_ = DS_CHECK;
      candidate_ = DP_NONE;
      validSince_ = now;
    }

    // TDS is ALWAYS assessed before pH. Do not dose pH before TDS is in range.
    // If TDS later falls below 900 (e.g. during pH correction), return to TDS phase.
    if (tdsPpm < DOSER_TDS_LOW_PPM && phase_ != DPH_TDS) {
      phase_ = DPH_TDS;
      candidate_ = DP_NONE;
      validSince_ = now;
      Serial.println(F("DOSER: TDS dropped; return to TDS correction"));
    }
    if (phase_ == DPH_TDS && tdsPpm >= DOSER_TDS_LOW_PPM) {
      phase_ = DPH_PH;
      candidate_ = DP_NONE;
      validSince_ = now;
      tdsPulses_ = 0;
      Serial.println(F("DOSER: TDS acceptable; now correct pH"));
    }

    DoserPump requested = DP_NONE;
    if (phase_ == DPH_TDS) {
      if (tdsPpm < DOSER_TDS_LOW_PPM) requested = DP_NUTRIENT;
    } else {
      if (ph < DOSER_PH_LOW) requested = DP_PH_UP;
      else if (ph > DOSER_PH_HIGH) requested = DP_PH_DOWN;
    }

    // Require stable request for 5 s to prevent sensor spikes causing doses.
    if (requested != candidate_) {
      candidate_ = requested;
      validSince_ = now;
      return;
    }
    if (now - validSince_ < DOSER_STABLE_MS) return;
    if (requested == DP_NONE) {
      allOff();
      if (phase_ == DPH_PH) phPulses_ = 0;
      return;
    }

    if (requested == DP_NUTRIENT) {
      if (tdsPulses_ >= DOSER_MAX_TDS_PULSES) { lock(DF_DOSE_LIMIT); return; }
      ++tdsPulses_;
    } else {
      if (phPulses_ >= DOSER_MAX_PH_PULSES) { lock(DF_DOSE_LIMIT); return; }
      ++phPulses_;
    }

    allOff();
    if (requested == DP_NUTRIENT) digitalWrite(NUTRIENT_RELAY_PIN, DOSER_RELAY_ON);
    if (requested == DP_PH_UP) digitalWrite(PH_UP_RELAY_PIN, DOSER_RELAY_ON);
    if (requested == DP_PH_DOWN) digitalWrite(PH_DOWN_RELAY_PIN, DOSER_RELAY_ON);
    active_ = requested;
    state_ = DS_PULSE;
    stateSince_ = now;
    Serial.print(F("DOSER: ON for 150 ms; pump="));
    Serial.println(requested == DP_NUTRIENT ? F("NUTRIENT") :
                   requested == DP_PH_UP ? F("PH UP") : F("PH DOWN"));
  }

  // Only after inspecting and correcting the fault, by explicit serial command.
  void resetAfterInspection() {
    allOff();
    tdsPulses_ = 0;
    phPulses_ = 0;
    fault_ = DF_NONE;
    state_ = DS_STARTUP;
    phase_ = DPH_TDS;
    stateSince_ = millis();
    candidate_ = DP_NONE;
  }
  void stopForDisabledMode() { allOff(); }
  DoserState state() const { return state_; }
  DoserFault fault() const { return fault_; }
  DoserPump pump() const { return active_; }
  uint8_t pulses() const { return tdsPulses_ + phPulses_; }
  DoserPhase phase() const { return phase_; }
private:
  void allOff() {
    digitalWrite(PH_UP_RELAY_PIN, DOSER_RELAY_OFF);
    digitalWrite(PH_DOWN_RELAY_PIN, DOSER_RELAY_OFF);
    digitalWrite(NUTRIENT_RELAY_PIN, DOSER_RELAY_OFF);
    active_ = DP_NONE;
  }
  void lock(DoserFault fault) {
    if (state_ != DS_LOCKOUT) {
      Serial.print(F("DOSER: LOCKOUT reason=")); Serial.println((int)fault);
    }
    allOff();
    state_ = DS_LOCKOUT;
    fault_ = fault;
  }
  DoserState state_ = DS_LOCKOUT;
  DoserPhase phase_ = DPH_TDS;
  DoserFault fault_ = DF_NONE;
  DoserPump active_ = DP_NONE;
  DoserPump candidate_ = DP_NONE;
  unsigned long stateSince_ = 0;
  unsigned long validSince_ = 0;
  uint8_t tdsPulses_ = 0;
  uint8_t phPulses_ = 0;
};
#endif
