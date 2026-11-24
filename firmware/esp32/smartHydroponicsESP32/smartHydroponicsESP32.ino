
#include <Arduino.h>
#include <WiFi.h>
#include <PubSubClient.h>
#include <string.h>

#include "config.h"

// =====================================================
// UART CONFIGURATION
// =====================================================

#define RXD2 16
#define TXD2 17

constexpr size_t BUFFER_SIZE = 200;

char rxBuffer[BUFFER_SIZE];
size_t rxIndex = 0;
bool discardingLine = false;

// =====================================================
// MQTT CONFIGURATION
// =====================================================

WiFiClient espClient;
PubSubClient mqttClient(espClient);

unsigned long lastWiFiAttempt = 0;
unsigned long lastMQTTAttempt = 0;

bool previousWiFiState = false;

unsigned long mqttPublishedCount = 0;
unsigned long mqttDroppedCount = 0;

// =====================================================
// DOSING MESSAGE DISPLAY
// =====================================================

void displayDosingMessage(const char* message) {

  if (strstr(message, "DOSER,DISABLED")) {
    Serial.println("[DOSING] Automatic dosing disabled");
  }

  else if (strstr(message, "DOSER,WAIT_FLOW")) {
    Serial.println("[DOSING] Waiting for circulation flow...");
  }

  else if (strstr(message, "DOSER,FLOW_OK")) {
    Serial.println("[DOSING] Circulation flow confirmed");
  }

  else if (strstr(message, "DOSER,FLOW_LOST")) {
    Serial.println("[DOSING ALERT] Circulation flow lost!");
  }

  else if (strstr(message, "DOSER,PHASE,TDS")) {
    Serial.println("[DOSING] Starting TDS adjustment phase");
  }

  else if (strstr(message, "DOSER,PHASE,PH")) {
    Serial.println("[DOSING] Starting pH adjustment phase");
  }

  else if (strstr(message, "DOSER,PUMP_ON,NUTRIENT")) {
    Serial.println("[DOSING] Nutrient pump ON");
  }

  else if (strstr(message, "DOSER,PUMP_ON,PH_UP")) {
    Serial.println("[DOSING] pH+ pump ON");
  }

  else if (strstr(message, "DOSER,PUMP_ON,PH_DOWN")) {
    Serial.println("[DOSING] pH- pump ON");
  }

  else if (strstr(message, "DOSER,PUMP_OFF")) {
    Serial.println("[DOSING] Dosing pump OFF");
  }

  else if (strstr(message, "DOSER,MIXING")) {
    Serial.println("[DOSING] Mixing solution...");
  }

  else if (strstr(message, "DOSER,MONITORING")) {
    Serial.println("[DOSING] Monitoring water parameters");
  }

  else if (strstr(message, "DOSER,LOCKOUT")) {
    Serial.println("[DOSING ALERT] Safety lockout activated!");
  }

  else if (strstr(message, "DOSER,RESET")) {
    Serial.println("[DOSING] System reset");
  }

  else {
    Serial.print("[DOSING] ");
    Serial.println(message);
  }
}

// =====================================================
// WIFI
// =====================================================

void startWiFiConnection() {

  Serial.println("\n[WiFi] Connecting to university network...");

  WiFi.mode(WIFI_STA);
  WiFi.setSleep(false);
  WiFi.setAutoReconnect(true);

  WiFi.begin(
    WIFI_SSID,
    WPA2_AUTH_PEAP,
    WIFI_IDENTITY,
    WIFI_USERNAME,
    WIFI_PASSWORD
  );

  lastWiFiAttempt = millis();
}

void updateWiFi() {

  bool connected = (WiFi.status() == WL_CONNECTED);

  if (connected && !previousWiFiState) {

    Serial.println("\n[WiFi] Connected!");
    Serial.print("[WiFi] IP: ");
    Serial.println(WiFi.localIP());

    Serial.print("[WiFi] Signal: ");
    Serial.print(WiFi.RSSI());
    Serial.println(" dBm");
  }

  if (!connected && previousWiFiState) {
    Serial.println("[WiFi] Connection lost!");
  }

  previousWiFiState = connected;

  if (!connected &&
      millis() - lastWiFiAttempt >= WIFI_RETRY_INTERVAL) {

    Serial.println("[WiFi] Retrying connection...");

    WiFi.disconnect();
    startWiFiConnection();
  }
}

// =====================================================
// MQTT
// =====================================================

bool connectMQTT() {

  if (WiFi.status() != WL_CONNECTED) {
    return false;
  }

  Serial.println("[MQTT] Connecting to broker...");

  bool connected = mqttClient.connect(
    "HydroponicESP32",
    MQTT_USERNAME,
    MQTT_PASSWORD,
    TOPIC_STATUS,
    1,
    true,
    "OFFLINE"
  );

  if (connected) {

    Serial.println("[MQTT] Connected!");

    mqttClient.publish(
      TOPIC_STATUS,
      "ONLINE",
      true
    );

    return true;
  }

  Serial.print("[MQTT] Connection failed. State: ");
  Serial.println(mqttClient.state());

  return false;
}

void updateMQTT() {

  if (WiFi.status() != WL_CONNECTED) {
    return;
  }

  if (!mqttClient.connected()) {

    if (millis() - lastMQTTAttempt >= MQTT_RETRY_INTERVAL) {

      lastMQTTAttempt = millis();
      connectMQTT();
    }
  }
  else {
    mqttClient.loop();
  }
}

// =====================================================
// FORWARD MEGA MESSAGES TO MQTT
// =====================================================

void publishMegaMessage(const char* message) {

  if (!mqttClient.connected()) {
    mqttDroppedCount++;
    return;
  }

  const char* topic = nullptr;

  if (strncmp(message, "DATA,", 5) == 0) {
    topic = TOPIC_TELEMETRY;
  }

  else if (strncmp(message, "ALERT,", 6) == 0) {
    topic = TOPIC_ALERTS;
  }

  else if (strncmp(message, "INFO,", 5) == 0) {
    topic = TOPIC_EVENTS;
  }

  if (topic == nullptr) {
    return;
  }

  if (mqttClient.publish(topic, message)) {
    mqttPublishedCount++;
  }
  else {
    mqttDroppedCount++;
    Serial.println("[MQTT] Message publish failed!");
  }
}

// =====================================================
// PROCESS MEGA MESSAGES
// =====================================================

void processMessage(const char* message) {

  // Forward the original message to MQTT.
  publishMegaMessage(message);

  // Preserve local Serial Monitor display.
  if (strncmp(message, "DATA,", 5) == 0) {

    Serial.println("\n[MEGA TELEMETRY]");
    Serial.println(message);
  }

  else if (
    strncmp(message, "INFO,DOSER,", 11) == 0 ||
    strncmp(message, "ALERT,DOSER,", 12) == 0
  ) {

    displayDosingMessage(message);
  }

  else if (strncmp(message, "ALERT,", 6) == 0) {

    Serial.print("[MEGA ALERT] ");
    Serial.println(message);
  }

  else if (strncmp(message, "INFO,", 5) == 0) {

    Serial.print("[MEGA INFO] ");
    Serial.println(message);
  }

  else {

    Serial.print("[MEGA] ");
    Serial.println(message);
  }
}

// =====================================================
// UART RECEPTION
// =====================================================

void readMegaUART() {

  while (Serial2.available()) {

    char incoming = Serial2.read();

    if (incoming == '\r') {
      continue;
    }

    if (incoming == '\n') {

      if (!discardingLine && rxIndex > 0) {

        rxBuffer[rxIndex] = '\0';
        processMessage(rxBuffer);
      }

      rxIndex = 0;
      discardingLine = false;
      continue;
    }

    if (discardingLine) {
      continue;
    }

    if (rxIndex < BUFFER_SIZE - 1) {
      rxBuffer[rxIndex++] = incoming;
    }
    else {

      Serial.println("[UART] Message too long. Discarding.");

      rxIndex = 0;
      discardingLine = true;
    }
  }
}

// =====================================================
// SETUP
// =====================================================

void setup() {

  Serial.begin(115200);

  Serial.println("\n================================");
  Serial.println(" SMART HYDROPONIC ESP32 GATEWAY");
  Serial.println(" UART + WiFi + MQTT");
  Serial.println("================================");

  // Configure UART reception.
  Serial2.setRxBufferSize(1024);

  Serial2.begin(
    9600,
    SERIAL_8N1,
    RXD2,
    TXD2
  );

  Serial.println("[UART] Serial2 initialized at 9600 baud");

  // Configure MQTT.
  mqttClient.setServer(
    MQTT_BROKER_HOST,
    MQTT_BROKER_PORT
  );

  mqttClient.setKeepAlive(20);
  mqttClient.setSocketTimeout(2);
  mqttClient.setBufferSize(512);

  // Start Wi-Fi.
  startWiFiConnection();
}

// =====================================================
// MAIN LOOP
// =====================================================

void loop() {

  // Always prioritize reading Mega messages.
  readMegaUART();

  // Maintain network connections.
  updateWiFi();
  updateMQTT();

  // Periodic diagnostics.
  static unsigned long lastStats = 0;

  if (millis() - lastStats >= 15000) {

    lastStats = millis();

    Serial.println("\n========== GATEWAY STATUS ==========");

    Serial.print("WiFi: ");
    Serial.println(
      WiFi.status() == WL_CONNECTED ? "CONNECTED" : "DISCONNECTED"
    );

    Serial.print("MQTT: ");
    Serial.println(
      mqttClient.connected() ? "CONNECTED" : "DISCONNECTED"
    );

    Serial.print("Published messages: ");
    Serial.println(mqttPublishedCount);

    Serial.print("Dropped messages: ");
    Serial.println(mqttDroppedCount);

    Serial.println("====================================");
  }
}
