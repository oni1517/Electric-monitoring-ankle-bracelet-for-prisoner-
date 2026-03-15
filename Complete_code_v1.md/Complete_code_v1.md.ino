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
std::string targetMAC = "50:f1:4a:49:90:3a";

/* GPS */
TinyGPSPlus gps;
HardwareSerial gpsSerial(2);

/* Bluetooth */
BluetoothSerial SerialBT;

/* Pins */
#define BUZZER 25
#define REED 27
#define RXD2 16
#define TXD2 17

/* BLE SETTINGS (restored for better range) */
int threshold = -100;
int lostCount = 0;
int lostLimit = 20;
int avgRSSI = -100;

/* GPS interval */
unsigned long lastSend = 0;
int sendInterval = 10000;

void setup() {

  Serial.begin(115200);

  pinMode(BUZZER, OUTPUT);
  pinMode(REED, INPUT_PULLUP);

  gpsSerial.begin(9600, SERIAL_8N1, RXD2, TXD2);

  /* OLED INIT */
  Wire.begin(21,22);
  display.begin(SSD1306_SWITCHCAPVCC,0x3C);
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

  delay(1000);
}

void loop() {

  /* READ GPS */
  while (gpsSerial.available()) {
    gps.encode(gpsSerial.read());
  }

  float lat = gps.location.lat();
  float lon = gps.location.lng();

  /* BLE SCAN */

  BLEScanResults* results = pBLEScan->start(2,false);

  bool found=false;
  int rssiValue=-100;

  for(int i=0;i<results->getCount();i++){

    BLEAdvertisedDevice device=results->getDevice(i);

    std::string addr = std::string(device.getAddress().toString().c_str());

    if(addr==targetMAC){

      found=true;
      lostCount=0;

      rssiValue=device.getRSSI();

      avgRSSI=(avgRSSI*3+rssiValue)/4;
    }
  }

  if(!found){
    lostCount++;
  }

  pBLEScan->clearResults();

  /* REED SENSOR */

  bool tamper=digitalRead(REED);

  /* STATUS LOGIC */

  bool buzzerState=false;
  bool alert=false;
  String status="INSIDE";

  if(tamper){

    buzzerState=true;
    alert=true;
    status="TAMPER";

  }
  else if(lostCount>=lostLimit){

    buzzerState=true;
    alert=true;
    status="BEACON LOST";

  }
  else if(avgRSSI<threshold && lostCount>2){

    buzzerState=true;
    alert=true;
    status="OUTSIDE";

  }

  digitalWrite(BUZZER,buzzerState);

  /* SEND GPS */

  if(alert && gps.location.isValid()){

    if(SerialBT.hasClient()){

      SerialBT.println("ALERT LOCATION");
      SerialBT.print("https://maps.google.com/?q=");
      SerialBT.print(lat,6);
      SerialBT.print(",");
      SerialBT.println(lon,6);
      SerialBT.println("----");

    }

  }
  else if(gps.location.isValid()){

    if(millis()-lastSend>sendInterval){

      if(SerialBT.hasClient()){

        SerialBT.print("https://maps.google.com/?q=");
        SerialBT.print(lat,6);
        SerialBT.print(",");
        SerialBT.println(lon,6);

      }

      lastSend=millis();
    }
  }

  /* OLED DISPLAY */

  display.clearDisplay();

  display.setCursor(0,0);
  display.println("Bracelet Monitor");
  display.println("----------------");

  display.print("BT: ");
  display.println(lostCount >= lostLimit ? "LOST" : "CONNECTED");

  display.print("Sat Lock: ");
  display.println(gps.location.isValid() ? "YES" : "NO");

  display.print("Miss: ");
  display.println(lostCount);

  display.print("Reed: ");
  display.println(tamper ? "OPEN" : "CLOSED");

  display.print("Status: ");
  display.println(status);

  display.print("Lat:");
  display.println(lat,4);

  display.print("Lng:");
  display.println(lon,4);

  display.display();
}