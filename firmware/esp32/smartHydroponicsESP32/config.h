
#pragma once

// University Wi-Fi
#define WIFI_SSID       "Najah"
#define WIFI_USERNAME   "12220650"
#define WIFI_PASSWORD   "172604"
#define WIFI_IDENTITY   ""

// MQTT broker on your MacBook
#define MQTT_BROKER_HOST "172.23.22.149"
#define MQTT_BROKER_PORT 1883

// MQTT authentication
#define MQTT_USERNAME "greenhouse"
#define MQTT_PASSWORD "123456"
// MQTT topics
#define TOPIC_TELEMETRY "greenhouse/telemetry"
#define TOPIC_EVENTS    "greenhouse/events"
#define TOPIC_ALERTS    "greenhouse/alerts"
#define TOPIC_STATUS    "greenhouse/system/status"

// Reconnection intervals
#define WIFI_RETRY_INTERVAL 15000UL
#define MQTT_RETRY_INTERVAL 5000UL
