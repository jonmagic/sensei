#include "Adafruit_GPS.h"
#include <math.h>
#define mySerial Serial1
Adafruit_GPS GPS(&mySerial);
#define GPSECHO  false
boolean usingInterrupt = false;
#define APP_VERSION 10
byte bufferSize = 64;
byte bufferIndex = 0;
char buffer[65];
char c;
uint32_t timer;
int photoPin = A0;

double convertDegMinToDecDeg (float degMin) {
  double min = 0.0;
  double decDeg = 0.0;

  //get the minutes, fmod() requires double
  min = fmod((double)degMin, 100.0);

  //rebuild coordinates in decimal degrees
  degMin = (int) ( degMin / 100 );
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
  // in case you are not using the interrupt above, you'll
  // need to 'hand query' the GPS, not suggested :(
  if (! usingInterrupt) {
    // read data from the GPS in the 'main loop'
    char c = GPS.read();
    // if you want to debug, this is a good time to do it!
    if (GPSECHO)
      if (c) Serial.print(c);
  }

  if (GPS.newNMEAreceived()) {
    if (!GPS.parse(GPS.lastNMEA())) {
      if (millis() - timer > 10000) {
        Spark.publish("GPS", "{ last: \""+String(GPS.lastNMEA())+"\"}", 60, PRIVATE );
        Spark.publish("GPS", "{ error: \"failed to parse\"}", 60, PRIVATE );
      }
      return;
    }
  }

  if (timer > millis())  timer = millis();

  if (millis() - timer > 10000) {
    timer = millis(); // reset the timer

    Spark.publish(
      "GPS",
      "{ lat: " + String(convertDegMinToDecDeg(GPS.latitude))
      + ", lon: -" + String(convertDegMinToDecDeg(GPS.longitude))
      + ", a: " + String(GPS.altitude)
      + " }",
      60,
      PRIVATE
    );

    Spark.publish(
      "GPS",
      + "{ q: " + String(GPS.fixquality)
      + ", s: " + String((int)GPS.satellites)
      + " }",
      60,
      PRIVATE
    );

    Spark.publish("GPS_RAW", String(GPS.lastNMEA()), 60, PRIVATE );

    Spark.publish(
      "PHOTO",
      "{ photo: " + String(analogRead(photoPin)) + " }",
      60,
      PRIVATE
    );
  }
}
