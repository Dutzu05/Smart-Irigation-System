#include <WiFi.h>
#include <WebServer.h>
#include <ArduinoJson.h>

const char* ssid = "ESP32Test"; // Your phone's hotspot SSID
const char* password = "12345678"; // Your phone's hotspot password
const int sensorPin = 34; // GPIO 34 (ADC1)
const int relayPin = 32;
const int airValue = 3500; // Update after calibration
const int waterValue = 1500; // Update after calibration
const int threshold = 30;
const unsigned long checkInterval = 5000;

WebServer server(80);
unsigned long lastCheck = 0;
int currentMoisture = 0;
bool isDry = false;
bool pumpRunning = false;

void handleStatus() {
  StaticJsonDocument<200> doc;
  doc["moisture"] = currentMoisture;
  doc["dry"] = isDry;
  doc["pumpRunning"] = pumpRunning;
  String response;
  serializeJson(doc, response);
  server.send(200, "application/json", response);
  Serial.println("Served /status request");
}

void handlePumpOn() {
  digitalWrite(relayPin, HIGH); // Use LOW if relay is active-low
  pumpRunning = true;
  server.send(200, "text/plain", "Pump turned ON by user");
  Serial.println("Pump manually turned on");
}

void handlePumpOff() {
  digitalWrite(relayPin, LOW); // Use HIGH if relay is active-low
  pumpRunning = false;
  server.send(200, "text/plain", "Pump turned OFF by user");
  Serial.println("Pump manually turned off");
}

void calibrateSensor() {
  Serial.println("Calibration Mode: Place sensor in AIR (dry)...");
  for (int i = 0; i < 5; i++) {
    int raw = 0;
    const int numReadings = 5;
    for (int j = 0; j < numReadings; j++) {
      raw += analogRead(sensorPin);
      delay(10);
    }
    raw /= numReadings;
    float voltage = (raw / 4095.0) * 3.3;
    Serial.print("Raw value (air): ");
    Serial.print(raw);
    Serial.print(" | Voltage: ");
    Serial.print(voltage, 2);
    Serial.println("V");
    delay(1000);
  }
  Serial.println("Now place sensor in WATER...");
  for (int i = 0; i < 5; i++) {
    int raw = 0;
    const int numReadings = 5;
    for (int j = 0; j < numReadings; j++) {
      raw += analogRead(sensorPin);
      delay(10);
    }
    raw /= numReadings;
    float voltage = (raw / 4095.0) * 3.3;
    Serial.print("Raw value (water): ");
    Serial.print(raw);
    Serial.print(" | Voltage: ");
    Serial.print(voltage, 2);
    Serial.println("V");
    delay(1000);
  }
  Serial.println("Calibration complete. Update airValue and waterValue.");
}

void testADC() {
  Serial.println("Testing ADC on GPIO 34...");
  for (int i = 0; i < 5; i++) {
    int raw = analogRead(sensorPin);
    float voltage = (raw / 4095.0) * 3.3;
    Serial.print("Test raw value: ");
    Serial.print(raw);
    Serial.print(" | Voltage: ");
    Serial.print(voltage, 2);
    Serial.println("V");
    delay(1000);
  }
}

void setup() {
  Serial.begin(115200);
  delay(1000);
  Serial.println("ESP32 Booting...");

  pinMode(relayPin, OUTPUT);
  digitalWrite(relayPin, LOW);
  pinMode(sensorPin, INPUT);

  analogReadResolution(12);
  analogSetAttenuation(ADC_11db);

  Serial.println("Initial sensor reading...");
  int initialRaw = analogRead(sensorPin);
  float initialVoltage = (initialRaw / 4095.0) * 3.3;
  Serial.print("Raw value: ");
  Serial.print(initialRaw);
  Serial.print(" | Voltage: ");
  Serial.print(initialVoltage, 2);
  Serial.println("V");
  if (initialRaw == 0) {
    Serial.println("WARNING: Sensor reading 0. Check wiring or sensor.");
  }

  // Connect to phone's hotspot
  Serial.println("Connecting to WiFi...");
  WiFi.mode(WIFI_STA); // Station mode
  WiFi.begin(ssid, password);
  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 20) {
    delay(500);
    Serial.print(".");
    attempts++;
  }
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\nWiFi connected");
    Serial.print("IP Address: ");
    Serial.println(WiFi.localIP());
  } else {
    Serial.println("\nFailed to connect to WiFi. Restarting...");
    ESP.restart();
  }

  server.on("/status", handleStatus);
  server.on("/pump/on", handlePumpOn);
  server.on("/pump/off", handlePumpOff);
  server.onNotFound([]() {
    server.send(404, "text/plain", "Not Found");
    Serial.println("Served 404 request");
  });
  server.begin();
  Serial.println("Web server started");
}

void loop() {
  server.handleClient();

  if (millis() % 10000 == 0) {
    Serial.print("Server running. IP Address: ");
    Serial.println(WiFi.localIP());
    Serial.print("WiFi Status: ");
    Serial.println(WiFi.status() == WL_CONNECTED ? "Connected" : "Disconnected");
  }

  unsigned long now = millis();
  if (now - lastCheck >= checkInterval) {
    int rawValue = 0;
    const int numReadings = 5;
    for (int i = 0; i < numReadings; i++) {
      rawValue += analogRead(sensorPin);
      delay(10);
    }
    rawValue /= numReadings;
    float voltage = (rawValue / 4095.0) * 3.3;

    if (rawValue == 0) {
      Serial.println("Error: Sensor reading 0. Check wiring or sensor.");
    } else if (rawValue < 0 || rawValue > 4095) {
      Serial.println("Error: Invalid sensor reading");
    } else {
      currentMoisture = map(rawValue, airValue, waterValue, 0, 100);
      currentMoisture = constrain(currentMoisture, 0, 100);
      isDry = currentMoisture < threshold;

      Serial.print("Raw value: ");
      Serial.print(rawValue);
      Serial.print(" | Voltage: ");
      Serial.print(voltage, 2);
      Serial.print("V | Moisture: ");
      Serial.print(currentMoisture);
      Serial.println("%");
    }
    lastCheck = now;
  }
}