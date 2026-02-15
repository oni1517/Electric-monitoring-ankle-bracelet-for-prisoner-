#include <BLEDevice.h>
#include <BLEScan.h>
#include <BLEAdvertisedDevice.h>

#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

BLEScan* pBLEScan;

std::string targetMAC = "50:f1:4a:49:90:3a";   // YOUR HM-10 MAC

int buzzerPin = 25;

/* ---- RANGE SETTINGS (for ~10m) ---- */
int threshold = -90;     // adjust after testing (-80 to -88 typical)
int lostCount = 0;
int lostLimit = 10;
/* ---------------------------------- */

int avgRSSI = -100;

void setup() {
  Serial.begin(115200);
  delay(1000);

  pinMode(buzzerPin, OUTPUT);

  // OLED
  Wire.begin(21, 22);
  display.begin(SSD1306_SWITCHCAPVCC, 0x3C);
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(WHITE);
  display.setCursor(0,0);
  display.println("Bracelet Starting...");
  display.display();
  delay(1000);
  display.clearDisplay();

  // BLE
  BLEDevice::init("");
  delay(500);
  Serial.println("BLE Ready");

  pBLEScan = BLEDevice::getScan();
  pBLEScan->setActiveScan(true);
}

void loop() {

  BLEScanResults results = pBLEScan->start(1, false);

  bool found = false;
  int rssiValue = -100;

  for (int i = 0; i < results.getCount(); i++) {
    BLEAdvertisedDevice device = results.getDevice(i);

    if (device.getAddress().toString() == targetMAC) {
      found = true;
      lostCount = 0;

      rssiValue = device.getRSSI();

      // Strong smoothing for stable distance
      avgRSSI = (avgRSSI * 4 + rssiValue) / 5;

      Serial.print("RSSI: ");
      Serial.print(rssiValue);
      Serial.print(" | AVG: ");
      Serial.println(avgRSSI);
    }
  }

  if (!found) {
    lostCount++;
    Serial.print("Missed: ");
    Serial.println(lostCount);
  }

  bool buzzerState = false;
  String statusText = "INSIDE";

  if (lostCount >= lostLimit) {
    buzzerState = true;
    statusText = "BEACON LOST";
  }
  else if (avgRSSI < threshold && lostCount > 1) {
    buzzerState = true;
    statusText = "OUTSIDE";
  }

  digitalWrite(buzzerPin, buzzerState ? HIGH : LOW);

  // OLED
  display.clearDisplay();
  display.setCursor(0,0);
  display.println("BLE Bracelet");
  display.println("----------------");
  display.print("RSSI: ");
  display.println(rssiValue);
  display.print("AVG: ");
  display.println(avgRSSI);
  display.print("Missed: ");
  display.println(lostCount);
  display.print("Status: ");
  display.println(statusText);
  display.print("Buzzer: ");
  display.println(buzzerState ? "ON" : "OFF");
  display.display();

  pBLEScan->clearResults();
  delay(150);
}
