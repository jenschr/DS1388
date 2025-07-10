/* Code written by Chia Jiun Wei @ 21 Mar 2017
 * <J.W.Chia@tudelft.nl>
 
 * DS1388: a library to provide high level APIs to interface with the 
 * Maxim Integrated Real-time Clock. It is possible to use this library 
 * in Energia (the Arduino port for MSP microcontrollers) or in other 
 * toolchains.
 
 * This file is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License 
 * version 3, both as published by the Free Software Foundation.
  
 */

 #include "DS1388.h"
 
/**  DS1388 class creator function
 *
 *   Parameters:
 *   DWire &i2c             I2C object
 *
 */
DS1388::DS1388()
{
    address = I2C_ADDRESS;
}

/**  Initialise the value of control register
 *   
 *   Control register is initialise to oscillator enable, watchdog counter disable, watchdog alarm disable
 *
 */
bool DS1388::begin()
{
	Wire.beginTransmission(address);
	if (Wire.endTransmission() == 0)
		return true;
	return writeRegister(CONTROL_REG, (EN_OSCILLATOR | DIS_WD_COUNTER));
}

static uint8_t bin2bcd(uint8_t val) {
	return val + 6 * (val / 10);
}
static uint8_t bcd2bin(uint8_t val) {
	return val - 6 * (val >> 4);
}

void DS1388::adjust(const DateTime &dt) {
	Wire.beginTransmission(address);
	Wire.write((uint8_t)0); // start at location 0
	Wire.write((uint8_t)0);
	Wire.write(bin2bcd(dt.second()));
	Wire.write(bin2bcd(dt.minute()));
	Wire.write(bin2bcd(dt.hour()));
	Wire.write(bin2bcd(0));
	Wire.write(bin2bcd(dt.day()));
	Wire.write(bin2bcd(dt.month()));
	Wire.write(bin2bcd(dt.year() - 2000U));
	Wire.endTransmission();
  }

/**  Get time
 *
 *	 Returns:
 *   DateTime			A DateTime object based on RTC settings
 */
DateTime DS1388::now()
{
	Wire.beginTransmission(address);
	Wire.write((byte)0);
	Wire.endTransmission();

	Wire.requestFrom(address, 8);
	_centisecond = bcd2bin(Wire.read()); // First byte is the hundredths of the second
	uint8_t ss = bcd2bin(Wire.read() & 0x7F);
	uint8_t mm = bcd2bin(Wire.read());
	uint8_t hh = bcd2bin(Wire.read());
	Wire.read();
	uint8_t d = bcd2bin(Wire.read());
	uint8_t m = bcd2bin(Wire.read());
	uint16_t y = bcd2bin(Wire.read()) + 2000U;

	return DateTime(y, m, d, hh, mm, ss);
}

uint8_t DS1388::centisecond()
{
	return _centisecond;
}

/**  Check the validity of the time (oscillator funtionality)
 *
 *	 Returns:
 *	 unsigned char		1: time invalid, oscillator stopped
 *						0: time valid
 */
unsigned char DS1388::oscillatorRunning()
{
	unsigned char ret;
	ret = readRegister(FLAG_REG);
	ret = (ret >> 7);
	return ret;
}

/**  Clear oscillator status flag
 *
 */
void DS1388::OSC_clear_flag()
{
	unsigned char reg_save;
	
	reg_save = readRegister(FLAG_REG);
	writeRegister(FLAG_REG, (reg_save & 0x40));
}

/**  Check watchdog status
 *
 *	 Returns:
 *	 unsigned char		1: watchdog counter reached zero, triggered reset, flag must be cleared
 *						0: watchdog in normal operation
 */
unsigned char DS1388::WD_status()
{
	unsigned char ret;
	ret = readRegister(FLAG_REG);
	ret = (ret >> 6) & 0x01;
	return ret;
}

/**  Clear watchdog status flag
 *
 */
void DS1388::WD_clear_flag()
{
	unsigned char reg_save;
	
	reg_save = readRegister(FLAG_REG);
	writeRegister(FLAG_REG, (reg_save & 0x80));
}

/**  Returns the value (1 byte) of the selected internal register
 *
 *   Parameters:
 *   unsigned char reg     register number
 *
 *   Returns:
 *   unsigned char         register value
 *
 */
unsigned char DS1388::readRegister(unsigned char reg)
{
    unsigned char ret = -1;
    Wire.beginTransmission(address);
    Wire.write(reg);

    unsigned char res = Wire.requestFrom(address, 1);
    if (res == 1)
    {
		ret = Wire.read();
    }

	int err = Wire.endTransmission();
	if( err != 0 )
	{
		Serial.print(address);
		Serial.print(" Error ");
		Serial.print(err);
		Serial.print(" reading from address ");
		Serial.println(reg);
	}
	return err == 0;

    return ret;
}


/**  Sets the value (1 byte) of the selected internal register
 *   
 *   Parameters:
 *   unsigned char reg     register number
 *   unsigned char val     register value
 *
 */
bool DS1388::writeRegister(unsigned char reg, unsigned char val)
{
    Wire.beginTransmission(address);
    Wire.write(reg);
    Wire.write(val & 0xFF);      
    int err = Wire.endTransmission();
	if( err != 0 )
	{
		Serial.print(address);
		Serial.print(" Error writing ");
		Serial.print(val);
		Serial.print(" to address ");
		Serial.println(reg);
	}
	return err == 0;
}