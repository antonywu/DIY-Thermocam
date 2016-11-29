/*
*
* MAIN SKETCH - Main entry point for the firmware
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

/* Current firmware version */

#define Version "Firmware 2.30 from 29.11.2016"
#define fwVersion 230

/* External Libraries */

#include "Libraries/SPI/SPI.h"
#include "Libraries/I2C/i2c_t3.h"
#include "Libraries/EEPROM/EEPROM.h"
#include "Libraries/ADC/ADC.h"
#include "Libraries/Metro/Metro.h"
#include "Libraries/Time/Time.h"
#include "Libraries/Bounce/Bounce.h"
#include "Libraries/SdFat/SdFat.h"

/* General Includes */

#include "General/GlobalDefines.h"
#include "General/GlobalVariables.h"
#include "General/GlobalMethods.h"
#include "General/ColorSchemes.h"

/* Modules */

#include "Hardware/Hardware.h"
#include "GUI/GUI.h"
#include "Thermal/Thermal.h"

/* Main Entry point */

void setup()
{
	//Init the hardware components
	initHardware();

	//Enter USB connection if no display attached
	if (checkNoDisplay())
		goto usbConnection;

	//Check for hardware issues
	checkDiagnostic();

	//Do the first start setup if required
	if (checkFirstStart())
		firstStart();

	//Check for firmware upgrade done
	checkFWUpgrade();

	//Read all settings from EEPROM
	readEEPROM();

	//Show the live mode helper if required
	if (checkLiveModeHelper())
		liveModeHelper();

	//Go to the live Mode
	liveMode();

usbConnection:
	//Go to the USB connection
	serialInit();
}

/* Loop forever, only serial mode without display */

void loop()
{
	serialOutput();
}
