#include <Adafruit_HDC302x.h>

Adafruit_HDC302x hdc = Adafruit_HDC302x();

void setup() {
  Serial.begin(115200);

  Serial.println("Adafruit HDC302x test!");

  if (! hdc.begin(0x44, &Wire)) {
    Serial.println("Could not find sensor?");
    while (1);
  }
  displayNISTID();

  // Turn the heater on at quarter power
  if (hdc.heaterEnable(HEATER_QUARTER_POWER)) {
    Serial.println("Heater set to quarter power.");
  } else {
    Serial.println("Failed to set heater power.");
    while (1);
  }

  delay(1000);
    
  if (hdc.isHeaterOn()) {   // Check if the heater is on
    Serial.println("Heater is on.");
  } else {
    Serial.println("Heater should be on but isn't?");
    while (1);
  }

  if (hdc.heaterEnable(HEATER_OFF)) {  // Turn the heater off
    Serial.println("Heater turned off.");
  } else {
    Serial.println("Failed to turn heater off.");
    while (1);
  }

  if (!hdc.isHeaterOn()) {  // Verify the heater is off
      Serial.println("Heater is confirmed off.");
  } else {
      Serial.println("Heater is still on.");
      while (1);
  }

/*
  // Write offsets +8.20%RH and -7.17°C
  double targetRH = 8.20;
  double targetT = -7.17;

  if (! hdc.writeOffsets(targetT, targetRH)) {
  Serial.println("Failed to write offsets.");
  while (1);
  }
  Serial.println("Offsets written successfully.");
*/

  hdc.writeOffsets(0, 0);

  // Read back the offsets
  double readRH = 0.0;
  double readT = 0.0;

  if (!hdc.readOffsets(readT, readRH)) {
     Serial.println("Failed to read offsets.");
     while(1);
  }
  Serial.print("Read RH Offset: ");
  Serial.print(readRH);
  Serial.println("%");

  Serial.print("Read T Offset: ");
  Serial.print(readT);
  Serial.println("°C");

  // Set the auto measurement mode
  hdc.setAutoMode(AUTO_MEASUREMENT_1MPS_LP0);

  // Get and print the current auto mode, note this is cached
  hdcAutoMode_t currentMode = hdc.getAutoMode();

  Serial.print(F("Current Auto Measure Mode: "));
  switch (currentMode) {
    case AUTO_MEASUREMENT_0_5MPS_LP0:
        Serial.println(F("0.5 measurements/second at Low Power 0 (lowest noise)"));
        break;
    case AUTO_MEASUREMENT_0_5MPS_LP1:
        Serial.println(F("0.5 measurements/second at Low Power 1"));
        break;
    case AUTO_MEASUREMENT_0_5MPS_LP2:
        Serial.println(F("0.5 measurements/second at Low Power 2"));
        break;
    case AUTO_MEASUREMENT_0_5MPS_LP3:
        Serial.println(F("0.5 measurements/second at Low Power 3 (lowest power)"));
        break;
    case AUTO_MEASUREMENT_1MPS_LP0:
        Serial.println(F("1 measurement/second at Low Power 0 (lowest noise)"));
        break;
    case AUTO_MEASUREMENT_1MPS_LP1:
        Serial.println(F("1 measurement/second at Low Power 1"));
        break;
    case AUTO_MEASUREMENT_1MPS_LP2:
        Serial.println(F("1 measurement/second at Low Power 2"));
        break;
    case AUTO_MEASUREMENT_1MPS_LP3:
        Serial.println(F("1 measurement/second at Low Power 3 (lowest power)"));
        break;
    case AUTO_MEASUREMENT_2MPS_LP0:
        Serial.println(F("2 measurements/second at Low Power 0 (lowest noise)"));
        break;
    case AUTO_MEASUREMENT_2MPS_LP1:
        Serial.println(F("2 measurements/second at Low Power 1"));
        break;
    case AUTO_MEASUREMENT_2MPS_LP2:
        Serial.println(F("2 measurements/second at Low Power 2"));
        break;
    case AUTO_MEASUREMENT_2MPS_LP3:
        Serial.println(F("2 measurements/second at Low Power 3 (lowest power)"));
        break;
    case AUTO_MEASUREMENT_4MPS_LP0:
        Serial.println(F("4 measurements/second at Low Power 0 (lowest noise)"));
        break;
    case AUTO_MEASUREMENT_4MPS_LP1:
        Serial.println(F("4 measurements/second at Low Power 1"));
        break;
    case AUTO_MEASUREMENT_4MPS_LP2:
        Serial.println(F("4 measurements/second at Low Power 2"));
        break;
    case AUTO_MEASUREMENT_4MPS_LP3:
        Serial.println(F("4 measurements/second at Low Power 3 (lowest power)"));
        break;
    case AUTO_MEASUREMENT_10MPS_LP0:
        Serial.println(F("10 measurements/second at Low Power 0 (lowest noise)"));
        break;
    case AUTO_MEASUREMENT_10MPS_LP1:
        Serial.println(F("10 measurements/second at Low Power 1"));
        break;
    case AUTO_MEASUREMENT_10MPS_LP2:
        Serial.println(F("10 measurements/second at Low Power 2"));
        break;
    case AUTO_MEASUREMENT_10MPS_LP3:
        Serial.println(F("10 measurements/second at Low Power 3 (lowest power)"));
        break;
    case EXIT_AUTO_MODE:
        Serial.println(F("Exited Auto Measurement Mode"));
        break;
    default:
        Serial.println(F("Unknown Mode"));
        break;
  }

  // hdc.setAutoMode(EXIT_AUTO_MODE);

  hdc.setHighAlert(60, 80);
  hdc.clearHighAlert(58, 79);
  hdc.setLowAlert(-10, 20);
  hdc.clearLowAlert(-9, 22);
}

void loop() {
  prettyPrintStatusRegister(hdc.readStatus());
  
  double temp = 0.0;
  double RH = 0.0;

  // perform a 'forced' reading every second if not in automatic mode
  if (hdc.getAutoMode() == EXIT_AUTO_MODE) {    
    if (! hdc.readTemperatureHumidityOnDemand(temp, RH, TRIGGERMODE_LP0)) {
      Serial.println("Failed to read temperature and humidity.");
      return;
    }
    delay(1000);
  } else {
     if (! hdc.readAutoTempRH(temp, RH)) {
      // Data not ready for reading, we'll silently retry later
      return;
     }
  }
  
  Serial.print("Temperature: ");
  Serial.print(temp);
  Serial.println(" °C");

  Serial.print("Humidity: ");
  Serial.print(RH);
  Serial.println(" %");
}

void displayNISTID() {
  uint8_t nistID[6];
    
  if (! hdc.readNISTID(nistID)) {
  Serial.println("Failed to read NIST ID.");
  return;
  }
  Serial.print("NIST ID: ");
  for (int i = 0; i < 6; i++) {
    if (nistID[i] < 0x10) {
      Serial.print("0"); // Print leading zero
    }
    Serial.print(nistID[i], HEX);
    if (i < 5) Serial.print(":"); // Print colon between bytes for readability
  }
  Serial.println();
}

void prettyPrintStatusRegister(uint16_t status) {
    Serial.print(F("Status Register: 0x")); Serial.println(status, HEX);
    if (status & 0x8000) {
        Serial.println(F("  Overall Alert Status: At least one active alert"));
    }
    if (status & 0x2000) {
        Serial.println(F("  Heater Status Enabled"));
    }
    if (status & 0x0800) {
        Serial.println(F("  RH Tracking Alert"));
    }
    if (status & 0x0400) {
        Serial.println(F("  T Tracking Alert"));
    }
    if (status & 0x0200) {
        Serial.println(F("  RH High Tracking Alert"));
    }
    if (status & 0x0100) {
        Serial.println(F("  RH Low Tracking Alert"));
    }
    if (status & 0x0080) {
        Serial.println(F("  T High Tracking Alert"));
    }
    if (status & 0x0040) {
        Serial.println(F("  T Low Tracking Alert"));
    }
    if (status & 0x0010) {
        Serial.println(F("  Device Reset Detected"));
    }
    if (status & 0x0001) {
        Serial.println(F("  Checksum Verification Fail (incorrect checksum received)"));
    }
}
