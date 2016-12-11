/*
*
* CREATE - Functions to create and display the thermal frameBuffer
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

/* Methods*/

/* Filter a 160x120 smallBuffer with 3x3 gaussian kernel */
void gaussianFilter() {
	byte gaussianKernel[3][3] = {
		{ 1, 2, 1 },
		{ 2, 4, 2 },
		{ 1, 2, 1 }
	};
	long sum;

	for (int y = 1; y < 119; y++) {
		for (int x = 1; x < 159; x++) {
			sum = 0;
			for (int k = -1; k <= 1; k++) {
				for (int j = -1; j <= 1; j++) {
					sum += gaussianKernel[j + 1][k + 1] * smallBuffer[(y - j) * 160 + (x - k)];
				}
			}
			smallBuffer[(y * 160) + x] = (unsigned short)(sum / 16.0);
		}
	}
}

/* Filter a 160x120 smallBuffer with a 3x3 box filter */
void boxFilter() {
	long sum;

	for (int y = 1; y < 119; y++) {
		for (int x = 1; x < 159; x++) {
			sum = 0;
			for (int k = -1; k <= 1; k++) {
				for (int j = -1; j <= 1; j++) {
					sum += smallBuffer[(y - j) * 160 + (x - k)];
				}
			}
			smallBuffer[(y * 160) + x] = (unsigned short)(sum / 9.0);
		}
	}
}

//Resize the pixels of thermal smallBuffer
void resizePixels(unsigned short* pixels, int w1, int h1, int w2, int h2) {
	//Calculate the ratio
	int x_ratio = (int)((w1 << 16) / w2) + 1;
	int y_ratio = (int)((h1 << 16) / h2) + 1;
	int x2, y2;
	for (int i = 0; i < h2; i++) {
		for (int j = 0; j < w2; j++) {
			x2 = ((j * x_ratio) >> 16);
			y2 = ((i * y_ratio) >> 16);
			pixels[(i * w1) + j] = pixels[(y2 * w1) + x2];
		}
	}
	//Set the other pixels to zero
	for (int j = 0; j < h2; j++) {
		for (int i = w2; i < w1; i++) {
			pixels[i + (j * w1)] = 65535;
		}
	}
	for (int j = h2; j < h1; j++) {
		for (int i = 0; i < w1; i++) {
			pixels[i + (j * w1)] = 65535;
		}
	}
}

/* Resize the thermal smallBuffer */
void resizeImage() {
	uint16_t  xmax, ymax;
	unsigned short* pixelBuffer;

	//For Teensy 3.1/3.2
	if (teensyVersion == teensyVersion_old)
	{
		xmax = 160;
		ymax = 120;
		pixelBuffer = smallBuffer;
	}
	//Teensy 3.6
	else
	{
		xmax = 320;
		ymax = 240;
		pixelBuffer = bigBuffer;
	}

	uint16_t newWidth = round(adjCombFactor * xmax);
	uint16_t newHeight = round(adjCombFactor * ymax);

	//Resize pixels
	resizePixels(pixelBuffer, xmax, ymax, newWidth, newHeight);

	//Move the smallBuffer right
	uint16_t rightMove = round((xmax - newWidth) / 2.0);
	for (int i = 0; i < rightMove; i++) {
		//First for all pixels
		for (int col = (xmax - 1); col > 0; col--)
			for (int row = 0; row < ymax; row++)
				pixelBuffer[col + (row * xmax)] = pixelBuffer[col - 1 + (row * xmax)];
		//And then fill the last one with white
		for (int row = 0; row < ymax; row++)
			pixelBuffer[row * xmax] = 65535;
	}
	//Move the smallBuffer down
	uint16_t downMove = round((ymax - newHeight) / 2.0);
	for (int i = 0; i < downMove; i++) {
		//First for all pixels
		for (int col = 0; col < xmax; col++)
			for (int row = (ymax - 1); row > 0; row--)
				pixelBuffer[col + (row * xmax)] = pixelBuffer[col + ((row - 1) * xmax)];
		//And then fill the last one with white
		for (int col = 0; col < xmax; col++)
			pixelBuffer[col] = 65535;
	}
}

/* Calculates the fill pixel for visual/combined */
void calcFillPixel(uint16_t x, uint16_t y) {
	uint16_t pixel;
	byte red, green, blue;

	//Combined - set to darker thermal
	if (displayMode == displayMode_combined) {
		//Get the thermal image color
		pixel = smallBuffer[x + (y * 160)];
		//And extract the RGB values out of it
		byte redT = (pixel & 0xF800) >> 8;
		byte greenT = (pixel & 0x7E0) >> 3;
		byte blueT = (pixel & 0x1F) << 3;
		//Mix both
		red = (byte)redT * (1 - adjCombAlpha) + 127 * adjCombAlpha;
		green = (byte)greenT * (1 - adjCombAlpha) + 127 * adjCombAlpha;
		blue = (byte)blueT * (1 - adjCombAlpha) + 127 * adjCombAlpha;
	}
	//Visual - set to black
	else {
		red = 0;
		green = 0;
		blue = 0;
	}

	//Set image to that calculated RGB565 value
	pixel = (((red & 248) | green >> 5) << 8)
		| ((green & 28) << 3 | blue >> 3);
	//Save
	smallBuffer[x + (y * 160)] = pixel;
}

/* Fill out the edges in combined or visual mode */
void fillEdges() {
	//Fill the edges
	uint16_t  x, y;

	//Top & Bottom edges
	for (x = 0; x < 160; x++) {
		//Top edge
		for (y = 0; y < (5 * adjCombDown); y++) {
			calcFillPixel(x, y);
		}

		//Bottom edge
		for (y = 119; y > (119 - (5 * adjCombUp)); y--) {
			calcFillPixel(x, y);
		}
	}

	//Left & right edges
	for (y = 5 * adjCombDown; y < (120 - (5 * adjCombUp)); y++) {
		//Left edge
		for (x = 0; x < (5 * adjCombRight); x++) {
			calcFillPixel(x, y);
		}

		//Right edge
		for (x = 159; x > (159 - (5 * adjCombLeft)); x--) {
			calcFillPixel(x, y);
		}
	}
}


/* Write the smallBuffer to the bigBuffer by resizing, eventually add transparency */
void smallToBigBuffer(bool trans)
{
	unsigned short A, B, C, D, outVal;
	int x, y, index;
	float x_ratio = 159.0 / 320;
	float y_ratio = 119.0 / 240;
	float x_diff, y_diff;
	//For transparency
	byte redV, greenV, blueV, redT, greenT, blueT, red, green, blue;
	uint16_t value;
	float scale = (colorElements - 1.0) / (maxValue - minValue);

	int offset = 0;
	for (int i = 0; i < 240; i++) {
		for (int j = 0; j < 320; j++) {

			x = (int)(x_ratio * j);
			y = (int)(y_ratio * i);
			x_diff = (x_ratio * j) - x;
			y_diff = (y_ratio * i) - y;
			index = y * 160 + x;

			A = smallBuffer[index];
			B = smallBuffer[index + 1];
			C = smallBuffer[index + 160];
			D = smallBuffer[index + 160 + 1];

			outVal = (unsigned short)(
				A*(1 - x_diff)*(1 - y_diff) + B*(x_diff)*(1 - y_diff) +
				C*(y_diff)*(1 - x_diff) + D*(x_diff*y_diff)
				);

			//No alpha transparency, just write into the framebuffer
			if (trans == false)
				bigBuffer[offset] = outVal;

			//We want to create a combined bigBuffer
			else
			{
				//Limit values
				if (outVal > maxValue)
					outVal = maxValue;
				if (outVal < minValue)
					outVal = minValue;

				//Calc color position
				value = (outVal - minValue) * scale;

				//Calc RGB color values for thermal smallBuffer
				redT = colorMap[3 * value];
				greenT = colorMap[3 * value + 1];
				blueT = colorMap[3 * value + 2];

				//Get the RGB565 color of the visual smallBuffer
				value = bigBuffer[offset];

				//And extract the RGB values out of it
				redV = (value & 0xF800) >> 8;
				greenV = (value & 0x7E0) >> 3;
				blueV = (value & 0x1F) << 3;

				//Mix both
				red = redT * (1 - adjCombAlpha) + redV * adjCombAlpha;
				green = greenT * (1 - adjCombAlpha) + greenV * adjCombAlpha;
				blue = blueT * (1 - adjCombAlpha) + blueV * adjCombAlpha;

				//Convert to RGB565
				bigBuffer[offset] = (((red & 248) | green >> 5) << 8) | ((green & 28) << 3 | blue >> 3);
			}

			//Raise counter
			offset++;
		}
	}
}

/* Store one package of 80 columns into RAM */
boolean savePackage(byte line, byte segment = 0) {
	//Go through the video pixels for one video line
	for (int column = 0; column < 80; column++) {
		//Make a 16-bit rawvalue from the lepton frame
		uint16_t result = (uint16_t)(leptonFrame[2 * column + 4] << 8
			| leptonFrame[2 * column + 5]);

		//Invalid value, return
		if (result == 0) {
			return 0;
		}

		//Lepton2
		if (leptonVersion != leptonVersion_3_shutter) {
			//Rotated or old hardware version
			if (((mlx90614Version == mlx90614Version_old) && (rotationEnabled == 0)) ||
				((mlx90614Version == mlx90614Version_new) && (rotationEnabled == 1))) {
				smallBuffer[(line * 2 * 160) + (column * 2)] = result;
				smallBuffer[(line * 2 * 160) + (column * 2) + 1] = result;
				smallBuffer[(line * 2 * 160) + 160 + (column * 2)] = result;
				smallBuffer[(line * 2 * 160) + 160 + (column * 2) + 1] = result;
			}
			//Non-rotated
			else {
				smallBuffer[19199 - ((line * 2 * 160) + (column * 2))] = result;
				smallBuffer[19199 - ((line * 2 * 160) + (column * 2) + 1)] = result;
				smallBuffer[19199 - ((line * 2 * 160) + 160 + (column * 2))] = result;
				smallBuffer[19199 - ((line * 2 * 160) + 160 + (column * 2) + 1)] = result;
			}
		}
		//Lepton3
		else {
			//Non-rotated
			if (!rotationEnabled) {
				switch (segment) {
				case 1:
					smallBuffer[19199 - (((line / 2) * 160) + ((line % 2) * 80) + (column))] = result;
					break;
				case 2:
					smallBuffer[14399 - (((line / 2) * 160) + ((line % 2) * 80) + (column))] = result;
					break;
				case 3:
					smallBuffer[9599 - (((line / 2) * 160) + ((line % 2) * 80) + (column))] = result;
					break;
				case 4:
					smallBuffer[4799 - (((line / 2) * 160) + ((line % 2) * 80) + (column))] = result;
					break;
				}
			}
			//Rotated
			else {
				switch (segment) {
				case 1:
					smallBuffer[((line / 2) * 160) + ((line % 2) * 80) + (column)] = result;
					break;
				case 2:
					smallBuffer[4800 + (((line / 2) * 160) + ((line % 2) * 80) + (column))] = result;
					break;
				case 3:
					smallBuffer[9600 + (((line / 2) * 160) + ((line % 2) * 80) + (column))] = result;
					break;
				case 4:
					smallBuffer[14400 + (((line / 2) * 160) + ((line % 2) * 80) + (column))] = result;
					break;
				}
			}
		}
	}

	//Everything worked
	return 1;
}

/* Get one smallBuffer from the Lepton module */
void getTemperatures() {
	byte leptonError, segmentNumbers, line;

	//For Lepton2 sensor, get only one segment per frame
	if (leptonVersion != leptonVersion_3_shutter)
		segmentNumbers = 1;
	//For Lepton3 sensor, get four packages per frame
	else
		segmentNumbers = 4;
	//Begin SPI transmission
	lepton_begin();
	//Go through the segments
	for (byte segment = 1; segment <= segmentNumbers; segment++) {
		leptonError = 0;
		do {
			for (line = 0; line < 60; line++) {
				//If line matches expectation
				if (lepton_read(line, segment) == NONE) {
					if (!savePackage(line, segment)) {
						//Stabilize framerate
						delayMicroseconds(800);
						//Raise lepton error
						leptonError++;
						break;
					}
				}
				//Reset if the expected line does not match the answer
				else {
					if (leptonError == 255) {
						//If show menu was entered
						if (showMenu) {
							lepton_end();
							return;
						}
						//Reset segment
						segment = 1;
						//Reset lepton error
						leptonError = 0;
						//Reset Lepton SPI
						lepton_reset();
						break;
					}

					//Stabilize framerate
					delayMicroseconds(800);
					//Raise lepton error
					leptonError++;
					break;
				}
			}
		} while (line != 60);
	}
	//End Lepton SPI
	lepton_end();
}

/* Clears the show temperatures array */
void clearTemperatures() {
	//Go through the array
	for (byte i = 0; i < 96; i++) {
		//Set the index to 65535
		tempPoints[i][0] = 65535;
		//Set the value to zero
		tempPoints[i][1] = 0;
	}
}

/* Shows the temperatures over the smallBuffer on the screen */
void showTemperatures() {
	uint16_t xpos, ypos;

	//Go through the array
	for (byte i = 0; i < 96; i++) {
		//Get index
		uint16_t index = tempPoints[i][0];

		//Check if the tempPoint is active
		if (index != 65535) {
			//Calculate x and y position
			xpos = (index % 160) * 2;
			ypos = (index / 160) * 2;

			//Display the point
			display_print((char*) ".", xpos, ypos);
			display_print((char*) ".", xpos + 1, ypos);
			display_print((char*) ".", xpos, ypos + 1);
			display_print((char*) ".", xpos + 1, ypos + 1);

			//Calc x position for the text
			if (xpos < 20)
				xpos = 0;
			else
				xpos -= 20;

			//Calc y position for the text
			ypos += 15;
			if (ypos > 239)
				ypos = 239;

			//Display the absolute temperature
			display_printNumF(calFunction(tempPoints[i][1]), 2, xpos, ypos);
		}
	}
}

/* Read the x and y coordinates when touch screen is pressed for Add Point */
void getTouchPos(uint16_t* x, uint16_t* y) {
	int iter = 0;
	TS_Point point;
	unsigned long tx = 0;
	unsigned long ty = 0;
	//Wait for touch press
	while (!touch_touched());
	//While touch pressed, iterate over readings
	while (touch_touched() == true) {
		point = touch_getPoint();
		if ((point.x >= 0) && (point.x <= 320) && (point.y >= 0)
			&& (point.y <= 240)) {
			tx += point.x;
			ty += point.y;
			iter++;
		}
	}
	*x = tx / iter;
	*y = ty / iter;
}

/* Function to add or remove a measurement point */
void tempPointFunction(bool remove) {
	uint16_t xpos, ypos;
	byte pos = 0;
	bool removed = false;

	//If remove points, check if there are some first
	if (remove) {
		//Go through the array
		for (byte i = 0; i < 96; i++) {
			if (tempPoints[i][0] != 65535)
			{
				removed = true;
				break;
			}
		}
		//No points available to remove
		if (!removed) {
			showFullMessage((char*) "No points available!", true);
			delay(1000);
			return;
		}
	}
	//If add points, check if we have space left
	else
	{
		//Go through the array
		byte i;
		for (i = 0; i < 96; i++) {
			if (tempPoints[i][0] == 65535)
			{
				pos = i;
				break;
			}
		}
		//Maximum number of points added
		if (i == 96) {
			showFullMessage((char*) "Remove a point first!", true);
			delay(1000);
			return;
		}
	}

redraw:
	//Safe delay
	delay(10);

	//Disable show menu for get temp
	showMenu = false;

	//Create thermal smallBuffer
	if (displayMode == displayMode_thermal)
		createThermalImg();
	//Create visual or combined smallBuffer
	else
		createVisCombImg();

	//Re-enable show menu
	showMenu = true;

	//Show it on the screen
	displayBuffer();

	//Set text color, font and background
	changeTextColor();
	display_setBackColor(VGA_TRANSPARENT);
	display_setFont(smallFont);
	//Show current temperature points
	showTemperatures();
	//Display title
	display_setFont(bigFont);
	display_print((char*) "Select position", CENTER, 210);

	//Get touched coordinates
	getTouchPos(&xpos, &ypos);

	//Divide through 2 to match array size
	xpos /= 2;
	ypos /= 2;

	//Remove point
	if (remove) {
		//Set marker to false
		removed = false;

		//Check for 10 pixels around the touch position
		for (uint16_t x = xpos - 10; x <= xpos + 10; x++) {
			for (uint16_t y = ypos - 10; y <= ypos + 10; y++) {
				//Calculate index number
				uint16_t index = x + (y * 160);
				//If index is valid
				if ((index >= 0) && (index < 19200)) {
					//Check for all 96 points
					for (byte i = 0; i < 96; i++)
					{
						//We found a valid temperature point
						if (tempPoints[i][0] == index)
						{
							//Set to invalid
							tempPoints[i][0] = 65535;
							//Reset value
							tempPoints[i][1] = 0;
							//Set markter to true
							removed = true;
						}
					}
				}
			}
		}
		//Show border
		drawMainMenuBorder();
		//Show removed message
		if (removed)
			showFullMessage((char*) "Point removed!", true);
		//Invalid position, redraw
		else {
			showFullMessage((char*) "Invalid position!", true);
			delay(1000);
			goto redraw;
		}
	}

	//Add point
	else {
		//Add index
		tempPoints[pos][0] = xpos + (ypos * 160);
		//Set raw value to zero
		tempPoints[pos][1] = 0;
		//Show border
		drawMainMenuBorder();
		//Show message
		showFullMessage((char*) "Point added!", true);
	}

	//Wait some time
	delay(1000);
}

/* Go through the array of temperatures and find min and max temp */
void limitValues() {
	maxValue = 0;
	minValue = 65535;
	uint16_t temp;
	for (int i = 0; i < 19200; i++) {
		//Get value
		temp = smallBuffer[i];
		//Find maximum temp
		if (temp > maxValue)
			maxValue = temp;
		//Find minimum temp
		if (temp < minValue)
			minValue = temp;
	}
}

/* Get the colors for hot / cold mode selection */
void getHotColdColors(byte* red, byte* green, byte* blue) {
	switch (hotColdColor) {
		//White
	case hotColdColor_white:
		*red = 255;
		*green = 255;
		*blue = 255;
		break;
		//Black
	case hotColdColor_black:
		*red = 0;
		*green = 0;
		*blue = 0;
		break;
		//White
	case hotColdColor_red:
		*red = 255;
		*green = 0;
		*blue = 0;
		break;
		//White
	case hotColdColor_green:
		*red = 0;
		*green = 255;
		*blue = 0;
		break;
		//White
	case hotColdColor_blue:
		*red = 0;
		*green = 0;
		*blue = 255;
		break;
	}
}

/* Convert the lepton values to RGB colors */
void convertColors(bool small) {
	uint8_t red, green, blue;
	uint16_t value;

	//Calculate the scale
	float scale = (colorElements - 1.0) / (maxValue - minValue);

	//For hot and cold mode, calculate rawlevel
	float hotColdRawLevel = 0.0;
	if ((hotColdMode != hotColdMode_disabled) && (displayMode != displayMode_combined))
		hotColdRawLevel = tempToRaw(hotColdLevel);

	//Size of the array & buffer
	int size;
	unsigned short* frameBuffer;
	//Teensy 3.6, not for preview
	if ((teensyVersion == teensyVersion_new) && !small) {
		size = 76800;
		frameBuffer = bigBuffer;
	}
	//Teensy 3.1 / 3.2 or preview
	else {
		size = 19200;
		frameBuffer = smallBuffer;
	}

	//Repeat for 160x120 data
	for (int i = 0; i < size; i++) {

		value = frameBuffer[i];

		//Limit values
		if (value > maxValue)
			value = maxValue;
		if (value < minValue)
			value = minValue;

		//Hot
		if ((hotColdMode == hotColdMode_hot) && (value >= hotColdRawLevel) && (calStatus != cal_warmup) && (displayMode != displayMode_combined))
			getHotColdColors(&red, &green, &blue);
		//Cold
		else if ((hotColdMode == hotColdMode_cold) && (value <= hotColdRawLevel) && (calStatus != cal_warmup) && (displayMode != displayMode_combined))
			getHotColdColors(&red, &green, &blue);
		//Apply colorscheme
		else {
			value = (value - minValue) * scale;
			red = colorMap[3 * value];
			green = colorMap[3 * value + 1];
			blue = colorMap[3 * value + 2];
		}

		//Convert to RGB565
		frameBuffer[i] = (((red & 248) | green >> 5) << 8) | ((green & 28) << 3 | blue >> 3);
	}
}

/* Find the position of the minimum and maximum value */
void findMinMaxPositions()
{
	uint16_t min = 65535;
	uint16_t max = 0;
	//Go through the smallBuffer
	for (int i = 0; i < 19200; i++)
	{
		if (smallBuffer[i] < min)
		{
			minTempPos = i;
			min = smallBuffer[i];
		}
		if (smallBuffer[i] > max)
		{
			maxTempPos = i;
			max = smallBuffer[i];
		}
	}
}

/* Refresh the temperature points*/
void refreshTempPoints() {
	//Go through the array
	for (byte i = 0; i < 20; i++) {
		//Get index
		uint16_t index = tempPoints[i][0];
		//Check if point is active
		if (index != 65535) {
			//Calculate x and y position
			uint16_t xpos = index % 160;
			uint16_t ypos = index / 160;
			//Update value
			tempPoints[i][1] = smallBuffer[xpos + (ypos * 160)];
		}
	}
}

/* Calculate the x and y position for min/max out of the pixel index */
void calculateMinMaxPoint(uint16_t* xpos, uint16_t* ypos, uint16_t pixelIndex) {
	//Get xpos and ypos
	*xpos = (pixelIndex % 160) * 2;
	*ypos = (pixelIndex / 160) * 2;

	//Limit position
	if (*ypos > 240)
		*ypos = 240;
	if (*xpos > 320)
		*xpos = 320;
}

/* Creates a thermal smallBuffer and stores it in the array */
void createThermalImg(bool small) {
	//Receive the temperatures over SPI
	getTemperatures();

	//Compensate calibration with object temp
	compensateCalib();

	//Refresh the temp points if required
	refreshTempPoints();

	//Find min / max position
	if (minMaxPoints != minMaxPoints_disabled)
		findMinMaxPositions();

	//Find min and max if not in manual mode and limits not locked
	if ((autoMode) && (!limitsLocked))
		limitValues();

	//If smallBuffer save, save the raw data
	if (imgSave == imgSave_create)
		saveRawData(true, saveFilename);

	//Apply low-pass filter
	if (filterType == filterType_box)
		boxFilter();
	else if (filterType == filterType_gaussian)
		gaussianFilter();

	//Teensy 3.6 - Resize to big buffer if not used for preview
	if ((teensyVersion == teensyVersion_new) && !small)
		smallToBigBuffer();

	//Convert lepton data to RGB565 colors
	convertColors(small);
}

/* Create the visual or combined smallBuffer display */
void createVisCombImg() {
	//Receive the temperatures over SPI
	getTemperatures();
	//Compensate calibration with object temp
	compensateCalib();

	//Refresh the temp points if required
	refreshTempPoints();

	//Find min / max position
	if (minMaxPoints != minMaxPoints_disabled)
		findMinMaxPositions();

	//Find min and max if not in manual mode and limits not locked
	if ((autoMode) && (!limitsLocked))
		limitValues();

	//For Teensy 3.6, decompress visual image before thermal
	if (teensyVersion == teensyVersion_new)
	{
		//Fill array white
		for (int i = 0; i < 76800; i++)
			bigBuffer[i] = 65535;
		//Get image from cam
		camera_get(camera_stream);
		//Resize the image
		resizeImage();
	}

	//For combined only
	if (displayMode == displayMode_combined) {
		//Apply low-pass filter
		if (filterType == filterType_box)
			boxFilter();
		else if (filterType == filterType_gaussian)
			gaussianFilter();

		//Teensy 3.6 - Resize to big buffer and create transparency
		if (teensyVersion == teensyVersion_new)
			smallToBigBuffer(true);
		//Teensy 3.1 / 3.2 - Convert raw values to RGB565 
		else
			convertColors();
	}

	//For Teensy 3.1/3.2, decompress visual image after thermal image
	if (teensyVersion == teensyVersion_old)
	{
		//Resize the thermal image
		resizeImage();
		//Fill the edges of the thermal image
		fillEdges();
		//Get the visual image and decompress it combined
		camera_get(camera_stream);
	}

}