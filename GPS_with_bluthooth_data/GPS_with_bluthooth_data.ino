#include <TinyGPSPlus.h>
#include <HardwareSerial.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include "BluetoothSerial.h"

/* OLED SETTINGS */
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

/* GPS */
TinyGPSPlus gps;
HardwareSerial gpsSerial(2);

/* BLUETOOTH */
BluetoothSerial SerialBT;

/* GPS PINS */
#define RXD2 16
#define TXD2 17

unsigned long lastSend = 0;
int sendInterval = 10000;

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

  /* BLUETOOTH INIT */
  SerialBT.begin("ESP32_GPS_TRACKER"); // Device name

  display.setCursor(0,10);
  display.println("Bluetooth Ready");
  display.display();
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
      sendToBluetooth(lat, lon);
      lastSend = millis();
    }
  }

  updateOLED(lat, lon);
}

void sendToBluetooth(float lat, float lon) {

  if (SerialBT.hasClient()) {

    SerialBT.println("Location:");
    SerialBT.print("https://maps.google.com/?q=");
    SerialBT.print(lat, 6);
    SerialBT.print(",");
    SerialBT.println(lon, 6);
    SerialBT.println("-------------------");
  }
}

void updateOLED(float lat, float lon) {

  display.clearDisplay();
  display.setCursor(0,0);

  display.println("GPS TRACKER");
  display.println("----------------");

  display.print("BT: ");
  display.println(SerialBT.hasClient() ? "Connected" : "Waiting");

  display.print("GPS: ");
  display.println(gps.location.isValid() ? "Locked" : "Searching");

  display.print("Sats: ");
  display.println(gps.satellites.value());

  display.print("Lat:");
  display.println(lat, 4);

  display.print("Lng:");
  display.println(lon, 4);

  display.display();
}