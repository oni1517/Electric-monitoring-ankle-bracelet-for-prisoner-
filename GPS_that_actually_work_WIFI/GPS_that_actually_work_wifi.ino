#include <WiFi.h>
#include <HTTPClient.h>
#include <TinyGPSPlus.h>
#include <HardwareSerial.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

/* OLED SETTINGS */
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

/* GPS */
TinyGPSPlus gps;
HardwareSerial gpsSerial(2);

/* WIFI */
const char* ssid = "abhyas zala ka";
const char* password = "nahi zala";

/* TELEGRAM */
String BOT_TOKEN = "";
String CHAT_ID   = "";

/* GPS PINS */
#define RXD2 16
#define TXD2 17

unsigned long lastSend = 0;
int sendInterval = 10000;

int lastTelegramResponse = 0;

void setup() {
  Serial.begin(115200);

  gpsSerial.begin(9600, SERIAL_8N1, RXD2, TXD2);

  /* OLED INIT */
  Wire.begin(21, 22);
  display.begin(SSD1306_SWITCHCAPVCC, 0x3C);
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(WHITE);

  display.setCursor(0,0);
  display.println("Starting...");
  display.display();

  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
  }
}

void loop() {

  while (gpsSerial.available()) {
    gps.encode(gpsSerial.read());
  }

  float lat = 0;
  float lon = 0;

  if (gps.location.isValid()) {
    lat = gps.location.lat();
    lon = gps.location.lng();

    if (millis() - lastSend > sendInterval) {
      sendToTelegram(lat, lon);
      lastSend = millis();
    }
  }

  updateOLED(lat, lon);
}

void sendToTelegram(float lat, float lon) {

  if (WiFi.status() == WL_CONNECTED) {

    HTTPClient http;

    String url = "https://api.telegram.org/bot" + BOT_TOKEN +
                 "/sendMessage?chat_id=" + CHAT_ID +
                 "&text=Location:%0Ahttps://maps.google.com/?q=" +
                 String(lat, 6) + "," + String(lon, 6);

    http.begin(url);
    lastTelegramResponse = http.GET();
    http.end();
  }
}

void updateOLED(float lat, float lon) {

  display.clearDisplay();
  display.setCursor(0,0);

  display.println("GPS TRACKER");
  display.println("----------------");

  display.print("WiFi: ");
  display.println(WiFi.status() == WL_CONNECTED ? "OK" : "Lost");

  display.print("GPS: ");
  display.println(gps.location.isValid() ? "Locked" : "Searching");

  display.print("Sats: ");
  display.println(gps.satellites.value());

  display.print("Lat:");
  display.println(lat, 4);

  display.print("Lng:");
  display.println(lon, 4);

  display.print("TG Resp:");
  display.println(lastTelegramResponse);

  display.display();
}
