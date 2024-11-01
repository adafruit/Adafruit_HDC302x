#include "Adafruit_HDC302x.h"

/**
 * Constructor for the HDC302x sensor driver.
 */
Adafruit_HDC302x::Adafruit_HDC302x() { currentAutoMode = EXIT_AUTO_MODE; }

/**
 * Initializes the HDC302x sensor.
 *
 * @param i2cAddr The I2C address of the HDC302x sensor.
 * @param wire The TwoWire object representing the I2C bus.
 * @return true if initialization was successful, otherwise false.
 */
bool Adafruit_HDC302x::begin(uint8_t i2cAddr, TwoWire *wire) {
  delay(5); // wait for device to be ready

  if (i2c_dev) {
    delete i2c_dev;
  }
  i2c_dev = new Adafruit_I2CDevice(i2cAddr, wire);

  if (!i2c_dev->begin()) {
    return false;
  }

  if (!reset()) {
    return false;
  }
  clearStatusRegister();

  uint16_t manufacturerID;
  if (!writeCommandReadData(HDC302x_Commands::READ_MANUFACTURER_ID,
                            manufacturerID) ||
      manufacturerID != 0x3000) {
    return false;
  }

  setAutoMode(EXIT_AUTO_MODE);

  return true;
}

/**
 * Sets the auto mode for measurements.
 *
 * @param mode The desired auto mode.
 */
void Adafruit_HDC302x::setAutoMode(hdcAutoMode_t mode) {
  currentAutoMode = mode;
  writeCommand(mode);
}

/**
 * Gets the current auto mode.
 *
 * @return The current auto mode.
 */
hdcAutoMode_t Adafruit_HDC302x::getAutoMode() const { return currentAutoMode; }

/**
 * Reads the temperature and humidity in auto mode.
 *
 * @param temp Reference to store the temperature value.
 * @param RH Reference to store the relative humidity value.
 * @return true if the data was successfully read and CRC checks passed,
 * otherwise false.
 */
bool Adafruit_HDC302x::readAutoTempRH(double &temp, double &RH) {
  return sendCommandReadTRH(MEASUREMENT_READOUT_AUTO_MODE, temp, RH);
}

/**
 * Reads the temperature and humidity on demand using the specified trigger
 * mode.
 *
 * @param temp Reference to store the temperature value.
 * @param RH Reference to store the relative humidity value.
 * @param mode The trigger mode to use for the measurement, defaults to
 * TRIGGERMODE_LP0 (lowest noise)
 * @return true if the data was successfully read and CRC checks passed,
 * otherwise false.
 */
bool Adafruit_HDC302x::readTemperatureHumidityOnDemand(
    double &temp, double &RH, hdcTriggerMode_t mode = TRIGGERMODE_LP0) {
  return sendCommandReadTRH(static_cast<uint16_t>(mode), temp, RH);
}

/**
 * Sends a command and reads the temperature and humidity.
 *
 * @param command The command to send.
 * @param temp Reference to store the temperature value.
 * @param RH Reference to store the relative humidity value.
 * @return true if the data was successfully read and CRC checks passed,
 * otherwise false.
 */
bool Adafruit_HDC302x::sendCommandReadTRH(uint16_t command, double &temp,
                                          double &RH) {
  // Trigger the temperature and humidity measurement
  if (!writeCommand(command)) {
    return false;
  }

  // Wait for conversion (tmeas in datasheet table 7.5)
  delay(20);

  // Read results
  uint8_t buffer[6];
  i2c_dev->read(buffer, 6);

  // Validate CRC for temperature data
  if (calculateCRC8(buffer, 2) != buffer[2]) {
    return false; // CRC check failed
  }

  // Validate CRC for humidity data
  if (calculateCRC8(buffer + 3, 2) != buffer[5]) {
    return false; // CRC check failed
  }

  uint16_t rawTemperature = (buffer[0] << 8) | buffer[1];
  uint16_t rawHumidity = (buffer[3] << 8) | buffer[4];

  // Convert raw temperature data to degrees Celsius
  temp = ((rawTemperature / 65535.0) * 175.0) - 45.0;

  // Convert raw humidity data to percentage
  RH = (rawHumidity / 65535.0) * 100.0;

  return true;
}

/**
 * Writes the temperature and relative humidity offsets to the sensor.
 *
 * @param T The temperature offset value.
 * @param RH The relative humidity offset value.
 * @return true if the command and data transmission were successful.
 */
bool Adafruit_HDC302x::writeOffsets(double T, double RH) {
  uint8_t RH_offset = calculateOffset(RH, false);
  uint8_t T_offset = calculateOffset(T, true);

  // Combine RH offset and T offset into one 2-byte data value
  uint16_t combined_offsets = (RH_offset << 8) | T_offset;

  // Write the combined offsets to the offset register
  return writeCommandData(HDC302x_Commands::ACCESS_OFFSETS, combined_offsets);
}

/**
 * Reads the temperature and relative humidity offsets from the sensor.
 *
 * @param T Reference to store the temperature offset value.
 * @param RH Reference to store the relative humidity offset value.
 * @return true if the command and data transmission were successful.
 */
bool Adafruit_HDC302x::readOffsets(double &T, double &RH) {
  uint16_t combined_offsets = 0;

  // Read the combined offsets from the offset register
  if (!writeCommandReadData(HDC302x_Commands::ACCESS_OFFSETS,
                            combined_offsets)) {
    return false;
  }

  // Extract the RH and T offsets from the combined value
  uint8_t RH_offset = (combined_offsets >> 8) & 0xFF;
  uint8_t T_offset = combined_offsets & 0xFF;

  // Convert the offsets to floating point values
  RH = invertOffset(RH_offset, false);
  T = invertOffset(T_offset, true);

  return true;
}

/**
 * Calculates the closest matching offset byte for temperature or relative
 * humidity.
 *
 * @param f The floating point offset value.
 * @param isTemp True if calculating temperature offset, false if calculating RH
 * offset.
 * @return The closest match offset byte.
 */
uint8_t Adafruit_HDC302x::calculateOffset(double f, bool isTemp) {
  // LSB values
  double lsb = isTemp ? 0.1708984375 : 0.1953125;

  // Calculate the absolute value and the sign bit
  uint8_t sign = (f < 0) ? 0x00 : 0x80;
  double abs_f = fabs(f);

  // Calculate the integer part of the offset
  uint8_t offset = static_cast<uint8_t>(round(abs_f / lsb));

  // Combine the sign bit and the offset value
  return sign | offset;
}

/**
 * Inverts the offset byte to a floating point value.
 *
 * @param offset The offset byte.
 * @param isTemp True if inverting temperature offset, false if inverting RH
 * offset.
 * @return The floating point offset value.
 */
double Adafruit_HDC302x::invertOffset(uint8_t offset, bool isTemp) {
  // LSB values
  double lsb = isTemp ? 0.1708984375 : 0.1953125;

  // Extract the sign bit
  bool isNegative = !(offset & 0x80);

  // Remove the sign bit from the offset
  uint8_t abs_offset = offset & 0x7F;

  // Calculate the floating point value
  double value = abs_offset * lsb;

  // Apply the sign
  return isNegative ? -value : value;
}

/**
 * Checks if the heater is currently enabled.
 *
 * @return true if the heater is on, otherwise false.
 */
bool Adafruit_HDC302x::isHeaterOn() {
  uint16_t status = readStatus();
  return (status & (1UL << 13));
}

/**
 * Enables or disables the heater.
 *
 * @param power The desired heater power setting.
 * @return true if the heater commands were successful, otherwise false.
 */
bool Adafruit_HDC302x::heaterEnable(HDC302x_HeaterPower power) {
  if (power == HEATER_OFF) {
    return writeCommand(HDC302x_Commands::DISABLE_HEATER);
  } else {
    if (!writeCommand(HDC302x_Commands::ENABLE_HEATER)) {
      return false;
    }
    return writeCommandData(HDC302x_Commands::SET_HEATER_POWER, power);
  }
}

/**
 * Sends an alert command with the specified temperature and humidity
 * thresholds.
 *
 * @param cmd The command to send.
 * @param T The temperature threshold value in degrees Celsius.
 * @param RH The relative humidity threshold value in percentage.
 * @return true if the command was successfully sent, otherwise false.
 */
bool Adafruit_HDC302x::alertCommand(uint16_t cmd, float T, float RH) {
  // Convert temperature and humidity to 16-bit binary values
  uint16_t rawTemp = static_cast<uint16_t>(((T + 45.0) / 175.0) * 65535.0);
  uint16_t rawRH = static_cast<uint16_t>((RH / 100.0) * 65535.0);

  // Retain the 7 MSBs for RH and the 9 MSBs for T
  uint16_t msbRH = (rawRH >> 9) & 0x7F;
  uint16_t msbT = (rawTemp >> 7) & 0x1FF;

  // Concatenate the 7 MSBs for RH with the 9 MSBs for T
  uint16_t threshold = (msbRH << 9) | msbT;

  // Use writeCommandData to send the command and data
  return writeCommandData(cmd, threshold);
}

/**
 * Sets the high alert thresholds for temperature and humidity.
 *
 * @param T The temperature threshold value in degrees Celsius.
 * @param RH The relative humidity threshold value in percentage.
 * @return true if the command was successfully sent, otherwise false.
 */
bool Adafruit_HDC302x::setHighAlert(float T, float RH) {
  return alertCommand(SET_HIGH_ALERT, T, RH);
}

/**
 * Sets the low alert thresholds for temperature and humidity.
 *
 * @param T The temperature threshold value in degrees Celsius.
 * @param RH The relative humidity threshold value in percentage.
 * @return true if the command was successfully sent, otherwise false.
 */
bool Adafruit_HDC302x::setLowAlert(float T, float RH) {
  return alertCommand(SET_LOW_ALERT, T, RH);
}

/**
 * Clears the high alert thresholds for temperature and humidity.
 *
 * @param T The temperature threshold value in degrees Celsius.
 * @param RH The relative humidity threshold value in percentage.
 * @return true if the command was successfully sent, otherwise false.
 */
bool Adafruit_HDC302x::clearHighAlert(float T, float RH) {
  return alertCommand(CLR_HIGH_ALERT, T, RH);
}

/**
 * Clears the low alert thresholds for temperature and humidity.
 *
 * @param T The temperature threshold value in degrees Celsius.
 * @param RH The relative humidity threshold value in percentage.
 * @return true if the command was successfully sent, otherwise false.
 */
bool Adafruit_HDC302x::clearLowAlert(float T, float RH) {
  return alertCommand(CLR_LOW_ALERT, T, RH);
}

/**
 * Reads the status register.
 *
 * @return The 16-bit value of the status register.
 */
uint16_t Adafruit_HDC302x::readStatus() {
  uint16_t status = 0;
  writeCommandReadData(HDC302x_Commands::READ_STATUS_REGISTER, status);
  return status;
}

/**
 * Clears the status register.
 *
 * @return true if the command was successfully sent, otherwise false.
 */
bool Adafruit_HDC302x::clearStatusRegister() {
  return writeCommand(HDC302x_Commands::CLEAR_STATUS_REGISTER);
}

/**
 * Reads the Manufacturer ID from the HDC302x sensor.
 *
 * @return Manufacturer ID if successful, 0 if failure.
 */
uint16_t Adafruit_HDC302x::readManufacturerID() {
  uint16_t manufacturerID = 0;
  if (!writeCommandReadData(HDC302x_Commands::READ_MANUFACTURER_ID,
                            manufacturerID)) {
    return 0; // Return 0 if there is an error
  }
  return manufacturerID;
}

/**
 * Reads the NIST ID from the HDC302x sensor.
 *
 * @param nist Array to store the 6-byte NIST ID.
 * @return true if all parts of the NIST ID were read successfully and CRC
 * checks were valid, otherwise false.
 */
bool Adafruit_HDC302x::readNISTID(uint8_t nist[6]) {
  uint16_t part1, part2, part3;
  if (!writeCommandReadData(HDC302x_Commands::READ_NIST_ID_SERIAL_BYTES_5_4,
                            part1) ||
      !writeCommandReadData(HDC302x_Commands::READ_NIST_ID_SERIAL_BYTES_3_2,
                            part2) ||
      !writeCommandReadData(HDC302x_Commands::READ_NIST_ID_SERIAL_BYTES_1_0,
                            part3)) {
    return false;
  }

  nist[0] = (uint8_t)(part1 >> 8);
  nist[1] = (uint8_t)(part1 & 0xFF);
  nist[2] = (uint8_t)(part2 >> 8);
  nist[3] = (uint8_t)(part2 & 0xFF);
  nist[4] = (uint8_t)(part3 >> 8);
  nist[5] = (uint8_t)(part3 & 0xFF);

  return true;
}

/**
 * Resets the HDC302x sensor.
 *
 * @return true if the reset command was sent successfully, otherwise false.
 */
bool Adafruit_HDC302x::reset() {
  return writeCommand(HDC302x_Commands::SOFT_RESET);
}

/********************************** LOW LEVEL COMMAND/DATA FUNCTIONS */

/**
 * Writes a command to the HDC302x sensor.
 *
 * @param command The command to write.
 * @return true if the command was written successfully, otherwise false.
 */
bool Adafruit_HDC302x::writeCommand(uint16_t command) {
  uint8_t buffer[2];
  buffer[0] = (uint8_t)(command >> 8);   // High byte
  buffer[1] = (uint8_t)(command & 0xFF); // Low byte

  return i2c_dev->write(buffer, 2);
}

/**
 * Writes a command to the HDC302x and reads back 16-bit data and an 8-bit CRC.
 *
 * @param command The command to send.
 * @param data Reference to store the 16-bit data read from the device.
 * @return true if the command and data transmission were successful and CRC
 * checks out.
 */
bool Adafruit_HDC302x::writeCommandReadData(uint16_t command, uint16_t &data) {
  uint8_t cmd_buffer[2];
  uint8_t data_buffer[3]; // Two bytes for data, one for CRC

  cmd_buffer[0] = (uint8_t)(command >> 8);   // High byte of the command
  cmd_buffer[1] = (uint8_t)(command & 0xFF); // Low byte of the command

  // Write the command and read the data + CRC
  if (!i2c_dev->write_then_read(cmd_buffer, 2, data_buffer, 3)) {
    return false; // Communication failed
  }

  // Calculate CRC
  uint8_t calculated_crc = calculateCRC8(data_buffer, 2);
  // Check if calculated CRC matches the received CRC
  if (calculated_crc != data_buffer[2]) {
    return false; // CRC mismatch
  }

  // CRC checks out, return the data
  data = (uint16_t)(data_buffer[0] << 8 | data_buffer[1]);
  return true;
}

/**
 * Writes a command and data to the HDC302x, followed by a CRC.
 *
 * @param cmd The command to send.
 * @param data The data to send.
 * @return true if the command and data transmission were successful.
 */
bool Adafruit_HDC302x::writeCommandData(uint16_t cmd, uint16_t data) {
  uint8_t buffer[5];
  buffer[0] = (uint8_t)(cmd >> 8);          // High byte of the command
  buffer[1] = (uint8_t)(cmd & 0xFF);        // Low byte of the command
  buffer[2] = (uint8_t)(data >> 8);         // High byte of the data
  buffer[3] = (uint8_t)(data & 0xFF);       // Low byte of the data
  buffer[4] = calculateCRC8(buffer + 2, 2); // Calculate CRC for the data

  return i2c_dev->write(buffer, 5);
}

/**
 * @brief Calculates the CRC-8 for the given data.
 *
 * This function calculates the CRC-8 for the given data array using the
 * polynomial 0x31.
 *
 * @param data Pointer to the data array.
 * @param len Length of the data array.
 * @return uint8_t The calculated CRC-8 value.
 */
uint8_t Adafruit_HDC302x::calculateCRC8(const uint8_t *data, int len) {
  uint8_t crc = 0xFF; // Typical initial value
  for (int i = 0; i < len; i++) {
    crc ^= data[i];               // XOR byte into least sig. byte of crc
    for (int j = 8; j > 0; j--) { // Loop over each bit
      if (crc & 0x80) {           // If the uppermost bit is 1...
        crc = (crc << 1) ^ 0x31;  // Polynomial used by HDC302x
      } else {
        crc = (crc << 1);
      }
    }
  }
  return crc; // Final XOR value can also be applied if specified by device
}
