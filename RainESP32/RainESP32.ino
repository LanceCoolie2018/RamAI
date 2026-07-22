#include <WiFi.h>
#include <PubSubClient.h>

// WiFi + MQTT credentials live in secrets.h (gitignored).
// Copy secrets.example.h → secrets.h and fill in your values.
#include "secrets.h"

// ============== CONFIG ==============
const char* device_name = "RainSensor";
// ====================================

WiFiClient espClient;
PubSubClient client(espClient);

#define RAIN_DO_PIN 13
#define RAIN_AO_PIN 17

// Sleep control
bool inSleepMode = false;

#define SLEEP_CMD_TOPIC "funhouse/command/sleep"
#define WAKE_CMD_TOPIC  "funhouse/command/wake"
#define SLEEP_STATE_TOPIC "funhouse/state/sleep"

// Publish throttling
unsigned long lastPublishTime = 0;
const unsigned long PUBLISH_INTERVAL = 5000;  // 5 seconds

void setup() {
  Serial.begin(115200);
  pinMode(RAIN_DO_PIN, INPUT);

  setupWiFi();
  
  client.setServer(mqtt_server, mqtt_port);
  client.setCallback(mqttCallback);
  client.setKeepAlive(60);

  reconnect();

  client.publish(SLEEP_STATE_TOPIC, "AWAKE", true);
  Serial.println("RainSensor ready");
}

void loop() {
  if (!client.connected()) {
    reconnect();
  }
  client.loop();

  if (!inSleepMode) {
    if (millis() - lastPublishTime >= PUBLISH_INTERVAL) {
      readAndSendRainData();
      lastPublishTime = millis();
    }
  } else {
    delay(100);
  }
}

void readAndSendRainData() {
  int digitalValue = digitalRead(RAIN_DO_PIN);
  int analogValue = analogRead(RAIN_AO_PIN);
  bool isRaining = digitalValue == LOW;

  client.publish("smartbed/sensor/rain", isRaining ? "1" : "0", true);
  client.publish("smartbed/sensor/rain_level", String(analogValue).c_str(), true);

  Serial.printf("Rain: %s | Level: %d\n", isRaining ? "YES" : "NO", analogValue);
}

void mqttCallback(char* topic, byte* payload, unsigned int length) {
  if (length == 0) return;

  String msg = "";
  for (int i = 0; i < length; i++) msg += (char)payload[i];
  msg.trim();

  Serial.printf("MQTT RX → %s : %s\n", topic, msg.c_str());

  if (String(topic) == SLEEP_CMD_TOPIC) {
    if (msg == "ON" || msg == "1" || msg == "SLEEP") {
      enterLowPowerMode();
    } else if (msg == "OFF" || msg == "0" || msg == "WAKE") {
      wakeDevice();
    }
  } 
  else if (String(topic) == WAKE_CMD_TOPIC) {
    if (msg == "ON" || msg == "1" || msg == "WAKE") {
      wakeDevice();
    }
  }
}

void enterLowPowerMode() {
  inSleepMode = true;
  client.publish(SLEEP_STATE_TOPIC, "SLEEPING", true);
  Serial.println("→ Entering low power mode");
}

void wakeDevice() {
  inSleepMode = false;
  client.publish(SLEEP_STATE_TOPIC, "AWAKE", true);
  Serial.println("→ Woken up");
  lastPublishTime = millis(); // Force immediate publish after wake
}

void setupWiFi() { /* unchanged */ 
  WiFi.begin(ssid, password);
  Serial.print("Connecting to WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWiFi connected");
  Serial.print("IP: "); Serial.println(WiFi.localIP());
}

void reconnect() {
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    
    if (client.connect(device_name, mqtt_user, mqtt_pass)) {
      Serial.println("connected");
      
      client.subscribe(SLEEP_CMD_TOPIC);
      client.subscribe(WAKE_CMD_TOPIC);
      
      // Re-announce state
      client.publish(SLEEP_STATE_TOPIC, inSleepMode ? "SLEEPING" : "AWAKE", true);
    } else {
      Serial.print("failed, rc=");
      Serial.println(client.state());
      delay(5000);
    }
  }
}