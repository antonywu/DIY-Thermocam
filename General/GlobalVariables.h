/*
*
* GLOBAL VARIABLES - Global variable declarations, that are used firmware-wide
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

//320x240 buffer (Teensy 3.6 only)
unsigned short* bigBuffer;
//160x120 buffer
unsigned short* smallBuffer;

//Fonts
extern uint8_t tinyFont[];
extern uint8_t smallFont[];
extern uint8_t bigFont[];

//Timer
Metro screenOff;
boolean screenPressed;
byte screenOffTime;

//Button Debouncer
Bounce buttonDebouncer(pin_button, 100);
Bounce touchDebouncer(pin_touch_irq, 100);

//SD
SdFat sd;
SdFile sdFile;
String sdInfo;

//Save filename 
char saveFilename[20];

//Battery ADC
ADC *batMeasure;
//Battery
int8_t batPercentage;
long batTimer;

//Convert RAW to BMP
bool convertEnabled;
//Save visual image
bool visualEnabled;
//Automatic mode
bool autoMode;
//Lock current limits
bool limitsLocked;
//Display rotation
bool rotationEnabled;

//Display options
bool batteryEnabled;
bool timeEnabled;
bool dateEnabled;
bool spotEnabled;
bool colorbarEnabled;
bool storageEnabled;
byte filterType;
byte minMaxPoints;

//Temperature format
bool tempFormat;

//Text color
byte textColor;

//Laser state
bool laserEnabled;

//Display mode
byte displayMode;

//Resolution, V2 only
bool hqRes;

//FLIR Lepton sensor version
byte leptonVersion;
//FLIR Lepton Shutter mode
byte leptonShutter;

//MLX90614 sensor version
boolean mlx90614Version;

//Teensy version
byte teensyVersion;

//HW diagnostic information
byte diagnostic = diag_ok;

//Current color scheme - standard is rainbow
byte colorScheme;
//Pointer to the current color scheme
const byte *colorMap;
//Number of rgb elements inside the color scheme
int16_t colorElements;

//Calibration offset
float calOffset;
//Calibration slope
float calSlope;
//Calibration status
byte calStatus;
//Calibration compensation
float calComp;
//Calibration warmup timer
long calTimer;

//Min & max lepton raw values
uint16_t maxValue;
uint16_t minValue;

//Position of min and maxtemp
uint16_t minTempPos;
uint16_t minTempVal;
uint16_t maxTempPos;
uint16_t maxTempVal;

//Hot / Cold mode
byte hotColdMode;
int16_t hotColdLevel;
byte hotColdColor;

//Array to store the tempPoints
uint16_t tempPoints[96][2];

//Adjust combined image
float adjCombAlpha;
float adjCombFactor;
byte adjCombLeft;
byte adjCombRight;
byte adjCombUp;
byte adjCombDown;

//Save Image in the next cycle
volatile byte imgSave;
//Save Video in the next cycle
volatile byte videoSave;
//Show Live Mode Menu in the next cycle
volatile bool showMenu;
//Handler for a long touch press
volatile bool longTouch;
//Check if in serial mode
volatile bool serialMode;
//Load touch decision marker
volatile byte loadTouch;