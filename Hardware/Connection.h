/*
*
* CONNECTION - Communication protocol for the USB serial data transmission
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

/* Defines */

//Start & Stop command
#define CMD_START              100
#define CMD_END	               200

//Serial terminal commands
#define CMD_GET_RAWLIMITS      110
#define CMD_GET_RAWDATA        111
#define CMD_GET_CONFIGDATA     112
#define CMD_GET_VISUALIMGLOW   113
#define CMD_GET_CALIBDATA      114
#define CMD_GET_SPOTTEMP       115
#define CMD_SET_TIME           116
#define CMD_GET_TEMPPOINTS     117
#define CMD_SET_LASER          118
#define CMD_GET_LASER          119
#define CMD_SET_SHUTTERRUN     120
#define CMD_SET_SHUTTERAUTO    121
#define CMD_SET_SHUTTERMANUAL  122
#define CMD_GET_SHUTTERSTATE   123
#define CMD_GET_BATTERYSTATUS  124
#define CMD_SET_CALSLOPE       125
#define CMD_SET_CALOFFSET      126
#define CMD_GET_MINMAXPOS      127 
#define CMD_GET_VISUALIMGHIGH  128
#define CMD_GET_FWVERSION      129
#define CMD_SET_LIMITSAUTO     130
#define CMD_SET_LIMITSMANUAL   131
#define CMD_SET_COLORSCHEME    132
#define CMD_SET_TEMPFORMAT     133
#define CMD_SET_SHOWSPOT       134
#define CMD_SET_SHOWCOLORBAR   135
#define CMD_SET_SHOWTEMPPOINTS 136
#define CMD_SET_TEMPPOINTS     137

//Serial frame commands
#define CMD_FRAME_RAW          150
#define CMD_FRAME_COLOR        151
//Types of frame responses
#define FRAME_CAPTURE          180
#define FRAME_STARTVID         181
#define FRAME_STOPVID          182
#define FRAME_NORMAL           183

/* Variables */

//Command, default send frame
byte sendCmd = FRAME_NORMAL;

/* Methods */

/* Get integer out of a text string */
int getInt(String text)
{
	char temp[6];
	text.toCharArray(temp, 5);
	int x = atoi(temp);
	return x;
}

/* Clear all buffers */
void serialClear() {
	Serial.clearReadError();
	Serial.clearWriteError();
	while (Serial.available() > 0) {
		Serial.read();
	}
}

/* Enter the serial connection mode if no display attached */
bool checkNoDisplay()
{
	//No connection to ILI9341, touch and SD -> go to USB serial
	if (!checkDiagnostic(diag_display) && !checkDiagnostic(diag_touch) && !checkDiagnostic(diag_sd))
		return true;
	//Display connected
	return false;
}

/* Send the lepton raw limits */
void sendRawLimits() {
	//Send min
	Serial.write((minValue & 0xFF00) >> 8);
	Serial.write(minValue & 0x00FF);
	//Send max
	Serial.write((maxValue & 0xFF00) >> 8);
	Serial.write(maxValue & 0x00FF);
}

/* Send the lepton raw data*/
void sendRawData(bool color = false) {
	uint16_t result;

	//For the Lepton2 sensor, write 4800 raw values
	if ((leptonVersion != leptonVersion_3_shutter) && (!color)) {
		for (int line = 0; line < 60; line++) {
			for (int column = 0; column < 80; column++) {
				result = smallBuffer[(line * 2 * 160) + (column * 2)];
				Serial.write((result & 0xFF00) >> 8);
				Serial.write(result & 0x00FF);
			}
		}
	}
	//For the Lepton3 sensor, write 19200 raw values
	else {
		for (int i = 0; i < 19200; i++) {
			Serial.write((smallBuffer[i] & 0xFF00) >> 8);
			Serial.write(smallBuffer[i] & 0x00FF);
		}
	}
}

/* Sends the configuration data */
void sendConfigData() {
	//Lepton version
	Serial.write(leptonVersion);
	//Rotation
	Serial.write(rotationEnabled);
	//Send color scheme
	Serial.write(colorScheme);
	//Send the temperature format
	Serial.write(tempFormat);
	//Send the show spot attribute
	Serial.write(spotEnabled);
	//Send the show colorbar attribute
	if (calStatus == cal_warmup)
		Serial.write((byte)0);
	else
		Serial.write(colorbarEnabled);
	//Send the temperature points enabled attribute
	if (calStatus == cal_warmup)
		Serial.write((byte)0);
	else
		Serial.write(pointsEnabled);
	//Send adjust limits allowed
	Serial.write((autoMode) && (!limitsLocked));
}

/* Sends the visual image as JPEG */
void sendVisualImg() {
	camera_capture();
	camera_get(camera_serial);
}

/* Sends the calibration data */
void sendCalibrationData() {
	uint8_t farray[4];
	//Send the calibration offset first
	floatToBytes(farray, (float)calOffset);
	for (int i = 0; i < 4; i++)
		Serial.write(farray[i]);
	//Send the calibration slope
	floatToBytes(farray, (float)calSlope);
	for (int i = 0; i < 4; i++)
		Serial.write(farray[i]);
}

/* Sends the spot temp*/
void sendSpotTemp() {
	uint8_t farray[4];
	floatToBytes(farray, mlx90614_temp);
	for (int i = 0; i < 4; i++)
		Serial.write(farray[i]);
}

/* Change the color scheme */
void changeColorScheme() {
	if (colorScheme < (colorSchemeTotal - 1))
		colorScheme++;
	else if (colorScheme == (colorSchemeTotal - 1))
		colorScheme = 0;
}

/* Sets the time */
void setTime() {
	//If no display connected, do not set time
	if (checkNoDisplay())
		return;

	//Wait for time string, maximum 1 second
	uint32_t timer = millis();
	while (!Serial.available() && ((millis() - timer) < 1000));
	//If there was no timestring
	if (Serial.available() == 0)
		return;
	//Read time
	String dateIn = Serial.readString();
	//Check if valid
	if (getInt(dateIn.substring(0, 4) >= 2016)) {
		//Set the clock
		setTime(getInt(dateIn.substring(11, 13)), getInt(dateIn.substring(14, 16)), getInt(dateIn.substring(17, 19)),
			getInt(dateIn.substring(8, 10)), getInt(dateIn.substring(5, 7)), getInt(dateIn.substring(0, 4)));
		//Set the RTC
		Teensy3Clock.set(now());
	}
}

/* Sets the calibration offset */
void setCalOffset() {
	uint8_t farray[4];
	//Read calibration offset
	for (int i = 0; i < 4; i++)
		farray[i] = Serial.read();
	calOffset = bytesToFloat(farray);
	//Store to EEPROM
	storeCalibration();
}

/* Sets the calibration slope */
void setCalSlope() {
	uint8_t farray[4];
	//Read calibration slope
	for (int i = 0; i < 4; i++)
		farray[i] = Serial.read();
	calSlope = bytesToFloat(farray);
	//Store to EEPROM
	storeCalibration();
}

/* Send the temperature points */
void sendTempPoints() {
	for (int i = 0; i < 192; i++) {
		Serial.write((showTemp[i] & 0xFF00) >> 8);
		Serial.write(showTemp[i] & 0x00FF);
	}
}

/* Send the laser state */
void sendLaserState() {
	Serial.write(laserEnabled);
}

/* Send the shutter mode */
void sendShutterMode() {
	Serial.write(leptonShutter);
}

/* Send the battery status in percentage */
void sendBatteryStatus() {
	Serial.write(batPercentage);
}

/* Sends the position of the min/max value */
void sendMinMaxPos() {
	uint16_t minXPos, minYPos, maxXPos, maxYPos;
	//Get raw temperatures
	getTemperatures();
	//Find min and max value
	findMinMaxPositions();
	//Calculate their position
	calculateMinMaxPoint(&minXPos, &minYPos, minTempPos);
	calculateMinMaxPoint(&maxXPos, &maxYPos, maxTempPos);
	//Minimum temp x position
	Serial.write((minXPos & 0xFF00) >> 8);
	Serial.write(minXPos & 0x00FF);
	//Minimum temp y position
	Serial.write((minYPos & 0xFF00) >> 8);
	Serial.write(minYPos & 0x00FF);
	//Maximum temp x position
	Serial.write((maxXPos & 0xFF00) >> 8);
	Serial.write(maxXPos & 0x00FF);
	//Maximum temp y position
	Serial.write((maxYPos & 0xFF00) >> 8);
	Serial.write(maxYPos & 0x00FF);
}

/* Send the current firmware version */
void sendFWVersion() {
	Serial.write(fwVersion);
}

/* Set the temperature limits to auto */
void setLimitsAuto()
{
	//Enable auto mode
	autoMode = true;
	//Disable limits locked
	limitsLocked = false;
	//Send ACK
	Serial.write(CMD_SET_LIMITSAUTO);
}

/* Set the temperature limits to manual */
void setLimitsManual()
{
	byte MSB, LSB;
	//Enable limits locked
	limitsLocked = true;
	//Receive minValue
	MSB = Serial.read();
	LSB = Serial.read();
	minValue = (((MSB) << 8) + LSB);
	//Receive maxValue
	MSB = Serial.read();
	LSB = Serial.read();
	maxValue = (((MSB) << 8) + LSB);
	//Send ACK
	Serial.write(CMD_SET_LIMITSMANUAL);
}

/* Set the color scheme */
void setColorScheme()
{
	//Read byte from serial port
	byte read = Serial.read();
	//Check if it has a valid number
	if ((read >= 0) && (read <= (colorSchemeTotal - 1)))
	{
		//Set color scheme to input
		colorScheme = read;
		//Select right color scheme
		selectColorScheme();
		//Save to EEPROM
		EEPROM.write(eeprom_colorScheme, colorScheme);
	}
	//Send ACK
	Serial.write(CMD_SET_COLORSCHEME);
}


/* Set the temperature format */
void setTempFormat()
{
	//Read byte from serial port
	byte read = Serial.read();
	//Check if it has a valid number
	if ((read >= 0) && (read <= 1))
	{
		//Set temperature format to input
		tempFormat = read;
		//Save to EEPROM
		EEPROM.write(eeprom_tempFormat, tempFormat);
	}
	//Send ACK
	Serial.write(CMD_SET_TEMPFORMAT);
}

/* Set the show spot information */
void setShowSpot()
{
	//Read byte from serial port
	byte read = Serial.read();
	//Check if it has a valid number
	if ((read >= 0) && (read <= 1))
	{
		//Set show spot to input
		spotEnabled = read;
		//Save to EEPROM
		EEPROM.write(eeprom_spotEnabled, spotEnabled);
	}
	//Send ACK
	Serial.write(CMD_SET_SHOWSPOT);
}

/* Set the show colorbar information */
void setShowColorbar()
{
	//Read byte from serial port
	byte read = Serial.read();
	//Check if it has a valid number
	if ((read >= 0) && (read <= 1))
	{
		//Set show colorbar to input
		colorbarEnabled = read;
		//Save to EEPROM
		EEPROM.write(eeprom_colorbarEnabled, colorbarEnabled);
	}
	//Send ACK
	Serial.write(CMD_SET_SHOWCOLORBAR);
}

/* Set the show temperature points information */
void setShowTempPoints()
{
	//Read byte from serial port
	byte read = Serial.read();
	//Check if it has a valid number
	if ((read >= 0) && (read <= 1))
	{
		//Set show temppoints to input
		pointsEnabled = read;
		//Save to EEPROM
		EEPROM.write(eeprom_pointsEnabled, pointsEnabled);
	}
	//Send ACK
	Serial.write(CMD_SET_SHOWTEMPPOINTS);
}

/* Set temperature points array */
void setTempPoints()
{
	byte MSB, LSB;
	//Go through the temp points array
	for (int i = 0; i < 192; i++) {
		//Read Min
		MSB = Serial.read();
		LSB = Serial.read();
		showTemp[i] = (((MSB) << 8) + LSB);
	}
	//Send ACK
	Serial.write(CMD_SET_TEMPPOINTS);
}

/* Sends a raw frame */
void sendFrame(bool color) {
	Serial.write(sendCmd);
	Serial.flush();
	//Send frame
	if (sendCmd == FRAME_NORMAL) {
		//Clear all serial buffers
		serialClear();
		//Convert to colors
		if (color) {
			//Apply low-pass filter
			if (filterType == filterType_box)
				boxFilter();
			else if (filterType == filterType_gaussian)
				gaussianFilter();
			//Convert to RGB565
			convertColors(true);
		}
		//Send raw data
		sendRawData(color);
		//Send limits
		sendRawLimits();
		//Send spot temp
		sendSpotTemp();
		//Send calibration data
		sendCalibrationData();
		//Send temperature points
		sendTempPoints();
	}
	//Switch back to send frame the next time
	else
		sendCmd = FRAME_NORMAL;
}

/* Evaluates commands from the serial port*/
bool serialHandler() {
	//Read command from Serial Port
	byte recCmd = Serial.read();

	//Decide what to do
	switch (recCmd) {
		//Send raw limits
	case CMD_GET_RAWLIMITS:
		sendRawLimits();
		break;
		//Send raw data
	case CMD_GET_RAWDATA:
		sendRawData();
		break;
		//Send config data
	case CMD_GET_CONFIGDATA:
		sendConfigData();
		break;
		//Send low visual imahe
	case CMD_GET_VISUALIMGLOW:
		//Only when camera is working
		if (checkDiagnostic(diag_camera)) {
			camera_changeRes(camera_resMiddle);
			sendVisualImg();
		}
		//Send false
		else
			Serial.write(0);
		break;
		//Send calibration data
	case CMD_GET_CALIBDATA:
		sendCalibrationData();
		break;
		//Send spot temp
	case CMD_GET_SPOTTEMP:
		sendSpotTemp();
		break;
		//Change time
	case CMD_SET_TIME:
		setTime();
		//Send ACK
		Serial.write(CMD_SET_TIME);
		break;
		//Send temperature points
	case CMD_GET_TEMPPOINTS:
		sendTempPoints();
		break;
		//Toggle laser
	case CMD_SET_LASER:
		toggleLaser();
		//Send ACK
		Serial.write(CMD_SET_LASER);
		break;
		//Send laser state
	case CMD_GET_LASER:
		sendLaserState();
		break;
		//Run the shutter
	case CMD_SET_SHUTTERRUN:
		lepton_ffc();
		//Send ACK
		Serial.write(CMD_SET_SHUTTERRUN);
		break;
		//Set shutter mode to manual
	case CMD_SET_SHUTTERMANUAL:
		lepton_ffcMode(false);
		//Send ACK
		Serial.write(CMD_SET_SHUTTERMANUAL);
		break;
		//Set shutter mode to auto
	case CMD_SET_SHUTTERAUTO:
		lepton_ffcMode(true);
		//Send ACK
		Serial.write(CMD_SET_SHUTTERAUTO);
		break;
		//Send the shutter mode
	case CMD_GET_SHUTTERSTATE:
		sendShutterMode();
		break;
		//Send battery status
	case CMD_GET_BATTERYSTATUS:
		sendBatteryStatus();
		break;
		//Set calibration offset
	case CMD_SET_CALOFFSET:
		setCalOffset();
		//Send ACK
		Serial.write(CMD_SET_CALOFFSET);
		break;
		//Set calibration slope
	case CMD_SET_CALSLOPE:
		setCalSlope();
		//Send ACK
		Serial.write(CMD_SET_CALSLOPE);
		break;
		//Send min/max position
	case CMD_GET_MINMAXPOS:
		sendMinMaxPos();
		break;
		//Send high visual image
	case CMD_GET_VISUALIMGHIGH:
		//Only when camera working
		if (checkDiagnostic(diag_camera)) {
			camera_changeRes(camera_resHigh);
			sendVisualImg();
		}
		//Otherwise send false
		else
			Serial.write(0);
		break;
		//Send firmware version
	case CMD_GET_FWVERSION:
		sendFWVersion();
		break;
		//Set limits to auto
	case CMD_SET_LIMITSAUTO:
		setLimitsAuto();
		break;
		//Set limits to manual
	case CMD_SET_LIMITSMANUAL:
		setLimitsManual();
		break;
		//Change colorscheme
	case CMD_SET_COLORSCHEME:
		setColorScheme();
		break;
		//Set temperature format
	case CMD_SET_TEMPFORMAT:
		setTempFormat();
		break;
		//Set show spot temp
	case CMD_SET_SHOWSPOT:
		setShowSpot();
		break;
		//Set show color bar
	case CMD_SET_SHOWCOLORBAR:
		setShowColorbar();
		break;
		//Set show temperature points
	case CMD_SET_SHOWTEMPPOINTS:
		setShowTempPoints();
		break;
		//Set temperature points
	case CMD_SET_TEMPPOINTS:
		setTempPoints();
		break;
		//Send raw frame
	case CMD_FRAME_RAW:
		sendFrame(false);
		break;
		//Send color frame
	case CMD_FRAME_COLOR:
		sendFrame(true);
		break;
		//End connection
	case CMD_END:
		return true;
		//Start connection, send ACK
	case CMD_START:
		Serial.write(CMD_START);
		break;
	}
	Serial.flush();
	return false;
}

/* Evaluate button presses */
void buttonHandler() {
	// Count the time to choose selection
	long startTime = millis();
	long endTime = millis() - startTime;
	while ((extButtonPressed()) && (endTime <= 1000))
		endTime = millis() - startTime;
	endTime = millis() - startTime;
	//Short press - request to save an image
	if ((endTime < 1000) && (sendCmd == FRAME_NORMAL)) {
		sendCmd = FRAME_CAPTURE;
	}
	//Long press - request to start or stop a video
	else {
		//Start video
		if ((videoSave == videoSave_disabled) && (sendCmd != FRAME_STOPVID)) {
			sendCmd = FRAME_STARTVID;
			videoSave = videoSave_recording;
			while (extButtonPressed());
		}
		//Stop video
		if ((videoSave == videoSave_recording) && (sendCmd != FRAME_STARTVID)) {
			sendCmd = FRAME_STOPVID;
			videoSave = videoSave_disabled;
			while (extButtonPressed());
		}
	}
}

/* Go into video output mode and wait for connected module */
void serialOutput() {
	//Send the frames
	while (true) {
		//Abort transmission when touched
		if (touch_touched() && checkDiagnostic(diag_touch))
			break;
		//Check warmup status
		checkWarmup();
		//Get the temps
		getTemperatures();
		//Compensate calibration with object temp
		compensateCalib();
		//Refresh the temp points if enabled
		if (pointsEnabled)
			refreshTempPoints();
		//Find min and max if not in manual mode and limits not locked
		if ((autoMode) && (!limitsLocked))
			limitValues();
		//Check button press if not in terminal mode
		if (extButtonPressed())
			buttonHandler();
		//Check for serial commands
		if (Serial.available() > 0) {
			//Check for exit
			if (serialHandler())
				break;
		}
	}
}

/* Method to init some basic values in case no display is used */
void serialInit()
{
	//Send error message if important components missing
	if (!checkDiagnostic(diag_spot))
		Serial.println("Spot sensor FAILED");
	//Check lepton config
	if (!checkDiagnostic(diag_lep_conf))
		Serial.println("Lepton I2C FAILED");
	//Check lepton data
	if (!checkDiagnostic(diag_lep_data))
		Serial.println("Lepton SPI FAILED");

	//Temperature format
	tempFormat = tempFormat_celcius;
	//Color scheme
	colorScheme = colorScheme_rainbow;
	colorMap = colorMap_rainbow;
	colorElements = 256;
	//Filter Type
	filterType = filterType_gaussian;
	//Display Mode
	displayMode = displayMode_thermal;
	//Hot / cold mode
	hotColdMode = hotColdMode_disabled;
	//Min/Max Points
	minMaxPoints = minMaxPoints_disabled;
	//Spot disable
	spotEnabled = false;
	//Points disable
	pointsEnabled = false;
	//Colorbar enable
	colorbarEnabled = true;
	//Calibration slope
	byte read = EEPROM.read(eeprom_calSlopeSet);
	if (read == eeprom_setValue)
		readCalibration();
	else
		calSlope = cal_stdSlope;
}

/* Tries to establish a connection to a thermal viewer or video output module*/
void serialConnect() {
	//Show message
	showFullMessage((char*)"Serial connection detected!");
	display_print((char*) "Touch screen to return", CENTER, 170);
	delay(1000);

	//Disable screen backlight
	disableScreenLight();

	//Turn laser off if enabled
	if (laserEnabled)
		toggleLaser();

	//Send ACK for Start
	Serial.write(CMD_START);

	//Go to the serial output
	serialOutput();

	//Send ACK for End
	Serial.write(CMD_END);

	//Re-Enable display backlight
	enableScreenLight();

	//Show message
	showFullMessage((char*)"Connection ended, return..");
	delay(1000);

	//Clear all serial buffers
	serialClear();

	//Change camera resolution back
	if (displayMode == displayMode_thermal)
		camera_changeRes(camera_resHigh);
	else
		camera_changeRes(camera_resLow);

	//Turn laser off if enabled
	if (laserEnabled)
		toggleLaser();

	//Switch back to auto shutter if manual used
	if (leptonShutter == leptonShutter_manual) {
		lepton_ffcMode(true);
		leptonShutter = leptonShutter_auto;
	}

	//Disable video mode
	videoSave = videoSave_disabled;
}