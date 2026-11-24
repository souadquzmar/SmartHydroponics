const express = require("express");
const { pool, checkDatabase } = require("./db");
const { getMqttStatus } = require("./mqtt");

const router = express.Router();

function getLimit(value) {
  const parsed = Number.parseInt(value || "100", 10);
  if (!Number.isFinite(parsed)) return 100;
  return Math.min(Math.max(parsed, 1), 500);
}

router.get("/health", async (_req, res) => {
  let database = "ok";
  try {
    await checkDatabase();
  } catch (_error) {
    database = "error";
  }
  const mqtt = getMqttStatus();
  res.status(database === "ok" ? 200 : 503).json({
    api: "ok",
    database,
    mqtt: mqtt.connected ? "connected" : "disconnected",
    lastMqttMessageAt: mqtt.lastMessageAt
  });
});

router.get("/telemetry/latest", async (_req, res, next) => {
  try {
    const [rows] = await pool.query("SELECT * FROM telemetry ORDER BY received_at DESC, id DESC LIMIT 1");
    res.json({ data: rows[0] || null });
  } catch (error) { next(error); }
});

router.get("/telemetry/history", async (req, res, next) => {
  try {
    const limit = getLimit(req.query.limit);
    const [rows] = await pool.query(
      "SELECT * FROM telemetry ORDER BY received_at DESC, id DESC LIMIT ?",
      [limit]
    );
    res.json({ count: rows.length, data: rows });
  } catch (error) { next(error); }
});

router.get("/events", async (req, res, next) => {
  try {
    const limit = getLimit(req.query.limit);
    const [rows] = await pool.query(
      "SELECT * FROM greenhouse_events ORDER BY received_at DESC, id DESC LIMIT ?",
      [limit]
    );
    res.json({ count: rows.length, data: rows });
  } catch (error) { next(error); }
});

router.get("/alerts", async (req, res, next) => {
  try {
    const limit = getLimit(req.query.limit);
    const [rows] = await pool.query(
      "SELECT * FROM greenhouse_alerts ORDER BY received_at DESC, id DESC LIMIT ?",
      [limit]
    );
    res.json({ count: rows.length, data: rows });
  } catch (error) { next(error); }
});

router.get("/system/status", async (_req, res, next) => {
  try {
    const [rows] = await pool.query("SELECT esp32_status, updated_at FROM system_status WHERE id = 1");
    res.json({ data: rows[0] || { esp32_status: "UNKNOWN", updated_at: null }, mqtt: getMqttStatus() });
  } catch (error) { next(error); }
});

module.exports = router;
