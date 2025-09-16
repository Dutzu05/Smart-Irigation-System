#include <WiFi.h>
#include <WebServer.h>
#include <ArduinoJson.h>

// WiFi credentials
const char* ssid = "ESP32Test"; // Replace with your WiFi network SSID
const char* password = "12345678"; // Replace with your WiFi network password

// Sensor pins
const int sensorPins[3] = {15, 5, 18}; // Plant 1, Plant 2, Plant 3

// Valve pins
const int valvePins[3] = {19, 21, 22};

// Pump pin
const int pumpPin = 23;

// Calibration values for sensors (adjust based on your sensor's raw readings)
const int airValue = 3500; // Analog reading in air (dry); typically high (e.g., 3500–4095)
const int waterValue = 1500; // Analog reading in water (wet); typically low (e.g., 1000–1500)

// Timing
const unsigned long checkInterval = 20000; // 20 seconds
unsigned long lastCheck = 0;

// Moisture readings
int moistureValues[3] = {0, 0, 0};

// State tracking
bool pumpRunning = false;
bool valvesRunning[3] = {false, false, false};

WebServer server(80);

// Read moisture and convert to percentage
int readMoisture(int analogValue) {
  // Map higher analog values (dry) to 0% and lower values (wet) to 100%
  int percent = map(analogValue, airValue, waterValue, 0, 100);
  // Ensure percentage stays within 0–100
  if (percent < 0) percent = 0;
  if (percent > 100) percent = 100;
  return percent;
}

void handleStatus() {
  StaticJsonDocument<300> doc;
  JsonArray moistureArray = doc.createNestedArray("moisture");
  for (int i = 0; i < 3; i++) {
    moistureArray.add(moistureValues[i]);
  }
  doc["pumpRunning"] = pumpRunning;
  JsonArray valvesArray = doc.createNestedArray("valvesRunning");
  for (int i = 0; i < 3; i++) {
    valvesArray.add(valvesRunning[i]);
  }
  String response;
  serializeJson(doc, response);
  server.send(200, "application/json", response);
  Serial.println("Served /status request");
}

void startWatering(int plantIndex) {
  if (plantIndex < 0 || plantIndex > 2) return;
  digitalWrite(valvePins[plantIndex], HIGH);
  valvesRunning[plantIndex] = true;
  digitalWrite(pumpPin, HIGH);
  pumpRunning = true;
  Serial.printf("Started watering plant %d, Valve pin %d set HIGH, Pump pin %d set HIGH\n", 
                plantIndex + 1, valvePins[plantIndex], pumpPin);
}

void stopWatering(int plantIndex) {
  if (plantIndex < 0 || plantIndex > 2) return;
  digitalWrite(valvePins[plantIndex], LOW);
  valvesRunning[plantIndex] = false;
  bool anyValveOpen = false;
  for (int i = 0; i < 3; i++) {
    if (valvesRunning[i]) {
      anyValveOpen = true;
      break;
    }
  }
  if (!anyValveOpen) {
    digitalWrite(pumpPin, LOW);
    pumpRunning = false;
    Serial.printf("All valves closed, Pump pin %d set LOW\n", pumpPin);
  }
  Serial.printf("Stopped watering plant %d, Valve pin %d set LOW\n", 
                plantIndex + 1, valvePins[plantIndex]);
}

void handleValve1On() { startWatering(0); server.send(200, "text/plain", "Valve 1 + Pump ON"); }
void handleValve1Off() { stopWatering(0); server.send(200, "text/plain", "Valve 1 + Pump OFF"); }
void handleValve2On() { startWatering(1); server.send(200, "text/plain", "Valve 2 + Pump ON"); }
void handleValve2Off() { stopWatering(1); server.send(200, "text/plain", "Valve 2 + Pump OFF"); }
void handleValve3On() { startWatering(2); server.send(200, "text/plain", "Valve 3 + Pump ON"); }
void handleValve3Off() { stopWatering(2); server.send(200, "text/plain", "Valve 3 + Pump OFF"); }

void setup() {
  Serial.begin(115200);

  // Attempt to connect to WiFi network
  Serial.println("Connecting to WiFi...");
  WiFi.begin(ssid, password);

  // Wait for connection with a 10-second timeout
  unsigned long startAttemptTime = millis();
  const unsigned long timeout = 10000; // 10 seconds

  while (WiFi.status() != WL_CONNECTED && millis() - startAttemptTime < timeout) {
    delay(500);
    Serial.print(".");
  }

  // Check connection status
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\nWiFi Connected");
    Serial.print("Use this IP in MAUI app: ");
    Serial.println(WiFi.localIP());
  } else {
    Serial.println("\nWiFi Connection Failed. Check SSID/password or network availability.");
    return; // Skip server setup if no connection
  }

  // Setup pins
  for (int i = 0; i < 3; i++) {
    pinMode(sensorPins[i], INPUT);
    pinMode(valvePins[i], OUTPUT);
    digitalWrite(valvePins[i], LOW);
  }
  pinMode(pumpPin, OUTPUT);
  digitalWrite(pumpPin, LOW);

  // Test pump pin
  Serial.println("Testing pump pin...");
  digitalWrite(pumpPin, HIGH);
  Serial.printf("Pump pin %d set HIGH for 2 seconds\n", pumpPin);
  delay(2000); // Hold HIGH for 2 seconds
  digitalWrite(pumpPin, LOW);
  Serial.printf("Pump pin %d set LOW\n", pumpPin);

  // Test sensors by reading raw values
  Serial.println("Initial sensor readings (raw analog values):");
  for (int i = 0; i < 3; i++) {
    int analogVal = analogRead(sensorPins[i]);
    Serial.printf("Plant %d sensor raw value: %d\n", i + 1, analogVal);
  }

  // Web server routes
  server.on("/status", handleStatus);
  server.on("/valve1on", handleValve1On);
  server.on("/valve1off", handleValve1Off);
  server.on("/valve2on", handleValve2On);
  server.on("/valve2off", handleValve2Off);
  server.on("/valve3on", handleValve3On);
  server.on("/valve3off", handleValve3Off);

  server.begin();
  Serial.println("Web server started");
}

void loop() {
  // Only handle client requests if WiFi is connected
  if (WiFi.status() == WL_CONNECTED) {
    server.handleClient();
  } else {
    Serial.println("WiFi disconnected. Attempting to reconnect...");
    WiFi.reconnect();
    delay(5000);
    if (WiFi.status() == WL_CONNECTED) {
      Serial.print("Reconnected. Use this IP in MAUI app: ");
      Serial.println(WiFi.localIP());
    }
  }

  unsigned long now = millis();
  if (now - lastCheck >= checkInterval) {
    lastCheck = now;
    for (int i = 0; i < 3; i++) {
      int analogVal = analogRead(sensorPins[i]);
      moistureValues[i] = readMoisture(analogVal);
      Serial.printf("Plant %d sensor raw value: %d, Moisture: %d%%\n", i + 1, analogVal, moistureValues[i]);
    }
  }
}
