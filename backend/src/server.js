const express = require("express");
const cors = require("cors");
const config = require("./config");
const { checkDatabase } = require("./db");
const { startMqtt, stopMqtt } = require("./mqtt");
const apiRoutes = require("./routes");

async function main() {
  await checkDatabase();
  console.log("[DB] Connected to MySQL");

  const app = express();
  app.use(cors({ origin: config.corsOrigin }));
  app.use(express.json({ limit: "32kb" }));
  app.use("/api", apiRoutes);

  app.use((error, _req, res, _next) => {
    console.error("[API] Error:", error.message);
    res.status(500).json({ error: "Internal server error" });
  });

  const server = app.listen(config.port, config.host, () => {
    console.log(`[API] Listening on http://${config.host}:${config.port}`);
  });

  startMqtt();

  const shutdown = () => {
    console.log("[APP] Shutting down...");
    stopMqtt();
    server.close(() => process.exit(0));
    setTimeout(() => process.exit(1), 5000).unref();
  };
  process.on("SIGINT", shutdown);
  process.on("SIGTERM", shutdown);
}

main().catch((error) => {
  console.error("[APP] Startup failed:", error.message);
  process.exit(1);
});
