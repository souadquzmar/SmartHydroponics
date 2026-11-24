CREATE DATABASE IF NOT EXISTS smart_hydroponics
  CHARACTER SET utf8mb4
  COLLATE utf8mb4_unicode_ci;

-- Change this password before running the script.
CREATE USER IF NOT EXISTS 'greenhouse_app'@'localhost'
  IDENTIFIED BY 'CHANGE_THIS_DB_PASSWORD';
GRANT SELECT, INSERT, UPDATE ON smart_hydroponics.* TO 'greenhouse_app'@'localhost';
FLUSH PRIVILEGES;

USE smart_hydroponics;

CREATE TABLE IF NOT EXISTS telemetry (
  id BIGINT UNSIGNED NOT NULL AUTO_INCREMENT PRIMARY KEY,
  ph DECIMAL(6,2) NULL,
  tds DECIMAL(8,2) NULL,
  water_temp DECIMAL(6,2) NULL,
  humidity DECIMAL(6,2) NULL,
  air_temp DECIMAL(6,2) NULL,
  heater_on BOOLEAN NULL,
  light_on BOOLEAN NULL,
  lighting VARCHAR(32) NULL,
  fan_on BOOLEAN NULL,
  pump_on BOOLEAN NULL,
  flow_lpm DECIMAL(8,3) NULL,
  ldr INT NULL,
  led_pwm INT NULL,
  water_level VARCHAR(16) NULL,
  raw_message TEXT NOT NULL,
  received_at TIMESTAMP(3) NOT NULL DEFAULT CURRENT_TIMESTAMP(3),
  INDEX idx_telemetry_received_at (received_at)
);

CREATE TABLE IF NOT EXISTS greenhouse_events (
  id BIGINT UNSIGNED NOT NULL AUTO_INCREMENT PRIMARY KEY,
  topic VARCHAR(255) NOT NULL,
  message_type VARCHAR(16) NOT NULL,
  event_name VARCHAR(100) NULL,
  raw_message TEXT NOT NULL,
  received_at TIMESTAMP(3) NOT NULL DEFAULT CURRENT_TIMESTAMP(3),
  INDEX idx_events_received_at (received_at)
);

CREATE TABLE IF NOT EXISTS greenhouse_alerts (
  id BIGINT UNSIGNED NOT NULL AUTO_INCREMENT PRIMARY KEY,
  topic VARCHAR(255) NOT NULL,
  alert_name VARCHAR(100) NULL,
  raw_message TEXT NOT NULL,
  received_at TIMESTAMP(3) NOT NULL DEFAULT CURRENT_TIMESTAMP(3),
  INDEX idx_alerts_received_at (received_at)
);

CREATE TABLE IF NOT EXISTS system_status (
  id TINYINT UNSIGNED NOT NULL PRIMARY KEY,
  esp32_status VARCHAR(32) NOT NULL,
  updated_at TIMESTAMP(3) NOT NULL DEFAULT CURRENT_TIMESTAMP(3)
    ON UPDATE CURRENT_TIMESTAMP(3)
);

INSERT INTO system_status (id, esp32_status)
VALUES (1, 'UNKNOWN')
ON DUPLICATE KEY UPDATE id = id;
