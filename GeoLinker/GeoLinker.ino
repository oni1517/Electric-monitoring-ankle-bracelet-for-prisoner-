#include <GeoLinker.h>
#include <WiFi.h>

// ================== GPS CONFIG ==================
HardwareSerial gpsSerial(1);
#define GPS_RX 16
#define GPS_TX 17
#define GPS_BAUD 9600

// ================== WIFI CONFIG =================
const char* ssid = "Shinde_sarkar";
const char* password = "ssoh0999";

// ================== GEOLINKER CONFIG ============
const char* apiKey = "cd_mem_120226_yT1psm";
const char* deviceID = "ESP32";

const uint16_t updateInterval = 2;
const bool enableOfflineStorage = true;
const uint8_t offlineBufferLimit = 20;
const bool enableAutoReconnect = true;

const int8_t timeOffsetHours = 5;
const int8_t timeOffsetMinutes = 30;

GeoLinker geo;

// ================== LED CONFIG ==================
#define LED_PIN 2
unsigned long lastBlink = 0;
bool ledState = false;

// =================================================
void setup() {
  Serial.begin(115200);
  delay(1000);

  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW);

  Serial.println("Starting GeoLinker Tracker...");

  // GPS Serial
  gpsSerial.begin(GPS_BAUD, SERIAL_8N1, GPS_RX, GPS_TX);
  Serial.println("GPS Serial started");

  // GeoLinker Init
  geo.begin(gpsSerial);
  geo.setApiKey(apiKey);
  geo.setDeviceID(deviceID);
  geo.setUpdateInterval_seconds(updateInterval);
  geo.setDebugLevel(DEBUG_BASIC);
  geo.enableOfflineStorage(enableOfflineStorage);
  geo.enableAutoReconnect(enableAutoReconnect);
  geo.setOfflineBufferLimit(offlineBufferLimit);
  geo.setTimeOffset(timeOffsetHours, timeOffsetMinutes);

  geo.setNetworkMode(GEOLINKER_WIFI);
  geo.setWiFiCredentials(ssid, password);

  Serial.print("Connecting WiFi...");
  if (!geo.connectToWiFi()) {
    Serial.println("FAILED ❌");
  } else {
    Serial.println("CONNECTED ✅");
  }

  Serial.println("Setup complete. Tracking started.\n");
}

// =================================================
void loop() {

  uint8_t status = geo.loop();

  if (status > 0) {

    switch(status) {

      case STATUS_SENT:
        Serial.println("🛰️ CLOUD OK → Data sent successfully (MAP LIVE)");
        digitalWrite(LED_PIN, HIGH);   // SOLID ON = Cloud receiving
        break;

      case STATUS_NETWORK_ERROR:
        Serial.println("📡 NETWORK ERROR → Reconnecting...");
        blinkLED(700);
        break;

      case STATUS_GPS_ERROR:
        Serial.println("❌ GPS ERROR → No Fix / Wiring issue");
        blinkLED(350);
        break;

      case STATUS_BAD_REQUEST_ERROR:
        Serial.println("🚫 SERVER REJECTED → Check API key / Device ID");
        blinkLED(120);
        break;

      case STATUS_PARSE_ERROR:
        Serial.println("⚠ GPS PARSE ERROR");
        blinkLED(120);
        break;

      case STATUS_INTERNAL_SERVER_ERROR:
        Serial.println("☁ SERVER ERROR → Try later");
        blinkLED(120);
        break;

      default:
        Serial.print("Unknown status: ");
        Serial.println(status);
        blinkLED(500);
        break;
    }
  }

  delay(50);
}

// =================================================
void blinkLED(int interval) {
  if (millis() - lastBlink > interval) {
    lastBlink = millis();
    ledState = !ledState;
    digitalWrite(LED_PIN, ledState);
  }
}
