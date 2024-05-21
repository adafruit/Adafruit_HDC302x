#ifndef ADAFRUIT_HDC302X_H
#define ADAFRUIT_HDC302X_H

#include "Arduino.h"
#include <Adafruit_BusIO_Register.h>
#include <Adafruit_I2CDevice.h>

enum HDC302x_Commands {
  SOFT_RESET = 0x30A2,
  READ_MANUFACTURER_ID = 0x3781,
  READ_NIST_ID_SERIAL_BYTES_5_4 =
      0x3683, // Command to read bytes 5 and 4 of the NIST ID
  READ_NIST_ID_SERIAL_BYTES_3_2 =
      0x3684, // Command to read bytes 3 and 2 of the NIST ID
  READ_NIST_ID_SERIAL_BYTES_1_0 =
      0x3685,                    // Command to read bytes 1 and 0 of the NIST ID
  ACCESS_OFFSETS = 0xA004,       // Command to write/read offsets
  SET_HEATER_POWER = 0x306E,     // Command to set heater power
  ENABLE_HEATER = 0x306D,        // Command to enable heater
  DISABLE_HEATER = 0x3066,       // Command to disable heater
  READ_STATUS_REGISTER = 0xF32D, // Command to read the status register
  CLEAR_STATUS_REGISTER = 0x3041, // Command to clear the status register
  MEASUREMENT_READOUT_AUTO_MODE = 0xE000, // Measurement Readout Auto Mode
  SET_LOW_ALERT = 0x6100,  // Configure ALERT Thresholds for Set Low Alert
  SET_HIGH_ALERT = 0x611D, // Configure ALERT Thresholds for Set High Alert
  CLR_LOW_ALERT = 0x610B,  // Configure ALERT Thresholds for Clear Low Alert
  CLR_HIGH_ALERT = 0x6116  // Configure ALERT Thresholds for Clear High Alert
};

typedef enum {
  AUTO_MEASUREMENT_0_5MPS_LP0 = 0x2032,
  AUTO_MEASUREMENT_0_5MPS_LP1 = 0x2024,
  AUTO_MEASUREMENT_0_5MPS_LP2 = 0x202F,
  AUTO_MEASUREMENT_0_5MPS_LP3 = 0x20FF,
  AUTO_MEASUREMENT_1MPS_LP0 = 0x2130,
  AUTO_MEASUREMENT_1MPS_LP1 = 0x2126,
  AUTO_MEASUREMENT_1MPS_LP2 = 0x212D,
  AUTO_MEASUREMENT_1MPS_LP3 = 0x21FF,
  AUTO_MEASUREMENT_2MPS_LP0 = 0x2236,
  AUTO_MEASUREMENT_2MPS_LP1 = 0x2220,
  AUTO_MEASUREMENT_2MPS_LP2 = 0x222B,
  AUTO_MEASUREMENT_2MPS_LP3 = 0x22FF,
  AUTO_MEASUREMENT_4MPS_LP0 = 0x2334,
  AUTO_MEASUREMENT_4MPS_LP1 = 0x2322,
  AUTO_MEASUREMENT_4MPS_LP2 = 0x2329,
  AUTO_MEASUREMENT_4MPS_LP3 = 0x23FF,
  AUTO_MEASUREMENT_10MPS_LP0 = 0x2737,
  AUTO_MEASUREMENT_10MPS_LP1 = 0x2721,
  AUTO_MEASUREMENT_10MPS_LP2 = 0x272A,
  AUTO_MEASUREMENT_10MPS_LP3 = 0x27FF,
  EXIT_AUTO_MODE = 0x3093
} hdcAutoMode_t;

typedef enum {
  TRIGGERMODE_LP0 = 0x2400, // Trigger-On Demand Mode, Low Power Mode 0
  TRIGGERMODE_LP1 = 0x240B, // Trigger-On Demand Mode, Low Power Mode 1
  TRIGGERMODE_LP2 = 0x2416, // Trigger-On Demand Mode, Low Power Mode 2
  TRIGGERMODE_LP3 = 0x24FF  // Trigger-On Demand Mode, Low Power Mode 3
} hdcTriggerMode_t;

typedef enum {
  HEATER_OFF = 0x0000,
  HEATER_QUARTER_POWER = 0x009F,
  HEATER_HALF_POWER = 0x03FF,
  HEATER_FULL_POWER = 0x3FFF
} HDC302x_HeaterPower;

/**!
   Functions and data for interfacing HDC302x
*/
class Adafruit_HDC302x {
public:
  Adafruit_HDC302x();
  bool begin(uint8_t i2cAddr = 0x44, TwoWire *wire = &Wire);
  bool reset();
  uint16_t readStatus();
  bool clearStatusRegister();

  uint16_t readManufacturerID();
  bool readNISTID(uint8_t nist[6]);

  bool heaterEnable(HDC302x_HeaterPower power);
  bool isHeaterOn();

  bool writeOffsets(double T, double RH);
  bool readOffsets(double &T, double &RH);

  void setAutoMode(hdcAutoMode_t mode);
  hdcAutoMode_t getAutoMode() const;
  bool readAutoTempRH(double &temp, double &RH);
  bool readTemperatureHumidityOnDemand(double &temp, double &RH,
                                       hdcTriggerMode_t mode);

  bool setHighAlert(float T, float RH);
  bool setLowAlert(float T, float RH);
  bool clearHighAlert(float T, float RH);
  bool clearLowAlert(float T, float RH);

  uint8_t calculateCRC8(const uint8_t *data, int len);

private:
  uint8_t calculateOffset(double f, bool isTemp);
  double invertOffset(uint8_t offset, bool isTemp);
  bool alertCommand(uint16_t cmd, float T, float RH);

  Adafruit_I2CDevice *i2c_dev = nullptr;
  bool writeCommand(uint16_t command);
  bool writeCommandData(uint16_t cmd, uint16_t data);
  bool writeCommandReadData(uint16_t command, uint16_t &data);
  bool sendCommandReadTRH(uint16_t command, double &temp, double &RH);
  hdcAutoMode_t currentAutoMode;
};

#endif // ADAFRUIT_HDC302X_H
