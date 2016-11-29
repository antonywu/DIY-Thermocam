/*
*
* LIVE MODE - GUI functions used in the live mode
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

/* Methods */

/* Display battery status in percentage */
void displayBatteryStatus() {
	//Check battery status every 60 seconds
	if (batTimer == 0)
		batTimer = millis();
	if ((millis() - batTimer) > 60000) {
		checkBattery();
		batTimer = millis();
	}
	//USB Power only
	if (batPercentage == -1)
		display_print((char*) "USB Power", 240, 0);
	//Low Battery
	else if (batPercentage == 0)
		display_print((char*) "LOW", 270, 0);
	//Display battery status in percentage
	else {
		//Charging, show plus symbol
		if(analogRead(pin_usb_measure) > 50)
		{
			display_printNumI(batPercentage, 270, 0, 3, ' ');
			display_print((char*) "%", 300, 0);
			display_print((char*) "+", 310, 0);
		}
		//Not charging
		else
		{
			display_printNumI(batPercentage, 280, 0, 3, ' ');
			display_print((char*) "%", 310, 0);
		}
	}
}

/* Display the current time on the screen*/
void displayTime() {
	//Tiny font
	if (teensyVersion == teensyVersion_old) {
		display_printNumI(hour(), 5, 228, 2, '0');
		display_print((char*) ":", 23, 228);
		display_printNumI(minute(), 27, 228, 2, '0');
		display_print((char*) ":", 45, 228);
		display_printNumI(second(), 49, 228, 2, '0');
	}
	//Small font
	else
	{
		display_printNumI(hour(), 5, 228, 2, '0');
		display_print((char*) ":", 20, 228);
		display_printNumI(minute(), 27, 228, 2, '0');
		display_print((char*) ":", 42, 228);
		display_printNumI(second(), 49, 228, 2, '0');
	}
}

/* Display the date on screen */
void displayDate() {
	//Tiny font
	if (teensyVersion == teensyVersion_old) {
		display_printNumI(day(), 5, 0, 2, '0');
		display_print((char*) ".", 23, 0);
		display_printNumI(month(), 27, 0, 2, '0');
		display_print((char*) ".", 45, 0);
		display_printNumI(year(), 49, 0, 4);
	}
	//Small font
	else
	{
		display_printNumI(day(), 5, 0, 2, '0');
		display_print((char*) ".", 20, 0);
		display_printNumI(month(), 27, 0, 2, '0');
		display_print((char*) ".", 42, 0);
		display_printNumI(year(), 49, 0, 4);
	}
}

/* Display the warmup message on screen*/
void displayWarmup() {
	char buffer[25];
	sprintf(buffer, "Sensor warmup, %2ds left", (int)abs(60 - ((millis() - calTimer) / 1000)));
	//Tinyfont
	if (teensyVersion == teensyVersion_old)
		display_print(buffer, 45, 200);
	//Smallfont
	else
		display_print(buffer, 65, 200);
}

/* Display the minimum and maximum point on the screen */
void displayMinMaxPoint(uint16_t pixelIndex, const char *str)
{
	uint16_t xpos, ypos;
	//Calculate x and y position
	calculateMinMaxPoint(&xpos, &ypos, pixelIndex);
	//Draw the marker
	display_drawLine(xpos, ypos, xpos, ypos);
	//Draw the string
	xpos += 4;
	if (xpos >= 310)
		xpos -= 10;
	if (ypos > 230)
		ypos = 230;
	display_print(str, xpos, ypos);
}

/* Display free space on screen*/
void displayFreeSpace() {
	//Tinyfont
	if (teensyVersion == teensyVersion_old)
		display_print(sdInfo, 197, 228);
	//Smallfont
	else
		display_print(sdInfo, 220, 228);
}

/* Show the current spot temperature on screen*/
void showSpot() {
	//Draw the spot circle
	display_drawCircle(160, 120, 12);
	//Draw the lines
	display_drawLine(136, 120, 148, 120);
	display_drawLine(172, 120, 184, 120);
	display_drawLine(160, 96, 160, 108);
	display_drawLine(160, 132, 160, 144);
	//Convert to float with a special method
	char buffer[10];
	floatToChar(buffer, mlx90614_temp);
	display_print(buffer, 145, 150);
}

/* Display addition information on the screen */
void displayInfos() {
	//Set text color
	setTextColor();
	//Set font and background
	display_setBackColor(VGA_TRANSPARENT);
	//For Teensy 3.6, set small font
	if (teensyVersion == teensyVersion_new)
		display_setFont(smallFont);
	//For Teensy 3.1 / 3.2, set tiny font
	else
		display_setFont(tinyFont);

	//Set write to image, not display
	display_writeToImage = true;
	//Check warmup
	checkWarmup();

	//If  not saving image or video
	if ((imgSave != imgSave_create) && (!videoSave)) {
		//Show battery status in percantage
		if (batteryEnabled)
			displayBatteryStatus();
		//Show the time
		if (timeEnabled)
			displayTime();
		//Show the date
		if (dateEnabled)
			displayDate();
		//Show storage information
		if (storageEnabled)
			displayFreeSpace();
		//Display warmup if required
		if ((!videoSave) && (calStatus == cal_warmup))
			displayWarmup();
		//Show the minimum / maximum points
		if (minMaxPoints & minMaxPoints_min)
			displayMinMaxPoint(minTempPos, (const char *)"C");
		if (minMaxPoints & minMaxPoints_max)
			displayMinMaxPoint(maxTempPos, (const char *)"H");
	}

	//Show the spot in the middle
	if (spotEnabled)
		showSpot();
	//Show the color bar when warmup is over and if enabled, not in visual mode
	if ((colorbarEnabled) && (calStatus != cal_warmup) && (displayMode != displayMode_visual))
		showColorBar();
	//Show the temperature points
	if (pointsEnabled)
		showTemperatures();

	//Set write back to display
	display_writeToImage = false;
}