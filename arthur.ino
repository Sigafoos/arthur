// @TODO include all licenses for code you reused

#include <Adafruit_MPL3115A2.h>
#include <Adafruit_MMA8451.h>
#include <Adafruit_Sensor.h>
#include <NeoSWSerial.h>
#include <SPI.h>
#include <SD.h>
#include <TinyGPS++.h>
#include <Wire.h>

// customization
static const int sd_pin = 8;
static const int gpsRX = 4, gpsTX = 3;
static const uint32_t gpsBaud = 9600;

// instantiate the interfaces
TinyGPSPlus gps;
NeoSWSerial gps_port(gpsRX, gpsTX);
Adafruit_MPL3115A2 baro = Adafruit_MPL3115A2();
Adafruit_MMA8451 accelerometer = Adafruit_MMA8451();
File fp;

// the variables where we store data
String timestamp;
int curr_tilt[3]; // x, y, z
int curr_accel[3]; // ibid
float curr_pressure;
float curr_altitude;
float curr_internal_temp;

void setup() {
  Serial.begin(9600);
  bool successful = true;

  gps_port.begin(gpsBaud);
  Serial.print("Initializing GPS... ");

  while (!gps.date.isValid() || !gps.time.isValid()) {
    while (gps_port.available() > 0) {
      gps.encode(gps_port.read());

      if (millis() > 5000 && gps.charsProcessed() < 10) {
        Serial.println("FAILED");
        successful = false;
      }
    }
  }
  Serial.println("OK");

  timestamp = formatDateTime(false);

  Serial.print("Initializing SD card... ");
  if (!SD.begin(sd_pin)) {
    Serial.println("FAILED");
    successful = false;
  } else {
    Serial.println("OK");

    if (!SD.exists("hab/")) {
      Serial.print("Creating directory "); Serial.print("hab/"); Serial.print("... ");
      if (!SD.mkdir("hab/")) {
        Serial.println("FAILED");
        successful = false;
      } else {
        Serial.println("OK");
      }
    } else {
      Serial.print("Directory "); Serial.print("hab/"); Serial.println(" exists");
    }
    Serial.print("Opening file "); Serial.print("hab/" + timestamp); Serial.print("... ");
    fp = SD.open("hab/" + timestamp, FILE_WRITE);
    if (!fp) {
      Serial.println("FAILED");
      successful = false;
    } else {
      Serial.println("OK");
    }
  }

  // connect to camera @TODO

  Serial.print("Starting accelerometer... ");
  if (! accelerometer.begin()) {
    Serial.println("FAILED");
    successful = false;
  } else {
    Serial.println("OK");
  }

  Serial.print("Starting barometer... ");
  if (! baro.begin()) {
    Serial.println("FAILED");
    successful = false;
  }
  Serial.println("OK");

  // set up radio object  @TODO

  if (successful) {
    // @TODO change this?
    // or change based on altitude and expected changes?
    accelerometer.setRange(MMA8451_RANGE_2_G);
    Serial.print("Accelerometer range = "); Serial.print(2 << accelerometer.getRange());
    Serial.println("G");
    Serial.println("GPS time determined. It is "); Serial.println(timestamp);
  } else {
    Serial.println("\nErrors were detected. HAB not starting.");
    while (1);
  }
}

void loop() {
  readData();
  saveDataToSD();
  if (shouldTakePicture() == true) {
    takePicture();
  }
  // format for APRS @TODO
  // send data @TODO
  // sleep?
  delay(1000);
}

void readData() {
  /* GPS data */
  // get GPS position @TODO
  // get latitude @TODO
  // get longitude @TODO
  // get height? @TODO
  // get other GPS stuff? @TODO

  /* External temperature */
  // @TODO

  /* Accelerometer data */
  accelerometer.read();
  curr_tilt[0] = accelerometer.x;
  curr_tilt[1] = accelerometer.y;
  curr_tilt[2] = accelerometer.z;

  // in m/s^2
  sensors_event_t event;
  accelerometer.getEvent(&event);
  curr_accel[0] = event.acceleration.x;
  curr_accel[1] = event.acceleration.y;
  curr_accel[2] = event.acceleration.z;

  /* Barometer data */
  // in kilopascals (kpa)
  // Use http://www.onlineconversion.com/pressure.htm for other units
  curr_pressure = baro.getPressure();

  // in meters
  curr_altitude = baro.getAltitude();
  curr_internal_temp = baro.getTemperature();
}

/*
   0  gps: datetime
   1  gps: latitude
   2  gps: longitude
   3  gps: speed
   4  gps: course
   5  gps: altitude
   6  gps: satellites
   7  gps: HDOP

*/
String formatDataForSD() {
  String parsed;
  if (gps.date.isValid() && gps.time.isValid()) {
    formatDateTime(true);
  }
  parsed += ",";
  if (gps.location.isValid()) {
    parsed += String(gps.location.rawLat().deg);
    parsed += ".";
    parsed += String(gps.location.rawLat().billionths);
    parsed += ",";
    parsed += String(gps.location.rawLng().deg);
    parsed += ".";
    parsed += String(gps.location.rawLng().billionths);
    parsed += ",";
  } else {
    parsed += ",,";
  }
  /*
     gps speed
     gps course
     gps alt
     gps sats
     gps hdop

  */
  return parsed += "\n";
}

/*
   In case you'd like to stub out the saving.
*/
void saveDataToSD() {
  fp.write(formatDataForSD().c_str());
}

bool shouldTakePicture() {
  return false;

  // @TODO
  // track last picture time and if it's been over X seconds take another
  // x should change based on altitude in a bell curve pattern.
  // above a certain height, x should be 0.
}

void takePicture() {
  // @TODO
}

String lpad(int number, byte width) {
  String formatted;
  int currentMax = 10;
  for (byte i = 1; i < width; i++) {
    if (number < currentMax) {
      formatted += "0";
    }
    currentMax *= 10;
  }
  formatted += number;
  return formatted;
}

String formatDateTime(bool separated) {
  String formatted;
  if (gps.date.isValid()) {
    formatted = String(gps.date.year());
    if (separated) formatted += "-";
    formatted += String(lpad(gps.date.month(), 2));
    if (separated) formatted += "-";
    formatted += String(lpad(gps.date.day(), 2));
  }
  if (gps.time.isValid()) {
    if (separated) formatted += " ";
    formatted += String(gps.time.hour());
    if (separated) formatted += ":";
    formatted += String(gps.time.minute());
    if (separated) formatted += ":";
    formatted += String(gps.time.second());
    if (separated) formatted += ".";
    formatted += String(gps.time.centisecond());
  }
  return formatted;
}
