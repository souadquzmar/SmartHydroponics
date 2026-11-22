# SmartHydroponics — flow-sensor-based dosing

Updated from `SmartHydroponics(8).ino`, `flow_pump(5).cpp`, and `flow_pump(5).h`. No float switch is used.

## Required files

Place these four files together in the Arduino sketch folder named `SmartHydroponics_Flow_Dosing_Fixed`:

- `SmartHydroponics_Flow_Dosing_Fixed.ino`: the updated Arduino Mega main sketch
- `automatic_dosing.h`: TDS-first dosing state machine
- `flow_pump.cpp`: original flow monitor plus `isFlowConfirmedForDosing()`
- `flow_pump.h`: original flow API plus new function declaration

You also MUST copy your real original modules and source files into the sketch folder: `ds18b20.h`, `tds_ec.h`, `ph_sensor.h`, `lighting.h`, `TempHumidity.h`, `water_heater.h`, `uart_mega.h`, and any corresponding `.cpp` or dependent files. These files were not provided and the full project cannot be compile-tested without them.

## Pin mapping

- **D2**: flow meter signal (interrupt, original)
- **D5**: circulation pump relay (original)
- **D30**: pH UP relay (proposed; verify no conflict)
- **D31**: pH DOWN relay (proposed; verify no conflict)
- **D32**: nutrient relay (proposed; verify no conflict)
- **No D33 float switch.**

All relay pins assume active-LOW boards. Dosing pumps must use rated relay/MOSFET drivers and an external power supply; never power from Arduino GPIO directly.

## Operation

- Circulation pump starts in `initFlowPump()`, with **60 seconds** grace before low-flow shutdown (your original behavior).
- Flow is counted on D2 using interrupts, sampled every 1 second, and converted to L/min using your **41.42** calibration constant.
- Dosing is *held OFF* until a sampled flow reading is **at least 0.50 L/min** after the circulation startup grace period.
- After first flow confirmation, dosing starts with another 60-second initial mixing delay.
- **TDS first:** if TDS <900 ppm, pulse nutrients for 150 ms, then wait 60 s before checking again. TDS >1000 ppm locks out dosing.
- **Then pH:** below 5.8 pulse pH UP, above 6.0 pulse pH DOWN, then wait 60 s before checking again.
- Each dosing action also requires 5 seconds of stable correction demand.
- If confirmed flow subsequently fails, ALL dosing pumps shut off and dosing locks out even if the circulation pump automatically retries.
- Maximum five pulses per TDS correction session and five per pH correction session (not persistent across power resets).

Your pump flow of **21 mL/s**, if measured correctly, suggests approximately **3.15 mL per 150 ms pulse**; short pulses must be measured directly. This can still be a substantial dose of concentrated pH adjuster in a 20 L reservoir.

## Activation and testing

`DOSING_COMMISSIONED` is intentionally `false` in the main sketch. First test relay OFF polarity, wiring, no pin conflicts, flow sensor and pump operation, pH and TDS readings, and fluid output at 150 ms. Set it to `true` **only after all checks pass** and safety controls are installed. Test dosing with plain water before adding chemicals.

To reset a latched dosing fault, inspect the cause, then send `RESET_DOSER` from USB Serial Monitor (9600 baud, newline). The system will wait for confirmed flow again; the reset command does not override flow interlock.

## What flow alone CANNOT establish

A flow meter provides evidence of circulation through the measured line, **not a reservoir-level reading**. It cannot guarantee the reservoir has enough water or that sensors and injection lines are correctly positioned. Telemetry explicitly leaves `waterLevelAvailable = false`.

## Outstanding limitations for unattended use

- Sensor modules were not supplied, so fresh, calibrated pH/TDS samples and validity flags cannot be confirmed. The current dose logic can use cached readings if their getters return old values.
- 150 ms timing is software-timed and may be lengthened by blocking operations elsewhere in the main loop. Check with a scope or timing meter and avoid blocking functions.
- Persist dose counters and fault state to EEPROM or other nonvolatile storage if unattended dosing is intended; power cycling currently clears counts and locks.
- Verify 60 seconds is sufficient for the actual mixing and that pH and nutrients are added at separate injection points.
- No flow sensor can replace dry-run/low-level protection in every failure mode.
