#ifndef _I2C_H
#define _I2C_H
#ifdef __cplusplus
extern "C" {
#endif

/// Initializes I2C0 on LPC Hardware.
/// @param i2cFrequency: Frequency in Hz from 0-1000000 or 0-1Mhz
void initialize_I2C0(unsigned int i2cFrequency);

/// Reads a single byte from an I2C Slave using POLLING
/// @param deviceAddress	The I2C Device Address
/// @param registerAddress	The register address to read
/// @return The byte read from slave device (might be 0 if error)
char i2cReadDeviceRegister(char deviceAddress, char registerAddress);

/// Writes a single byte to an I2C Slave using POLLING
/// @param deviceAddress	The I2C Device Address
/// @param registerAddress	The register address to read
/// @param value			The value to write to registerAddress
/// @return 0 if successful, or one of I2C States as error condition
char i2cWriteDeviceRegister(char deviceAddress, char registerAddress, char value);

#ifdef __cplusplus
}
#endif
#endif
