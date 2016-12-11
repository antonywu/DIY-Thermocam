/*
*
* LEPTON - Access the FLIR Lepton LWIR module
*
* DIY-Thermocam Firmware
*
* GNU General Public License v3.0
*
* Copyright by Max Ritter
*
* http://www.diy-thermocam.net
* https://github.com/maxritter/DIY-Thermocam
*
*/

/* Variables */
//Array to store one Lepton frame
byte leptonFrame[164];

//Lepton frame error return
enum LeptonReadError {
	NONE, DISCARD, SEGMENT_ERROR, ROW_ERROR, SEGMENT_INVALID
};

/* Methods */

/* Start Lepton SPI Transmission */
void lepton_begin() {
	//Start alternative clock line, except for old HW
	if (mlx90614Version == mlx90614Version_new)
		startAltClockline();
	//For Teensy  3.1 / 3.2 and Lepton3 use this one
	if ((teensyVersion == teensyVersion_old) && (leptonVersion == leptonVersion_3_shutter))
		SPI.beginTransaction(SPISettings(30000000, MSBFIRST, SPI_MODE0));
	//Otherwise use 20 Mhz maximum and SPI mode 1
	else
		SPI.beginTransaction(SPISettings(20000000, MSBFIRST, SPI_MODE1));
	//Start transfer  - CS LOW
	digitalWrite(pin_lepton_cs, LOW);
}

/* End Lepton SPI Transmission */
void lepton_end() {
	//End transfer - CS HIGH
	digitalWriteFast(pin_lepton_cs, HIGH);
	//End SPI Transaction
	SPI.endTransaction();
	//End alternative clock line, except for old HW
	if (mlx90614Version == mlx90614Version_new)
		endAltClockline();
}

/* Reset the SPI bus to re-initiate Lepton communication */
void lepton_reset()
{
	//End Lepton SPI
	lepton_end();
	//Short delay
	delay(186);
	//Begin Lepton SPI
	lepton_begin();
}

/* Reads one line (164 Bytes) from the lepton over SPI */
LeptonReadError lepton_read(byte line, byte seg) {
	//Receive one frame over SPI
	SPI.transfer(leptonFrame, 164);
	//Repeat as long as the frame is not valid, equals sync
	if ((leptonFrame[0] & 0x0F) == 0x0F) {
		return DISCARD;
	}
	//Check if the line number matches the expected line
	if (leptonFrame[1] != line) {
		return ROW_ERROR;
	}
	//For the Lepton3, check if the segment number matches
	if ((line == 20) && (leptonVersion == leptonVersion_3_shutter)) {
		byte segment = (leptonFrame[0] >> 4);
		if (segment == 0)
			return SEGMENT_INVALID;
		if (segment != seg)
			return SEGMENT_ERROR;
	}
	return NONE;
}

/* Trigger a flat-field-correction on the Lepton */
bool lepton_ffc() {
	Wire.beginTransmission(0x2A);
	Wire.write(0x00);
	Wire.write(0x04);
	Wire.write(0x02);
	Wire.write(0x42);
	byte error = Wire.endTransmission();
	return error;
}

/* Select I2C Register on the Lepton */
void lepton_setReg(byte reg) {
	Wire.beginTransmission(0x2A);
	Wire.write(reg >> 8 & 0xff);
	Wire.write(reg & 0xff);
	Wire.endTransmission();
}

/* Read I2C Register on the lepton */
int lepton_readReg(byte reg) {
	uint16_t reading;
	lepton_setReg(reg);
	Wire.requestFrom(0x2A, 2);
	reading = Wire.read();
	reading = reading << 8;
	reading |= Wire.read();
	return reading;
}

/* Set the shutter operation to manual/auto */
void lepton_ffcMode(bool automatic)
{
	//Array for the package
	byte package[32];
	//Read command
	Wire.beginTransmission(0x2A);
	Wire.write(0x00);
	Wire.write(0x04);
	Wire.write(0x02);
	Wire.write(0x3C);
	Wire.endTransmission();
	//Read old FFC package first
	Wire.beginTransmission(0x2A);
	while (lepton_readReg(0x2) & 0x01);
	uint8_t length = lepton_readReg(0x6);
	Wire.requestFrom((uint8_t)0x2A, length);
	//Read out the package
	for (byte i = 0; i < length; i++)
	{
		package[i] = Wire.read();
		Serial.write(package[i]);
	}
	Wire.endTransmission();

	//Alter the second bit
	if (automatic)
		package[1] = 0x01;
	else
		package[1] = 0x02;

	//Transmit the new package
	Wire.beginTransmission(0x2A);
	Wire.write(0x00);
	Wire.write(0x08);
	for (int i = 0; i < length; i++) {
		Wire.write(package[i]);
	}
	Wire.endTransmission();
	//Package length, use 32 here
	Wire.beginTransmission(0x2A);
	Wire.write(0x00);
	Wire.write(0x06);
	Wire.write(0x00);
	Wire.write(0x20);
	Wire.endTransmission();
	//Module and command ID
	Wire.beginTransmission(0x2A);
	Wire.write(0x00);
	Wire.write(0x04);
	Wire.write(0x02);
	Wire.write(0x3D);
	Wire.endTransmission();
}

/* Checks the Lepton hardware revision */
void lepton_version() {
	//Get AGC Command
	Wire.beginTransmission(0x2A);
	Wire.write(0x00);
	Wire.write(0x04);
	Wire.write(0x48);
	Wire.write(0x1C);
	byte error = Wire.endTransmission();
	//Lepton I2C error, set diagnostic
	if (error != 0) {
		setDiagnostic(diag_lep_conf);
		leptonVersion = leptonVersion_2_noShutter;
		return;
	}
	//Transfer the new package
	Wire.beginTransmission(0x2A);
	while (lepton_readReg(0x2) & 0x01);
	Wire.requestFrom(0x2A, lepton_readReg(0x6));
	char leptonhw[33];
	Wire.readBytes(leptonhw, 32);
	Wire.endTransmission();
	//Detected Lepton2 Shuttered
	if (strstr(leptonhw, "05-060") != NULL) {
		leptonVersion = leptonVersion_2_shutter;
	}
	//Detected Lepton3 Shuttered
	else if (strstr(leptonhw, "05-070") != NULL) {
		leptonVersion = leptonVersion_3_shutter;
	}
	//Detected Lepton2 No-Shutter
	else {
		leptonVersion = leptonVersion_2_noShutter;
	}
	//Write to EEPROM
	EEPROM.write(eeprom_leptonVersion, leptonVersion);
}

/* Set the shutter operation to manual or auto */
void lepton_shutterMode(bool automatic)
{
	//If there is no shutter, do not do anything
	if (leptonShutter == leptonShutter_none)
		return;

	//Set the shutter mode on the Lepton
	lepton_ffcMode(automatic);

	//Set shutter mode
	if (automatic)
		leptonShutter = leptonShutter_auto;
	else
		leptonShutter = leptonShutter_manual;
}

/* Set the radiometry mode (not used) */
void lepton_radiometry(bool enable)
{
	//Enable or disable radiometry
	Wire.beginTransmission(0x2A);
	Wire.write(0x00);
	Wire.write(0x08);
	Wire.write(0x00);
	Wire.write(enable);
	Wire.endTransmission();
	//Data length
	Wire.beginTransmission(0x2A);
	Wire.write(0x00);
	Wire.write(0x06);
	Wire.write(0x00);
	Wire.write(0x02);
	Wire.endTransmission();
	//RAD module with OEM bit and command
	Wire.beginTransmission(0x2A);
	Wire.write(0x00);
	Wire.write(0x04);
	Wire.write(0x4E);
	Wire.write(0x11);
	Wire.endTransmission();
}

/* Init the FLIR Lepton LWIR sensor */
void lepton_init() {
	//Check the Lepton HW Revision
	lepton_version();

	//Perform FFC if shutter is attached
	if (leptonVersion != leptonVersion_2_noShutter) {
		//Set shutter mode to auto
		leptonShutter = leptonShutter_auto;
		//Run the FFC and check return
		if (lepton_ffc())
			setDiagnostic(diag_lep_conf);
		//Wait some time
		delay(2000);
	}
	//No shutter attached
	else
		leptonShutter = leptonShutter_none;

	//Set the calibration timer
	calTimer = millis();
	//Set calibration status to warmup if not coming from mass storage
	calStatus = cal_warmup;
	//Set the compensation value to zero
	calComp = 0;

	//Activate auto mode
	autoMode = true;
	//Deactivate limits Locked
	limitsLocked = false;

	//Check if SPI works
	lepton_begin();
	do {
		digitalWriteFast(pin_lepton_cs, LOW);
		SPI.transfer(leptonFrame, 164);
		digitalWriteFast(pin_lepton_cs, HIGH);
	}
	//Repeat as long as the frame is not valid, equals sync
	while (((leptonFrame[0] & 0x0F) == 0x0F) && ((millis() - calTimer) < 1000));
	lepton_end();
	//If sync not received after a second, set diagnostic
	if ((leptonFrame[0] & 0x0F) == 0x0F)
		setDiagnostic(diag_lep_data);
}