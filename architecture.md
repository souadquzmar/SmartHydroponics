# Smart Hydroponic Greenhouse — Architecture

## 1. Project Overview

The Smart Hydroponic Greenhouse is a compact hydroponic system designed for residential use. It monitors environmental conditions and nutrient-solution parameters, automates selected operations, and supports remote monitoring.

The system is designed around two microcontrollers:

- **Arduino Mega 2560:** Local sensor acquisition, automatic control, actuator management, and safety logic.
- **ESP32:** Wi-Fi connectivity, communication with the backend, remote monitoring, and forwarding authorized remote commands to the Mega.

The system will be developed incrementally, with individual hardware components tested before full integration.

## 2. Hardware Components

### Sensors

| Sensor | Purpose |
|---|---|
| DHT22 | Ambient temperature and humidity |
| DS18B20 | Water temperature |
| pH sensor | Nutrient-solution acidity |
| TDS sensor | Estimated dissolved-solids concentration |
| LDR | Ambient light measurement |
| YF-S401 | Water flow measurement |
| Water-level sensor | Reservoir level monitoring |

### Actuators

- Four 12 V water pumps
- One 12 V fan
- One 12 V air pump
- Grow-light LED strip
- Water heater

The exact assignment of the four water pumps will be documented separately once their functions are finalized.

## 3. System Responsibilities

### 3.1 Arduino Mega 2560

The Mega is responsible for:

- Reading and processing sensor signals.
- Applying calibration to sensor measurements.
- Running local automatic-control logic.
- Controlling pumps, fan, lighting, and heater through suitable driver circuits.
- Enforcing local safety conditions.
- Exchanging sensor readings and commands with the ESP32 over UART.

Local safety decisions must not depend on an active Wi-Fi connection.

### 3.2 ESP32

The ESP32 is responsible for:

- Connecting the system to Wi-Fi.
- Communicating with the backend/API.
- Sending monitoring data to the backend.
- Receiving authorized remote commands.
- Forwarding validated commands to the Mega.
- Reporting connectivity or communication errors where supported.

### 3.3 Backend and Dashboard

These components will be developed in a later phase.

The planned functionality includes:

- Displaying current sensor readings.
- Recording historical measurements.
- Presenting alerts and system status.
- Supporting authorized remote controls where appropriate.

## 4. Communication Architecture

The Mega and ESP32 will communicate through UART serial communication.

The interface must define:

- Message format and delimiters.
- Sensor-data message structure.
- Command types and permitted values.
- Validation and error handling.
- Communication timeouts and recovery behavior.

Both boards must share a common ground for UART communication. Voltage levels must be checked, and appropriate level shifting must be used wherever required.

Remote commands must be validated before they can affect actuators.

## 5. Project Directory Structure

```text
SmartHydroponics/
├── README.md
├── architecture.md
│
├── hardware/
│   ├── wiring/
│   ├── pin-mapping.md
│   └── power-distribution.md
│
├── firmware/
│   ├── mega/
│   │   └── SmartHydroponics/
│   │       ├── SmartHydroponics.ino
│   │       ├── FlowSensor.h
│   │       ├── FlowSensor.cpp
│   │       ├── DHTSensor.h
│   │       ├── DHTSensor.cpp
│   │       ├── DS18B20Sensor.h
│   │       ├── DS18B20Sensor.cpp
│   │       ├── AnalogSensors.h
│   │       └── AnalogSensors.cpp
│   │
│   └── esp32/
│       └── README.md
│
├── backend/
│   └── README.md
│
├── dashboard/
│   └── README.md
│
├── tests/
│   └── README.md
│
└── simulation/
    └── README.md
```



