#include "Adafruit_GPS.h"
#include <math.h>

#define mySerial Serial1
#define GPSECHO  false
#define APP_VERSION 10
#define PHOTO_PIN A0
#include <dht11.h>
#define DHT11_PIN 0

dht11 DHT;
Adafruit_GPS GPS(&mySerial);
boolean usingInterrupt = false;
byte bufferSize = 64;
byte bufferIndex = 0;
char buffer[65];
char c;
uint32_t timer;
uint32_t timer2;
int dhtValue;

double convertDegMinToDecDeg (float degMin) {
  double min = 0.0;
  double decDeg = 0.0;
  min = fmod((double)degMin, 100.0); //get the minutes, fmod() requires double
  degMin = (int) ( degMin / 100 ); //rebuild coordinates in decimal degrees
  decDeg = degMin + ( min / 60 );
  return decDeg;
}

void setup() {
  GPS.begin(9600);
  mySerial.begin(9600);
  Serial.begin(115200);
  GPS.sendCommand(PMTK_SET_NMEA_OUTPUT_RMCGGA);
  GPS.sendCommand(PMTK_SET_NMEA_UPDATE_1HZ);
  GPS.sendCommand(PGCMD_NOANTENNA);

  delay(1000);
  mySerial.println(PMTK_Q_RELEASE);
  timer = millis();
  timer2 = millis();
  Spark.publish("GPS", "{ status: \"started up! "+String(APP_VERSION)+"\"}", 60, PRIVATE );

  IPAddress myIP = WiFi.localIP();
  Spark.publish(
    "MY_IP",
    String(myIP[0]) + "." + String(myIP[1]) + "." + String(myIP[2]) + "." + String(myIP[3]),
    60,
    PRIVATE
  );
}

void loop() {
  if (! usingInterrupt) {
    // in case you are not using the interrupt above, you'll
    // need to 'hand query' the GPS, not suggested :(
    char c = GPS.read(); // read data from the GPS in the 'main loop'
  }
  DHT.read(DHT11_PIN);

  if (timer > millis()) timer = millis();
  if (timer2 > millis()) timer2 = millis();

  if (millis() - timer > 10000) {
    timer = millis(); // reset the timer

    Spark.publish(
      "PHOTO",
      "{ photo: " + String(analogRead(PHOTO_PIN)) + " }",
      60,
      PRIVATE
    );

    Spark.publish(
      "DHT",
      "{ humidity: " + String(DHT.humidity) + ", temperature: " + String(DHT.temperature) + " }",
      60,
      PRIVATE
    );

    if (GPS.fixquality > 0 && (int)GPS.satellites > 0) {
      Spark.publish(
        "GPS",
        "{ lat: " + String(convertDegMinToDecDeg(GPS.latitude))
        + ", lon: -" + String(convertDegMinToDecDeg(GPS.longitude))
        + ", a: " + String(GPS.altitude)
        + " }",
        60,
        PRIVATE
      );

      if (millis() - timer2 > 120000) {
        System.sleep(SLEEP_MODE_DEEP, 1200); // sleep for 20 minutes
      }
    } else {
      Spark.publish(
        "GPS",
        + "{ q: " + String(GPS.fixquality)
        + ", s: " + String((int)GPS.satellites)
        + " }",
        60,
        PRIVATE
      );

      Spark.publish("GPS_RAW", String(GPS.lastNMEA()), 60, PRIVATE );

      if (millis() - timer2 > 120000) {
        System.sleep(SLEEP_MODE_DEEP, 300); // sleep for 5 minutes
      }
    }
  }
}
