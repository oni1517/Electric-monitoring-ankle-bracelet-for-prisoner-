#include <BLEDevice.h>
#include <BLEScan.h>
#include <BLEAdvertisedDevice.h>
#include <TinyGPSPlus.h>
#include <HardwareSerial.h>
#include "BluetoothSerial.h"
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

/* OLED */
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

/* BLE */
BLEScan* pBLEScan;
std::string targetMAC = "50:f1:4a:49:90:3a";//Add your own BLE ID

/* GPS */
TinyGPSPlus gps;
HardwareSerial gpsSerial(2);

/* Bluetooth */
BluetoothSerial SerialBT;

/* Pins */ 
#define BUZZER 25
#define PULSE_PIN 34
#define RXD2 16
#define TXD2 17


/* BLE SETTINGS */
int threshold = -100;
int lostCount = 0;
int lostLimit = 20;
int avgRSSI = -100;

/* GPS interval */
unsigned long lastSend = 0;
int sendInterval = 10000;

/* SHARED PULSE VARS (written by Core 0, read by Core 1) */
volatile int sharedBPM = 0;
volatile int sharedAmplitude = 0;
volatile int sharedLowBPMCount = 0;

/* PULSE TASK - runs on Core 0, unblocked by BLE */
void pulseTask(void* param) {
  unsigned long windowStart = millis();
  int beatCount = 0;
  bool beatDetected = false;
  int signalMax = 0;
  int signalMin = 4095;
  int bpmHistory[5] = {0};
  int histIndex = 0;
  unsigned long lastBeatTime = 0;
  int lowBPMCount = 0;

  while (true) {
    int signal = analogRead(PULSE_PIN);

    if (signal > signalMax) signalMax = signal;
    if (signal < signalMin) signalMin = signal;
    int amplitude = signalMax - signalMin;
    int pulseThreshold = (signalMax + signalMin) / 2;

    if (amplitude > 50) {
      if (signal > pulseThreshold && !beatDetected) {
        unsigned long now = millis();
        if (now - lastBeatTime > 450) {
          beatDetected = true;
          beatCount++;
          lastBeatTime = now;
        }
      }
      if (signal < pulseThreshold - 25) beatDetected = false;
    }

    if (millis() - windowStart >= 5000) {
      unsigned long actualWindow = millis() - windowStart;
      int rawBPM = (beatCount * 60000) / actualWindow;

      bpmHistory[histIndex] = rawBPM;
      histIndex = (histIndex + 1) % 5;
      int sum = 0;
      for (int i = 0; i < 5; i++) sum += bpmHistory[i];
      int bpm = sum / 5;

      if (amplitude < 50 || bpm < 40) lowBPMCount++;
      else lowBPMCount = 0;

      sharedBPM = bpm;
      sharedAmplitude = amplitude;
      sharedLowBPMCount = lowBPMCount;

      Serial.print("BPM: "); Serial.print(bpm);
      Serial.print(" | Amp: "); Serial.print(amplitude);
      Serial.print(" | LowCount: "); Serial.println(lowBPMCount);

      beatCount = 0;
      windowStart = millis();
      signalMax = 0;
      signalMin = 4095;
    }

    delay(15);
  }
}

void setup() {
  Serial.begin(115200);
 
  pinMode(BUZZER, OUTPUT);
  gpsSerial.begin(9600, SERIAL_8N1, RXD2, TXD2);

  /* OLED INIT */
  Wire.begin(21, 22);
  display.begin(SSD1306_SWITCHCAPVCC, 0x3C);
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(WHITE);
  display.println("Bracelet Booting");
  display.display();

  /* Bluetooth */
  SerialBT.begin("ESP32_GPS_TRACKER");

  /* BLE INIT */
  BLEDevice::init("");
  pBLEScan = BLEDevice::getScan();
  pBLEScan->setActiveScan(true);
  pBLEScan->setInterval(100);
  pBLEScan->setWindow(99);

  /* LAUNCH PULSE TASK ON CORE 0 */
  xTaskCreatePinnedToCore(pulseTask, "pulseTask", 4096, NULL, 1, NULL, 0);

  delay(1000);
}

void loop() {
  /* READ GPS */
  while (gpsSerial.available()) {
    gps.encode(gpsSerial.read());
  }
  float lat = gps.location.lat();
  float lon = gps.location.lng();

  /* READ SHARED PULSE DATA */
  int currentBPM = sharedBPM;
  int amplitude = sharedAmplitude;
  int lowBPMCount = sharedLowBPMCount;

  /* BLE SCAN */
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
      avgRSSI = (avgRSSI * 3 + rssiValue) / 4;
    }
  }
  if (!found) lostCount++;
  pBLEScan->clearResults();

  /* STATUS LOGIC */
  bool tamper = (lowBPMCount >= 3);
  bool buzzerState = false;
  bool alert = false;
  String status = "INSIDE";

  if (tamper) {
    buzzerState = true;
    alert = true;
    status = "TAMPER";
  } else if (lostCount >= lostLimit) {
    buzzerState = true;
    alert = true;
    status = "BEACON LOST";
  } else if (avgRSSI < threshold && lostCount > 2) {
    buzzerState = true;
    alert = true;
    status = "OUTSIDE";
  }

  digitalWrite(BUZZER, buzzerState);

  /* SEND GPS */
  if (alert && gps.location.isValid()) {
    if (SerialBT.hasClient()) {
      SerialBT.println("ALERT LOCATION");
      SerialBT.print("https://maps.google.com/?q=");
      SerialBT.print(lat, 6);
      SerialBT.print(",");
      SerialBT.println(lon, 6);
      SerialBT.println("----");
    }
  } else if (gps.location.isValid()) {
    if (millis() - lastSend > sendInterval) {
      if (SerialBT.hasClient()) {
        SerialBT.print("https://maps.google.com/?q=");
        SerialBT.print(lat, 6);
        SerialBT.print(",");
        SerialBT.println(lon, 6);
      }
      lastSend = millis();
    }
  }

  /* OLED DISPLAY */
  display.clearDisplay();
  display.setCursor(0, 0);
  display.println("Bracelet Monitor");
  display.println("----------------");
  display.print("BT: ");
  display.println(lostCount >= lostLimit ? "LOST" : "CONNECTED");
  display.print("Sat Lock: ");
  display.println(gps.location.isValid() ? "YES" : "NO");
  display.print("Miss: ");
  display.println(lostCount);
  display.print("BPM: ");
  if (amplitude < 50) display.println("--");
  else display.println(currentBPM);
  display.print("Status: ");
  display.println(status);
  display.print("Lat:");
  display.println(lat, 4);
  display.print("Lng:");
  display.println(lon, 4);
  display.display();
}