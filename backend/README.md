# Smart Hydroponic Greenhouse — Backend

Node.js + Express + MQTT.js + MySQL backend.

## What this starter does

- Subscribes to `greenhouse/#` on your existing Mosquitto broker.
- Parses the Mega's comma-separated `DATA`, `INFO`, and `ALERT` messages.
- Stores telemetry readings, events, and alerts in MySQL.
- Exposes read-only REST endpoints for the future web/mobile application.
- Tracks MQTT connection state and the latest ESP32 system status.

**It does not send commands to the Mega or ESP32.** Remote control is intentionally excluded until command validation and hardware safety rules are designed.

## 1. Requirements

- Node.js 20+
- MySQL 8+
- Your existing Mosquitto broker
- The ESP32 gateway publishing to `greenhouse/telemetry`, `greenhouse/events`, `greenhouse/alerts`, and `greenhouse/system/status`

## 2. Create the database

Log in to MySQL as an administrator and run `database/schema.sql`. For example:

```bash
mysql -u root -p < database/schema.sql
```

The schema creates the `smart_hydroponics` database and a restricted application user. **Change the example database password in the SQL file before running it.** Alternatively, create the database/user yourself and adjust `.env`.

## 3. Configure environment

```bash
cp .env.example .env
```

Edit `.env`:
- `MQTT_PASSWORD`: the password for your existing Mosquitto `greenhouse` user.
- `DB_PASSWORD`: the MySQL application user's password.
- Keep `MQTT_URL=mqtt://127.0.0.1:1883` when the backend runs on the same Mac as Mosquitto.
- If the broker is on another machine, use that machine's reachable IP instead of `127.0.0.1`.

Do not commit `.env` or share passwords.

## 4. Install and run

```bash
npm install
npm run dev
```

For normal execution:

```bash
npm start
```

The server listens on `http://127.0.0.1:3000` by default.

## 5. Test the API

```bash
curl http://127.0.0.1:3000/api/health
curl http://127.0.0.1:3000/api/telemetry/latest
curl "http://127.0.0.1:3000/api/telemetry/history?limit=20"
curl "http://127.0.0.1:3000/api/events?limit=20"
curl "http://127.0.0.1:3000/api/alerts?limit=20"
curl http://127.0.0.1:3000/api/system/status
```

Initially, history endpoints may return empty arrays until new MQTT messages arrive.

## MQTT message format supported

Telemetry example:

```text
DATA,ph=11.94,tds=779.0,waterTemp=17.81,humidity=63.2,airTemp=20.2,heater=ON,light=OFF,lighting=BRIGHT,fan=OFF,pump=OFF,flowLpm=0.00,ldr=76,ledPwm=0,water=UNKNOWN
```

Events and alerts are stored as their original message text, together with topic and timestamp. Telemetry fields that are `UNKNOWN` or not numeric are stored as SQL `NULL` where appropriate.

## API overview

| Method | Endpoint | Purpose |
|---|---|---|
| GET | `/api/health` | API, database, and MQTT health |
| GET | `/api/telemetry/latest` | Most recent telemetry row |
| GET | `/api/telemetry/history?limit=100` | Recent telemetry rows |
| GET | `/api/events?limit=100` | Recent INFO events |
| GET | `/api/alerts?limit=100` | Recent ALERT messages |
| GET | `/api/system/status` | Latest retained ESP32 status and MQTT connection |

All endpoints are read-only in this first milestone.

## Notes

- The backend subscribes with QoS 0 because the current ESP32 gateway publishes through PubSubClient at QoS 0.
- Messages published while the backend is offline are not guaranteed to be recovered unless the publisher/broker setup is later changed to queue or persist them.
- Use a local development network first. Before exposing the API outside your laptop/network, add authentication, authorization, HTTPS, input/rate limiting, and secure secret management.
