#include <WiFi.h>
#include <PubSubClient.h>
#include <Adafruit_DotStar.h>
#include <Adafruit_ST7789.h>
#include <Adafruit_GFX.h>
#include <SPI.h>
#include <Adafruit_AHTX0.h>

// WiFi + MQTT credentials live in secrets.h (gitignored).
// Copy secrets.example.h → secrets.h and fill in your values.
#include "secrets.h"

// ============== CONFIG ==============
const char* device_name = "DegenStation";
// ====================================

WiFiClient espClient;
PubSubClient client(espClient);

Adafruit_DotStar dotstar(5, PIN_DOTSTAR_DATA, PIN_DOTSTAR_CLOCK, DOTSTAR_BRG);
Adafruit_AHTX0 aht;

#define TFT_CS        40
#define TFT_DC        39
#define TFT_RST       41
#define TFT_BACKLIGHT 21
Adafruit_ST7789 tft = Adafruit_ST7789(TFT_CS, TFT_DC, TFT_RST);

#define LIGHT_DO_PIN 13
#define LIGHT_AO_PIN 17
#define SLEEP_TOUCH_PIN 6

float temperature = 0.0;
float humidity = 0.0;
int lightLevel = 0;
bool isDark = false;
bool isRaining = false;
int brightness = 120;

String current_ssid = "Disconnected";
String current_ip = "";
int rssi = 0;

unsigned long lastButtonTime = 0;
bool inSleepMode = false;

// Last values for partial updates
float lastTemp = -999;
float lastHum = -999;
bool lastRaining = false;
String lastSsid = "";
String lastIp = "";

// MQTT Topics
#define SLEEP_CMD_TOPIC "funhouse/command/sleep"
#define WAKE_CMD_TOPIC "funhouse/command/wake"
#define SLEEP_STATE_TOPIC "funhouse/state/sleep"

void setup() {
  Serial.begin(115200);
  fullHardwareInit();

  setupWiFi();
  client.setServer(mqtt_server, mqtt_port);
  client.setCallback(mqttCallback);

  client.subscribe(SLEEP_CMD_TOPIC);
  client.subscribe(WAKE_CMD_TOPIC);
  client.subscribe("degenstation/leds/set");
  client.subscribe("smartbed/sensor/rain");

  client.publish(SLEEP_STATE_TOPIC, "AWAKE", true);

  updateDisplay();
  Serial.println("DegenStation ready");
}

void fullHardwareInit() {
  pinMode(TFT_BACKLIGHT, OUTPUT);
  digitalWrite(TFT_BACKLIGHT, LOW); delay(50);
  digitalWrite(TFT_BACKLIGHT, HIGH); delay(100);

  tft.init(240, 240);
  tft.setRotation(0);
  tft.fillScreen(ST77XX_BLACK);
  tft.setTextColor(ST77XX_WHITE);
  tft.setTextSize(2);

  dotstar.begin();
  dotstar.setBrightness(brightness);
  dotstar.fill(dotstar.Color(255, 0, 0));
  dotstar.show();

  if (!aht.begin()) Serial.println("AHT20 not found");
  pinMode(LIGHT_DO_PIN, INPUT);
}

void loop() {
  if (!client.connected()) reconnect();
  client.loop();

  unsigned long now = millis();

  if (now - lastButtonTime > 300) {
    if (touchRead(SLEEP_TOUCH_PIN) > 20000) {
      if (!inSleepMode) enterLowPowerMode();
      else wakeDevice();
      lastButtonTime = now;
    }

    if (touchRead(7) > 20000) {
      Serial.println("CT7 Touch - LEDs OFF");
      dotstar.fill(0); dotstar.show();
      client.publish("degenstation/leds/set", "off");
      lastButtonTime = now;
    }
    if (touchRead(8) > 20000) {
      Serial.println("CT8 Touch - LEDs ON");
      dotstar.fill(dotstar.Color(0, 255, 0)); dotstar.show();
      client.publish("degenstation/leds/set", "green");
      lastButtonTime = now;
    }

    // Inverted Brightness Slider
    for (int i = 9; i <= 13; i++) {
      if (touchRead(i) > 18000) {
        brightness = map(i, 9, 13, 255, 20);
        dotstar.setBrightness(brightness);
        dotstar.show();
        lastButtonTime = now;
        break;
      }
    }
  }

  if (!inSleepMode && millis() % 4000 < 100) {
    readAndSendSensors();
    updateNetworkStatus();
    updateDisplayPartial();
  } else if (inSleepMode) {
    delay(50);
  }
}

void enterLowPowerMode() {
  inSleepMode = true;
  client.publish(SLEEP_STATE_TOPIC, "SLEEPING", true);
  digitalWrite(TFT_BACKLIGHT, LOW);
  dotstar.fill(0); dotstar.show();
}

void wakeDevice() {
  inSleepMode = false;
  fullHardwareInit();
  client.publish(SLEEP_STATE_TOPIC, "AWAKE", true);
  updateDisplay();
}

void mqttCallback(char* topic, byte* payload, unsigned int length) {
  String msg = "";
  for (int i = 0; i < length; i++) msg += (char)payload[i];

  if (String(topic) == SLEEP_CMD_TOPIC && msg == "ON") enterLowPowerMode();
  else if (String(topic) == WAKE_CMD_TOPIC && msg == "ON") wakeDevice();

  if (String(topic) == "smartbed/sensor/rain") isRaining = (payload[0] == '1');

  if (msg.indexOf("red") >= 0) dotstar.fill(dotstar.Color(0, 255, 0));
  else if (msg.indexOf("green") >= 0) dotstar.fill(dotstar.Color(255, 0, 0));
  else if (msg.indexOf("blue") >= 0) dotstar.fill(dotstar.Color(0, 0, 255));
  else if (msg.indexOf("off") >= 0) dotstar.fill(0);
  dotstar.show();
}

void readAndSendSensors() {
  sensors_event_t temp_event, humidity_event;
  aht.getEvent(&humidity_event, &temp_event);

  temperature = temp_event.temperature * 9.0 / 5.0 + 32.0;
  humidity = humidity_event.relative_humidity;

  lightLevel = analogRead(LIGHT_AO_PIN);
  isDark = digitalRead(LIGHT_DO_PIN) == HIGH;

  char tempStr[8]; dtostrf(temperature, 4, 1, tempStr);
  client.publish("smartbed/sensor/temperature", tempStr);

  char humStr[8]; dtostrf(humidity, 4, 1, humStr);
  client.publish("smartbed/sensor/humidity", humStr);

  char lightStr[8]; dtostrf(lightLevel, 5, 0, lightStr);
  client.publish("smartbed/sensor/light", lightStr);

  client.publish("smartbed/sensor/dark", isDark ? "1" : "0");
}

void updateNetworkStatus() {
  current_ssid = WiFi.SSID();
  current_ip = WiFi.localIP().toString();
  rssi = WiFi.RSSI();
}

void updateDisplayPartial() {
  tft.setTextSize(2);

  // Large Title
  tft.setTextSize(3);
  tft.setCursor(10, 10);
  tft.setTextColor(ST77XX_WHITE);
  tft.print("Smart Bed");
  tft.setTextSize(2);

  // Network (kept minimal)
  if (current_ssid != lastSsid || current_ip != lastIp) {
    tft.fillRect(10, 70, 220, 40, ST77XX_BLACK);
    tft.setCursor(10, 70); tft.print("Net: " + current_ssid);
    tft.setCursor(10, 95); tft.print("IP: " + current_ip);
    lastSsid = current_ssid;
    lastIp = current_ip;
  }

  // Temperature
  if (abs(temperature - lastTemp) > 0.2) {
    tft.fillRect(10, 130, 220, 20, ST77XX_BLACK);
    tft.setCursor(10, 130); tft.printf("Temp: %.1f F", temperature);
    lastTemp = temperature;
  }

  // Humidity
  if (abs(humidity - lastHum) > 0.5) {
    tft.fillRect(10, 160, 220, 20, ST77XX_BLACK);
    tft.setCursor(10, 160); tft.printf("Hum: %.1f %%", humidity);
    lastHum = humidity;
  }

  // Rain Indicator (promoted)
  if (isRaining != lastRaining) {
    tft.fillRect(10, 200, 220, 30, ST77XX_BLACK);
    tft.setCursor(10, 200);
    if (isRaining) {
      tft.setTextColor(ST77XX_RED);
      tft.setTextSize(3);
      tft.print("RAINING!");
    } else {
      tft.setTextColor(ST77XX_GREEN);
      tft.setTextSize(2);
      tft.print("Dry");
    }
    lastRaining = isRaining;
  }
}

void updateDisplay() {
  tft.fillScreen(ST77XX_BLACK);
  updateDisplayPartial();
}

void setupWiFi() {
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) delay(500);
  Serial.println("WiFi connected");
}

void reconnect() {
  while (!client.connected()) {
    if (client.connect(device_name, mqtt_user, mqtt_pass)) {
      Serial.println("MQTT connected");
      client.subscribe(SLEEP_CMD_TOPIC);
      client.subscribe(WAKE_CMD_TOPIC);
      client.subscribe("degenstation/leds/set");
      client.subscribe("smartbed/sensor/rain");
    } else {
      delay(5000);
    }
  }
}