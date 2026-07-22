#include <WiFi.h>
#include <PubSubClient.h>
#include <OneWire.h>
#include <DallasTemperature.h>

// WiFi + MQTT credentials live in secrets.h (gitignored).
// Copy secrets.example.h → secrets.h and fill in your values.
#include "secrets.h"

// ============== CONFIG ==============
const char* device_name = "Kegstat";
// ====================================

WiFiClient espClient;
PubSubClient client(espClient);

OneWire oneWire;
DallasTemperature sensors;

const int ONE_WIRE_BUS = 6;   // Working pin from your test

float temperatureC = 0.0;
float temperatureF = 0.0;

unsigned long lastMsg = 0;
const long interval = 5000;   // Send every 5 seconds

void setup() {
  Serial.begin(115200);
  
  oneWire = OneWire(ONE_WIRE_BUS);
  sensors = DallasTemperature(&oneWire);
  sensors.begin();
  sensors.setResolution(12);

  setupWiFi();
  client.setServer(mqtt_server, mqtt_port);
  // No callback needed unless you want to receive commands later

  Serial.println("Kegstat ready");
}

void setupWiFi() {
  Serial.print("Connecting to ");
  Serial.println(ssid);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWiFi connected");
  Serial.print("IP: ");
  Serial.println(WiFi.localIP());
}

void reconnect() {
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection as ");
    Serial.println(device_name);
    
    if (client.connect(device_name, mqtt_user, mqtt_pass)) {
      Serial.println("MQTT connected");
      // No subscriptions needed for now (add later if you want commands)
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      delay(5000);
    }
  }
}

void loop() {
  if (!client.connected()) {
    reconnect();
  }
  client.loop();

  unsigned long now = millis();
  if (now - lastMsg > interval) {
    lastMsg = now;
    readAndSendTemperature();
  }
}

void readAndSendTemperature() {
  sensors.requestTemperatures();
  temperatureC = sensors.getTempCByIndex(0);
  temperatureF = sensors.getTempFByIndex(0);

  if (temperatureC != DEVICE_DISCONNECTED_C && temperatureC != -127.0) {
    char tempCStr[8];
    char tempFStr[8];
    dtostrf(temperatureC, 6, 2, tempCStr);
    dtostrf(temperatureF, 6, 2, tempFStr);

    // Publish using same style as your DegenStation / smartbed sensors
    client.publish("kegstat/sensor/temperature_c", tempCStr);
    client.publish("kegstat/sensor/temperature_f", tempFStr);

    Serial.print("✅ Kegstat Temperature: ");
    Serial.print(temperatureF);
    Serial.println(" °F");
  } else {
    Serial.println("DS18B20 read error");
  }
}