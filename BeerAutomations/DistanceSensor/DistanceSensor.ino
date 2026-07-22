#include <Wire.h>
#include <Adafruit_VL53L0X.h>
#include <Adafruit_PWMServoDriver.h>
#include <WiFi.h>
#include <PubSubClient.h>
#include <SPI.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ILI9341.h>
#include <Adafruit_TSC2007.h>
#include <Preferences.h>

// WiFi + MQTT credentials live in secrets.h (gitignored).
// Copy secrets.example.h → secrets.h and fill in your values.
#include "secrets.h"

// ============== CONFIG ==============
const char* device_name = "KegBot";
// ====================================

// TFT Pins
#define TFT_CS   9
#define TFT_DC  10
#define TFT_RST -1

Adafruit_ILI9341 tft = Adafruit_ILI9341(TFT_CS, TFT_DC, TFT_RST);
Adafruit_TSC2007 ts = Adafruit_TSC2007();

Adafruit_PWMServoDriver pwm = Adafruit_PWMServoDriver();
Adafruit_VL53L0X lox = Adafruit_VL53L0X();

WiFiClient espClient;
PubSubClient client(espClient);

Preferences prefs;

// === Cooler Temperature (Throttled) ===
float coolerTempF = -999.0;
unsigned long lastTempDisplayUpdate = 0;
const unsigned long TEMP_DISPLAY_INTERVAL = 300000UL;  // 5 minutes

// Beer Tap + Keg Settings
const int CUP_DETECTION_THRESHOLD_MM = 200;
const int STOP_MARGIN_MM = 65;
const int CUP_REMOVAL_THRESHOLD_MM = 250;

const int SERVO_CHANNEL = 0;
const int SERVO_OPEN_ANGLE = 47;
const int SERVO_CLOSED_ANGLE = 3;

const unsigned long CUP_CONFIRM_MS = 700;
const unsigned long COOLDOWN_MS = 3000;

const float KEG_CAPACITY_ML = 6600.0;
float totalDispensed_ml = 0.0;
float FLOW_RATE_ML_PER_SECOND = 22.0;

bool pouring = false;
unsigned long pourStartTime = 0;
unsigned long lastStopTime = 0;
unsigned long cupDetectedTime = 0;
unsigned long lastMeasurementTime = 0;
float lastPourVolume = 0;

int distanceHistory[6] = {0};
int historyIndex = 0;

float lastDisplayedPercent = -1;   // Track to avoid flicker

void setup() {
  Serial.begin(115200);
  Wire.begin();
  pwm.begin();
  pwm.setPWMFreq(50);
  setServoAngle(SERVO_CLOSED_ANGLE);

  // TFT Setup
  tft.begin();
  tft.setTextWrap(false);
  tft.setRotation(3);
  tft.fillScreen(ILI9341_BLACK);
  ts.begin();

  if (!lox.begin()) {
    Serial.println("VL53L0X failed");
    while(1);
  }

  setupWiFi();
  client.setServer(mqtt_server, mqtt_port);
  client.setCallback(mqttCallback);

  prefs.begin("kegbot", false);  // Open storage
  totalDispensed_ml = prefs.getFloat("total_dispensed", 0.0);
  Serial.printf("Loaded saved keg usage: %.0f ml\n", totalDispensed_ml);

  Serial.println("\n=== DegenStation KegBOT with TFT Ready ===\n");
  drawStaticUI();
  updateDisplay();

  // Subscribe after connection is ready
  client.subscribe("kegstat/sensor/temperature_f");
  Serial.println("Subscribed to kegstat temperature (5-min display throttle)");
}

void loop() {
  if (!client.connected()) reconnect();
  client.loop();

  if (millis() - lastMeasurementTime >= 50) {
    lastMeasurementTime = millis();

    VL53L0X_RangingMeasurementData_t measure;
    lox.rangingTest(&measure, false);
    if (measure.RangeStatus == 4) return;

    int raw = measure.RangeMilliMeter;

    distanceHistory[historyIndex] = raw;
    historyIndex = (historyIndex + 1) % 6;
    int distance = 0;
    for (int i = 0; i < 6; i++) distance += distanceHistory[i];
    distance /= 6;

    if (!pouring) {
      if (distance < CUP_DETECTION_THRESHOLD_MM && distance > 40) {
        if (cupDetectedTime == 0) cupDetectedTime = millis();
        else if (millis() - cupDetectedTime >= CUP_CONFIRM_MS) {
          if (millis() - lastStopTime >= COOLDOWN_MS) {
            startPouring();
          }
        }
      } else {
        cupDetectedTime = 0;
      }
    } else {
      if (distance >= CUP_REMOVAL_THRESHOLD_MM || distance <= STOP_MARGIN_MM) {
        stopPouring();
      }
    }
  }

  // Update display only when needed for percentage
  float currentPercent = max(0.0, 100.0 - (totalDispensed_ml / KEG_CAPACITY_ML * 100.0));
  if (abs(currentPercent - lastDisplayedPercent) > 0.5) {
    updateDisplay();
    lastDisplayedPercent = currentPercent;
  }

  handleTouch();
}

void drawStaticUI() {
  tft.fillScreen(ILI9341_BLACK);
  tft.setRotation(0);
  tft.setTextColor(ILI9341_CYAN);
  tft.setTextSize(3);
  tft.setCursor(65, 10);
  tft.println("KegBOT");

  // Reset Button at bottom - centered
  tft.fillRoundRect(20, 260, 200, 50, 12, ILI9341_RED);
  tft.setTextColor(ILI9341_WHITE);
  tft.setTextSize(2);
  tft.setCursor(65, 275);
  tft.println("RESET KEG");
}

void updateDisplay() {
  float percent = max(0.0, 100.0 - (totalDispensed_ml / KEG_CAPACITY_ML * 100.0));

  // Clear dynamic areas
  tft.fillRect(30, 55, 200, 95, ILI9341_BLACK);   // Percentage
  tft.fillRect(25, 160, 220, 100, ILI9341_BLACK); // Status + Temp + Last Pour

  // Percentage
  tft.setTextSize(7);
  tft.setTextColor(ILI9341_YELLOW);
  tft.setCursor(40, 65);
  tft.print(percent, 0);
  tft.println("%");

  // Status
  tft.setTextSize(2);
  tft.setCursor(55, 165);
  if (pouring) {
    tft.setTextColor(ILI9341_GREEN);
    tft.println("POURING NOW");
  } else {
    tft.setTextColor(ILI9341_WHITE);
    tft.println("Ready");
  }

  // Cooler Temperature - Simple F only (throttled)
  unsigned long now = millis();
  tft.setTextSize(2);
  tft.setCursor(15, 190);
  tft.setTextColor(ILI9341_CYAN);
  tft.print("Cooler: ");
  if (coolerTempF > -500) {
    if ((now - lastTempDisplayUpdate >= TEMP_DISPLAY_INTERVAL) || lastTempDisplayUpdate == 0) {
      tft.setTextColor(ILI9341_WHITE);
      tft.print(coolerTempF, 1);
      tft.print(" F");          // Simple F
      tft.println();
      lastTempDisplayUpdate = now;
    } else {
      tft.setTextColor(ILI9341_WHITE);
      tft.print(coolerTempF, 1);
      tft.print(" F");          // Simple F
      tft.println();
    }
  } else {
    tft.setTextColor(ILI9341_LIGHTGREY);
    tft.println("-- F");
  }

  // Last Pour
  tft.setTextSize(2);
  tft.setCursor(15, 215);
  tft.setTextColor(ILI9341_LIGHTGREY);
  tft.print("Last Pour: ");
  tft.setTextColor(ILI9341_WHITE);
  tft.print(lastPourVolume, 0);
  tft.println(" ml");
}
  

unsigned long lastTouchTime = 0;

void handleTouch() {
  TS_Point p = ts.getPoint();
  if (p.z > 20) {
    int16_t x = map(p.y, 250, 3750, 0, 320);
    int16_t y = map(p.x, 250, 3750, 240, 0);

    x = constrain(x, 0, 319);
    y = constrain(y, 0, 239);

    Serial.printf("Touch: x=%d, y=%d\n", x, y);

    if (x > 250 && x < 300 && y > 63 && y < 220) {
      if (millis() - lastTouchTime > 1000) {
        Serial.println("Reset button touched!");

        if (pouring) {
          pouring = false;
          setServoAngle(SERVO_CLOSED_ANGLE);
        }

        totalDispensed_ml = 0.0;
        lastPourVolume = 0.0;
        client.publish("kegbot/remaining_percent", "100", true);
        client.publish("kegbot/total_dispensed", "0", true);
        
        Serial.println("✅ KEG RESET SUCCESSFUL");
        updateDisplay();
        lastTouchTime = millis();
      }
    }
  }
}

void startPouring() {
  pouring = true;
  updateDisplay();
  pourStartTime = millis();
  cupDetectedTime = 0;
  Serial.println("\n>>> CUP CONFIRMED - POURING <<<");
  setServoAngle(SERVO_OPEN_ANGLE);
  client.publish("kegbot/status", "pouring", true);
}

void stopPouring() {
  pouring = false;
  lastStopTime = millis();
  float pourVolume = ((millis() - pourStartTime) / 1000.0) * FLOW_RATE_ML_PER_SECOND;
  totalDispensed_ml += pourVolume;
  lastPourVolume = pourVolume;

  setServoAngle(SERVO_CLOSED_ANGLE);
  client.publish("kegbot/status", "idle", true);
  client.publish("kegbot/pour_volume", String(pourVolume).c_str(), true);
  client.publish("kegbot/total_dispensed", String(totalDispensed_ml).c_str(), true);
  prefs.putFloat("total_dispensed", totalDispensed_ml);

  float percent = max(0.0, 100.0 - (totalDispensed_ml / KEG_CAPACITY_ML * 100.0));
  client.publish("kegbot/remaining_percent", String(percent, 1).c_str(), true);
  
  Serial.printf("Pour: %.0f ml | Remaining: %.1f%%\n", pourVolume, percent);
}

void setServoAngle(int angle) {
  int pulse = map(angle, 0, 180, 150, 600);
  pwm.setPWM(SERVO_CHANNEL, 0, pulse);
}

// WiFi + MQTT functions
void setupWiFi() {
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWiFi connected");
}

void reconnect() {
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    if (client.connect(device_name, mqtt_user, mqtt_pass)) {
      Serial.println("connected");
      client.subscribe("kegbot/command");
      Serial.println("Subscribed to kegbot/command");
      client.publish("kegbot/command", "", true);
      
      // Re-subscribe to temperature on reconnect
      client.subscribe("kegstat/sensor/temperature_f");
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      delay(5000);
    }
  }
}

void mqttCallback(char* topic, byte* payload, unsigned int length) {
  String receivedTopic = String(topic);
  String message = "";
  for (int i = 0; i < length; i++) message += (char)payload[i];

  Serial.printf("MQTT Received - Topic: [%s] | Message: [%s]\n", topic, message.c_str());

  if (receivedTopic == "kegstat/sensor/temperature_f") {
    coolerTempF = message.toFloat();
    Serial.printf("✅ Received Cooler Temp: %.1f °F\n", coolerTempF);
    
    // Force initial display
    if (lastTempDisplayUpdate == 0) {
      updateDisplay();
      lastTempDisplayUpdate = millis();
    }
  }

  if (receivedTopic == "kegbot/command") {
    if (message == "reset_keg") {
      totalDispensed_ml = 0.0;
      prefs.putFloat("total_dispensed", 0.0);
      client.publish("kegbot/remaining_percent", "100", true);
      client.publish("kegbot/total_dispensed", "0", true);
      Serial.println("✅ KEG RESET SUCCESSFUL");
    }
  }
}