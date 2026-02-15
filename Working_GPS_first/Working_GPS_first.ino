#include <TinyGPS++.h>
#include <HardwareSerial.h>

TinyGPSPlus gps;
HardwareSerial gpsSerial(2);

#define LED_PIN 2

unsigned long lastBlink = 0;
bool ledState = false;

void setup() {
  Serial.begin(115200);
  gpsSerial.begin(9600, SERIAL_8N1, 16, 17);
  pinMode(LED_PIN, OUTPUT);

  Serial.println("🛰️ GPS Tracking Started...");
}

void loop() {

  while (gpsSerial.available()) {
    gps.encode(gpsSerial.read());
  }

  if (gps.charsProcessed() < 10) {
    Serial.println("❌ No GPS data");
    blinkLED(1000);  // Slow blink
  }
  else {
    if (gps.satellites.value() > 0) {
      Serial.print("🛰️ Satellites: ");
      Serial.println(gps.satellites.value());

      if (gps.location.isValid()) {
        Serial.print("📍 Latitude: ");
        Serial.println(gps.location.lat(), 6);

        Serial.print("📍 Longitude: ");
        Serial.println(gps.location.lng(), 6);

        Serial.println("-----------------------");
        blinkLED(200);   // Fast blink = LOCKED
      }
    }
    else {
      Serial.println("📡 GPS connected, searching satellites...");
      blinkLED(500);   // Medium blink
    }
  }

  delay(1000);
}

void blinkLED(int interval) {
  if (millis() - lastBlink > interval) {
    lastBlink = millis();
    ledState = !ledState;
    digitalWrite(LED_PIN, ledState);
  }
}
