const mqtt = require("mqtt");
const config = require("./config");
const { pool } = require("./db");

let client;
let connected = false;
let lastMessageAt = null;

function parseKeyValueMessage(raw) {
  const parts = raw.split(",");
  const type = (parts.shift() || "").trim();
  const values = {};

  for (const part of parts) {
    const separator = part.indexOf("=");
    if (separator < 1) continue;
    const key = part.slice(0, separator).trim();
    const value = part.slice(separator + 1).trim();
    values[key] = value;
  }
  return { type, values };
}

function numberOrNull(value) {
  if (value === undefined || value === null || value === "" || value === "UNKNOWN") return null;
  const number = Number(value);
  return Number.isFinite(number) ? number : null;
}

function boolOrNull(value) {
  if (value === undefined || value === null || value === "UNKNOWN") return null;
  if (value === "ON") return true;
  if (value === "OFF") return false;
  return null;
}

async function saveTelemetry(raw, values) {
  const sql = `
    INSERT INTO telemetry
    (ph, tds, water_temp, humidity, air_temp, heater_on, light_on,
     lighting, fan_on, pump_on, flow_lpm, ldr, led_pwm, water_level, raw_message)
    VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)
  `;
  const params = [
    numberOrNull(values.ph),
    numberOrNull(values.tds),
    numberOrNull(values.waterTemp),
    numberOrNull(values.humidity),
    numberOrNull(values.airTemp),
    boolOrNull(values.heater),
    boolOrNull(values.light),
    values.lighting || null,
    boolOrNull(values.fan),
    boolOrNull(values.pump),
    numberOrNull(values.flowLpm),
    numberOrNull(values.ldr),
    numberOrNull(values.ledPwm),
    values.water || null,
    raw
  ];
  await pool.execute(sql, params);
}

async function saveEvent(topic, raw, type, values) {
  const eventName = values.DOSER || values.event || values.name || null;
  await pool.execute(
    `INSERT INTO greenhouse_events (topic, message_type, event_name, raw_message)
     VALUES (?, ?, ?, ?)`,
    [topic, type, eventName, raw]
  );
}

async function saveAlert(topic, raw, values) {
  const alertName = values.DOSER || values.pump || values.water || values.fan || "ALERT";
  await pool.execute(
    `INSERT INTO greenhouse_alerts (topic, alert_name, raw_message) VALUES (?, ?, ?)`,
    [topic, alertName, raw]
  );
}

async function handleMessage(topic, payload) {
  const raw = payload.toString("utf8").trim();
  if (!raw) return;
  lastMessageAt = new Date();

  const { type, values } = parseKeyValueMessage(raw);

  if (topic === "greenhouse/system/status") {
    const status = raw.toUpperCase() === "ONLINE" ? "ONLINE" :
      raw.toUpperCase() === "OFFLINE" ? "OFFLINE" : raw.slice(0, 32);
    await pool.execute(
      `INSERT INTO system_status (id, esp32_status) VALUES (1, ?)
       ON DUPLICATE KEY UPDATE esp32_status = VALUES(esp32_status)`,
      [status]
    );
    return;
  }

  if (topic === "greenhouse/telemetry" && type === "DATA") {
    await saveTelemetry(raw, values);
    return;
  }

  if (topic === "greenhouse/events" && type === "INFO") {
    await saveEvent(topic, raw, type, values);
    return;
  }

  if (topic === "greenhouse/alerts" && type === "ALERT") {
    await saveAlert(topic, raw, values);
  }
}

function startMqtt() {
  client = mqtt.connect(config.mqtt.url, {
    username: config.mqtt.username,
    password: config.mqtt.password,
    clientId: `${config.mqtt.clientId}-${process.pid}`,
    clean: true,
    reconnectPeriod: 3000,
    connectTimeout: 10000
  });

  client.on("connect", () => {
    connected = true;
    console.log(`[MQTT] Connected to ${config.mqtt.url}`);
    client.subscribe(config.mqtt.topic, { qos: 0 }, (error) => {
      if (error) console.error("[MQTT] Subscribe failed:", error.message);
      else console.log(`[MQTT] Subscribed to ${config.mqtt.topic}`);
    });
  });

  client.on("reconnect", () => {
    connected = false;
    console.log("[MQTT] Reconnecting...");
  });

  client.on("close", () => { connected = false; });
  client.on("offline", () => { connected = false; });
  client.on("error", (error) => console.error("[MQTT] Error:", error.message));

  client.on("message", (topic, payload) => {
    handleMessage(topic, payload).catch((error) => {
      console.error(`[MQTT] Failed to process ${topic}:`, error.message);
    });
  });

  return client;
}

function getMqttStatus() {
  return { connected, lastMessageAt };
}

function stopMqtt() {
  if (client) client.end(true);
}

module.exports = { startMqtt, stopMqtt, getMqttStatus };
