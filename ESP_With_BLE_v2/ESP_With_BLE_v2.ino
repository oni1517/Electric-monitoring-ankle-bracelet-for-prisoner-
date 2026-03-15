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

std::string targetMAC = "50:f1:4a:49:90:3a";

int buzzerPin = 25;
int reedPin = 27;   // REED SENSOR PIN

/* ---- RANGE SETTINGS ---- */
int threshold = -100;
int lostCount = 0;
int lostLimit = 15;
/* ------------------------ */

int avgRSSI = -100;

void setup() {

  Serial.begin(115200);
  delay(1000);

  pinMode(buzzerPin, OUTPUT);
  pinMode(reedPin, INPUT_PULLUP);   // Reed switch

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

  BLEScanResults* results = pBLEScan->start(1, false);

  bool found = false;
  int rssiValue = -100;

  for (int i = 0; i < results->getCount(); i++) {

    BLEAdvertisedDevice device = results->getDevice(i);

    std::string addr = std::string(device.getAddress().toString().c_str());

    if (addr == targetMAC) {

      found = true;
      lostCount = 0;

      rssiValue = device.getRSSI();

      avgRSSI = (avgRSSI * 6 + rssiValue) / 7;

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

  /* ---- REED SENSOR CHECK ---- */
  bool tamper = digitalRead(reedPin);   // HIGH = open / tampered
  /* --------------------------- */

  bool buzzerState = false;
  String statusText = "INSIDE";

  if (tamper) {
    buzzerState = true;
    statusText = "TAMPER!";
  }
  else if (lostCount >= lostLimit) {
    buzzerState = true;
    statusText = "BEACON LOST";
  }
  else if (avgRSSI < threshold && lostCount > 2) {
    buzzerState = true;
    statusText = "OUTSIDE";
  }

  digitalWrite(buzzerPin, buzzerState ? HIGH : LOW);

  /* ---- OLED DISPLAY ---- */

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

  display.print("Reed: ");
  display.println(tamper ? "OPEN" : "CLOSED");

  display.print("Status: ");
  display.println(statusText);

  display.print("Buzzer: ");
  display.println(buzzerState ? "ON" : "OFF");

  display.display();

  pBLEScan->clearResults();

  delay(150);
}