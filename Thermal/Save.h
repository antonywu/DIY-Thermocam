/*
*
* SAVE - Save images and videos to the internal storage
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

//160 x 120 bitmap header
const char bmp_header_small[66] = { 0x42, 0x4D, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x42, 0x00, 0x00, 0x00, 0x28, 0x00, 0x00,
0x00, 0xA0, 0x00, 0x00, 0x00, 0x88, 0xFF, 0xFF, 0xFF, 0x01,
0x00, 0x10, 0x00, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xF8, 0x00,
0x00, 0xE0, 0x07, 0x00, 0x00, 0x1F, 0x00, 0x00, 0x00 };

//320 x 240 bitmap header
const char bmp_header_middle[66] = { 0x42, 0x4D, 0x36, 0x58, 0x02, 0x00, 0x00,
0x00, 0x00, 0x00, 0x42, 0x00, 0x00, 0x00, 0x28, 0x00, 0x00,
0x00, 0x40, 0x01, 0x00, 0x00, 0xF0, 0x00, 0x00, 0x00, 0x01,
0x00, 0x10, 0x00, 0x03, 0x00, 0x00, 0x00, 0x00, 0x58, 0x02,
0x00, 0xC4, 0x0E, 0x00, 0x00, 0xC4, 0x0E, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xF8, 0x00,
0x00, 0xE0, 0x07, 0x00, 0x00, 0x1F, 0x00, 0x00, 0x00 };

//640 x 480 bitmap header
const char bmp_header_large[66] = { 0x42, 0x4D, 0x36, 0x60, 0x09, 0x00, 0x00,
0x00, 0x00, 0x00, 0x42, 0x00, 0x00, 0x00, 0x28, 0x00, 0x00, 0x00,
0x80, 0x02, 0x00, 0x00, 0xE0, 0x01, 0x00, 0x00, 0x01, 0x00, 0x10,
0x00, 0x03, 0x00, 0x00, 0x00, 0x00, 0x60, 0x09, 0x00, 0xC4, 0x0E,
0x00, 0x00, 0xC4, 0x0E, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0xF8, 0x00, 0x00, 0xE0, 0x07, 0x00, 0x00,
0x1F, 0x00, 0x00, 0x00 };

/* Methods */

/* Creates a filename from the current time & date */
void createSDName(char* filename, boolean folder) {
	char buffer[5];
	//Year
	itoa(year(), buffer, 10);
	strncpy(&filename[0], buffer, 4);
	//Month
	itoa(month(), buffer, 10);
	if (month() < 10) {
		filename[4] = '0';
		strncpy(&filename[5], buffer, 1);
	}
	else {
		strncpy(&filename[4], buffer, 2);
	}
	//Day
	itoa(day(), buffer, 10);
	if (day() < 10) {
		filename[6] = '0';
		strncpy(&filename[7], buffer, 1);
	}
	else {
		strncpy(&filename[6], buffer, 2);
	}
	//Hour
	itoa(hour(), buffer, 10);
	if (hour() < 10) {
		filename[8] = '0';
		strncpy(&filename[9], buffer, 1);
	}
	else {
		strncpy(&filename[8], buffer, 2);
	}
	//Minute
	itoa(minute(), buffer, 10);
	if (minute() < 10) {
		filename[10] = '0';
		strncpy(&filename[11], buffer, 1);
	}
	else {
		strncpy(&filename[10], buffer, 2);
	}
	//Second
	itoa(second(), buffer, 10);
	if (second() < 10) {
		filename[12] = '0';
		if (!folder)
			strncpy(&filename[13], buffer, 1);
		else
			strcpy(&filename[13], buffer);
	}
	else {
		if (!folder)
			strncpy(&filename[12], buffer, 2);
		else
			strcpy(&filename[12], buffer);
	}
}

/* Creates a bmp file for the thermal image */
void createBMPFile(char* filename) {
	//File extension and open
	strcpy(&filename[14], ".BMP");
	sdFile.open(filename, O_RDWR | O_CREAT | O_AT_END);
	//Write the BMP header
	sdFile.write((uint8_t*)bmp_header_large, 66);
}

/* Creates a jpg file for the visual image */
void createJPGFile(char* filename, char* dirname) {
	//Begin SD Transmission
	startAltClockline();
	//Go into the video folder if required
	if (dirname != NULL)
		sd.chdir(dirname);
	//File extension and open
	strcpy(&filename[14], ".JPG");
	sdFile.open(filename, O_RDWR | O_CREAT | O_AT_END);
	//Switch clockline
	endAltClockline();
}

/* Creates a folder for the video capture */
void createVideoFolder(char* dirname) {
	startAltClockline();
	//Build the dir from the current time & date
	createSDName(dirname, true);
	//Create folder
	sd.mkdir(dirname);
	//Go into that folder
	sd.chdir(dirname);
	endAltClockline();
}

/* Checks the requirements for image save */
void checkImageSave() {
	//ThermocamV4 or DIY-Thermocam V2 - check SD card
	if (!checkSDCard()) {
		showTransMessage((char*) "Waiting for card..");
		while (!checkSDCard());
	}

	//Check if there is at least 1MB of space left
	if (getSDSpace() < 1000) {
		showTransMessage((char*) "SD card full!");
		imgSave = imgSave_disabled;
		return;
	}

	//Set text color
	changeTextColor();
	//Set background transparent
	display_setBackColor(VGA_TRANSPARENT);
	//Display to screen in big font
	display_setFont(bigFont);

	//Capture visual image if enabled and saving
	if (visualEnabled && (displayMode == displayMode_thermal) && (checkDiagnostic(diag_camera)))
	{
		display_print((char*) "Hold cam steady..", CENTER, 70);
		delay(1000);
		camera_capture();
		display_print((char*) "Saving..", CENTER, 160);
	}

	//Show save message
	else if (spotEnabled)
		display_print((char*) "Saving..", CENTER, 70);
	else
		display_print((char*) "Saving..", CENTER, 110);

	//Build save filename from the current time & date
	createSDName(saveFilename);

	//Set marker to create image
	imgSave = imgSave_create;
	//Switch back to small font
	display_setFont(smallFont);
}

/* Creates the filename for the video frames */
void frameFilename(char* filename, uint16_t count) {
	filename[0] = '0' + count / 10000 % 10;
	filename[1] = '0' + count / 1000 % 10;
	filename[2] = '0' + count / 100 % 10;
	filename[3] = '0' + count / 10 % 10;
	filename[4] = '0' + count % 10;
}

/* Save video frame to image file */
void saveVideoFrame(char* filename, char* dirname) {
	//Begin SD Transmission
	startAltClockline();

	//Switch to video folder 
	sd.chdir(dirname);
	// Open the file for writing
	sdFile.open(filename, O_RDWR | O_CREAT | O_AT_END);

	//Write 160x120 BMP header
	sdFile.write((uint8_t*)bmp_header_small, 66);

	//Write 160x120 data
	for (int i = 0; i < 19200; i++) {
		sdFile.write(smallBuffer[i] & 0x00FF);
		sdFile.write((smallBuffer[i] & 0xFF00) >> 8);
	}

	//Close the file
	sdFile.close();
	//Switch Clock back to Standard
	endAltClockline();
}

/* Proccess video frames */
void processVideoFrames(int framesCaptured, char* dirname) {
	char buffer[30];
	char filename[] = "00000.DAT";
	int framesConverted = 0;

	//Display screen content
	display_fillScr(200, 200, 200);
	display_setBackColor(200, 200, 200);
	display_setFont(bigFont);
	display_setColor(VGA_BLUE);
	display_print((char*)"Video conversion", CENTER, 30);
	display_setFont(smallFont);
	display_setColor(VGA_BLACK);
	display_print((char*)"Converts all .DAT to .BMP frames", CENTER, 80);
	display_print((char*)"Press button to abort the process", CENTER, 120);
	sprintf(buffer, "Frames converted: %5d / %5d", framesConverted, framesCaptured);
	display_print(buffer, CENTER, 160);
	sprintf(buffer, "Folder name: %s", dirname);
	display_print(buffer, CENTER, 200);

	//Switch to processing mode
	videoSave = videoSave_processing;

	//Font color
	display_setBackColor(200, 200, 200);
	display_setFont(smallFont);
	display_setColor(VGA_BLACK);

	//Go through all the frames in the folder
	for (framesConverted = 0; framesConverted < framesCaptured; framesConverted++) {
		//Button pressed, exit
		if (videoSave != videoSave_processing)
			break;

		//Get filename
		frameFilename(filename, framesConverted);
		strcpy(&filename[5], ".DAT");
		//Load Raw data
		loadRawData(filename, dirname);

		//Apply low-pass filter
		if (filterType == filterType_box)
			boxFilter();
		else if (filterType == filterType_gaussian)
			gaussianFilter();

		//Find min / max position
		if (minMaxPoints != minMaxPoints_disabled)
			findMinMaxPositions();

		//Convert lepton data to RGB565 colors
		convertColors(true);

		//Save frame to image file
		strcpy(&filename[5], ".BMP");
		saveVideoFrame(filename, dirname);

		//Update screen content
		sprintf(buffer, "Frames converted: %5d / %5d", framesConverted + 1, framesCaptured);
		display_print(buffer, CENTER, 160);
	}

	//All images converted!
	showFullMessage((char*) "Video converted!");
	delay(1000);
}

/* Saves raw data for an image or an video frame */
void saveRawData(bool isImage, char* name, uint16_t framesCaptured) {
	uint16_t result;

	//Start SD
	startAltClockline(true);
	//Create filename for image
	if (isImage) {
		strcpy(&name[14], ".DAT");
		sdFile.open(name, O_RDWR | O_CREAT | O_AT_END);
	}
	//Create filename for video frame
	else {
		char filename[] = "00000.DAT";
		frameFilename(filename, framesCaptured);
		sd.chdir(name);
		sdFile.open(filename, O_RDWR | O_CREAT | O_AT_END);
	}

	//For the Lepton2 sensor, write 4800 raw values
	if (leptonVersion != leptonVersion_3_shutter) {
		for (int line = 0; line < 60; line++) {
			for (int column = 0; column < 80; column++) {
				result = smallBuffer[(line * 2 * 160) + (column * 2)];
				sdFile.write((result & 0xFF00) >> 8);
				sdFile.write(result & 0x00FF);
			}
		}
	}

	//For the Lepton3 sensor, write 19200 raw values
	else {
		for (int i = 0; i < 19200; i++) {
			sdFile.write((smallBuffer[i] & 0xFF00) >> 8);
			sdFile.write(smallBuffer[i] & 0x00FF);
		}
	}

	//Write min and max
	sdFile.write((minValue & 0xFF00) >> 8);
	sdFile.write(minValue & 0x00FF);
	sdFile.write((maxValue & 0xFF00) >> 8);
	sdFile.write(maxValue & 0x00FF);

	//Write the object temp 
	uint8_t farray[4];
	floatToBytes(farray, mlx90614_temp);
	for (int i = 0; i < 4; i++)
		sdFile.write(farray[i]);

	//Write the color scheme
	sdFile.write(colorScheme);
	//Write the temperature format
	sdFile.write(tempFormat);
	//Write the show spot attribute
	sdFile.write(spotEnabled);
	//Write the show colorbar attribute
	if (calStatus == cal_warmup)
		sdFile.write((byte)0);
	else
		sdFile.write(colorbarEnabled);
	//Write the show hottest / coldest attribute
	sdFile.write(minMaxPoints);

	//Write calibration offset
	floatToBytes(farray, (float)calOffset);
	for (int i = 0; i < 4; i++)
		sdFile.write(farray[i]);
	//Write calibration slope
	floatToBytes(farray, (float)calSlope);
	for (int i = 0; i < 4; i++)
		sdFile.write(farray[i]);

	//Write temperature points
	for (byte i = 0; i < 96; i++) {
		//Write index
		sdFile.write((tempPoints[i][0] & 0xFF00) >> 8);
		sdFile.write(tempPoints[i][0] & 0x00FF);
		//Write value
		sdFile.write((tempPoints[i][1] & 0xFF00) >> 8);
		sdFile.write(tempPoints[i][1] & 0x00FF);
	}

	//Close the file
	sdFile.close();
	//Switch Clock back to Standard
	endAltClockline();
}

/* Saves additional images to the internal storage */
void saveImages() {
	//Save Bitmap image if activated or in visual / combined mode
	if (convertEnabled || (displayMode == displayMode_visual) || (displayMode == displayMode_combined))
		saveBuffer(saveFilename);

	//Save visual image in thermal mode
	if (visualEnabled && (displayMode == displayMode_thermal) && (checkDiagnostic(diag_camera))) {
		//Create JPEG file
		createJPGFile(saveFilename);
		//Save visual image in full-res
		camera_get(camera_save);
	}

	//Refresh free space
	refreshFreeSpace();

	//Disable image save marker
	imgSave = imgSave_disabled;
}

/* Saves the content of the screen buffer to the sd card */
void saveBuffer(char* filename, char* dirname) {
	unsigned short pixel;
	//Begin SD Transmission
	startAltClockline(true);

	//Switch to video folder if video
	if (dirname != NULL)
		sd.chdir(dirname);

	//Create file
	createBMPFile(filename);

	//Teensy 3.1/3.2
	if (teensyVersion == teensyVersion_old)
	{
		//Allocate space for sd buffer
		uint8_t* sdBuffer = (uint8_t*)calloc(320, sizeof(uint8_t));
		//Save 640x480 pixels
		for (uint8_t y = 0; y < 120; y++) {
			//Write them into the sd buffer
			for (uint8_t x = 0; x < 160; x++) {
				pixel = smallBuffer[((119 - y) * 160) + x];
				sdBuffer[x * 2] = pixel & 0x00FF;
				sdBuffer[(x * 2) + 1] = (pixel & 0xFF00) >> 8;
			}
			//Write them to the sd card with 640x480 resolution
			for (uint8_t i = 0; i < 4; i++) {
				for (uint8_t x = 0; x < 160; x++) {
					for (uint8_t j = 0; j < 4; j++) {
						sdFile.write(sdBuffer[x * 2]);
						sdFile.write(sdBuffer[(x * 2) + 1]);
					}
				}
			}
		}
		//De-allocate space
		free(sdBuffer);
	}

	//Teensy 3.6
	else
	{
		//Allocate space for sd buffer
		uint8_t* sdBuffer = (uint8_t*)calloc(1280, sizeof(uint8_t));
		//Save 640x480 pixels
		for (byte y = 0; y < 240; y++) {
			//Write them into the sd buffer
			for (uint16_t x = 0; x < 320; x++) {
				pixel = bigBuffer[((239 - y) * 320) + x];
				sdBuffer[x * 4] = pixel & 0x00FF;
				sdBuffer[(x * 4) + 1] = (pixel & 0xFF00) >> 8;
				sdBuffer[(x * 4) + 2] = pixel & 0x00FF;
				sdBuffer[(x * 4) + 3] = (pixel & 0xFF00) >> 8;
			}
			//Write them to the sd card with 640x480 resolution
			sdFile.write(sdBuffer, 1280);
			sdFile.write(sdBuffer, 1280);
		}
		//De-allocate space
		free(sdBuffer);
	}

	//Close file
	sdFile.close();

	//End SD Transmission
	endAltClockline();
}