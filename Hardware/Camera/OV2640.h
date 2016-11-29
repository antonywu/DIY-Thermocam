/*
*
* OV2640 - Driver for the Arducam camera module
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

/* Include */

#include "OV2640Regs.h"

/* Sensor related definitions */

#define BMP 0
#define JPEG 1

#define OV2640_ADDRESS 0x60

#define OV2640_160x120 		0
#define OV2640_176x144 		1
#define OV2640_320x240 		2
#define OV2640_352x288 		3
#define OV2640_640x480		4	
#define OV2640_800x600 		5	
#define OV2640_1024x768		6	
#define OV2640_1280x1024	7	
#define OV2640_1600x1200	8	

#define OV2640_CHIPID_HIGH 0x0A
#define OV2640_CHIPID_LOW 0x0B

/* I2C control definition */

#define I2C_ADDR_8BIT 0
#define I2C_ADDR_16BIT 1
#define I2C_REG_8BIT 0
#define I2C_REG_16BIT 1
#define I2C_DAT_8BIT 0
#define I2C_DAT_16BIT 1

/* Register initialization tables for SENSORs */

#define SENSOR_REG_TERM_8BIT                0xFF
#define SENSOR_REG_TERM_16BIT               0xFFFF
#define SENSOR_VAL_TERM_8BIT                0xFF
#define SENSOR_VAL_TERM_16BIT               0xFFFF
#define MAX_FIFO_SIZE		0x5FFFF

/* ArduChip registers definition */

#define RWBIT 0x80 
#define ARDUCHIP_TEST1 0x00 
#define ARDUCHIP_MODE 0x02  
#define MCU2LCD_MODE 0x00
#define CAM2LCD_MODE 0x01
#define LCD2MCU_MODE 0x02
#define ARDUCHIP_TIM 0x03
#define ARDUCHIP_FIFO 0x04 
#define FIFO_CLEAR_MASK 0x01
#define FIFO_START_MASK 0x02
#define FIFO_RDPTR_RST_MASK 0x10
#define FIFO_WRPTR_RST_MASK 0x20
#define ARDUCHIP_GPIO 0x06
#define GPIO_RESET_MASK 0x01
#define GPIO_PWDN_MASK 0x02 
#define GPIO_PWREN_MASK	0x04
#define BURST_FIFO_READ	0x3C
#define SINGLE_FIFO_READ 0x3D
#define ARDUCHIP_REV 0x40
#define VER_LOW_MASK 0x3F
#define VER_HIGH_MASK 0xC0
#define ARDUCHIP_TRIG 0x41
#define VSYNC_MASK 0x01
#define SHUTTER_MASK 0x02
#define CAP_DONE_MASK 0x08
#define FIFO_SIZE1 0x42
#define FIFO_SIZE2 0x43
#define FIFO_SIZE3 0x44

/* Methods */

/* I2C Write 8bit address, 8bit data */
byte ov2640_wrSensorReg8_8(int regID, int regDat) {
	Wire.beginTransmission(0x60 >> 1);
	Wire.write(regID & 0x00FF);
	Wire.write(regDat & 0x00FF);

	if (Wire.endTransmission())
		return 0;

	delay(1);

	return 1;
}

/* I2C Read 8bit address, 8bit data */
byte ov2640_rdSensorReg8_8(uint8_t regID, uint8_t* regDat) {
	Wire.beginTransmission(0x60 >> 1);
	Wire.write(regID & 0x00FF);
	Wire.endTransmission();

	Wire.requestFrom((0x60 >> 1), 1);
	if (Wire.available())
		*regDat = Wire.read();

	delay(1);
	return (1);
}

/* I2C Array Write 8bit address, 8bit data */
int ov2640_wrSensorRegs8_8(const struct sensor_reg* reglist) {
	uint16_t reg_addr = 0;
	uint16_t reg_val = 0;
	const struct sensor_reg *next = reglist;
	while ((reg_addr != 0xff) | (reg_val != 0xff))
	{
		reg_addr = pgm_read_word(&next->reg);
		reg_val = pgm_read_word(&next->val);
		ov2640_wrSensorReg8_8(reg_addr, reg_val);
		next++;
	}

	return 1;
}

/* SPI write operation */
void ov2640_busWrite(int address, int value) {
	//Take the SS pin low to select the chip
	SPI.beginTransaction(SPISettings(8000000, MSBFIRST, SPI_MODE0));
	CORE_PIN8_CONFIG = PORT_PCR_MUX(2);
	CORE_PIN12_CONFIG = PORT_PCR_MUX(1);
	digitalWrite(pin_cam_cs, LOW);
	//Send in the address and value via SPI
	SPI.transfer(address);
	SPI.transfer(value);
	//Take the SS pin high to de-select the chip
	digitalWrite(pin_cam_cs, HIGH);
	CORE_PIN8_CONFIG = PORT_PCR_MUX(1);
	CORE_PIN12_CONFIG = PORT_PCR_MUX(2);
	SPI.endTransaction();
}

/* SPI read operation */
uint8_t ov2640_busRead(int address) {
	//Take the SS pin low to select the chip
	SPI.beginTransaction(SPISettings(8000000, MSBFIRST, SPI_MODE0));
	CORE_PIN8_CONFIG = PORT_PCR_MUX(2);
	CORE_PIN12_CONFIG = PORT_PCR_MUX(1);
	digitalWrite(pin_cam_cs, LOW);
	//Send in the address and value via SPI
	SPI.transfer(address);
	uint8_t value = SPI.transfer(0x00);
	//Take the SS pin high to de-select the chip
	digitalWrite(pin_cam_cs, HIGH);
	CORE_PIN8_CONFIG = PORT_PCR_MUX(1);
	CORE_PIN12_CONFIG = PORT_PCR_MUX(2);
	SPI.endTransaction();
	//Return value
	return value;
}

/* Read ArduChip internal registers */
uint8_t ov2640_readReg(uint8_t addr) {
	uint8_t data;
	data = ov2640_busRead(addr & 0x7F);
	return data;
}

/* Write ArduChip internal registers */
void ov2640_writeReg(uint8_t addr, uint8_t data) {
	ov2640_busWrite(addr | 0x80, data);
}

/* Init the camera */
boolean ov2640_init(void) {
	uint8_t vid, pid;

	//Test SPI connection first
	ov2640_writeReg(ARDUCHIP_TEST1, 0x55);
	uint8_t rtnVal = ov2640_readReg(ARDUCHIP_TEST1);
	if (rtnVal != 0x55)
		return 0;

	//Test I2C connection second
	ov2640_wrSensorReg8_8(0xff, 0x01);
	ov2640_rdSensorReg8_8(OV2640_CHIPID_HIGH, &vid);
	ov2640_rdSensorReg8_8(OV2640_CHIPID_LOW, &pid);
	if ((vid != 0x26) || (pid != 0x42))
		return 0;

	//Init registers
	ov2640_wrSensorReg8_8(0xff, 0x01);
	ov2640_wrSensorReg8_8(0x12, 0x80);

	//Wait some time
	delay(100);

	//Set format to JPEG
	ov2640_wrSensorRegs8_8(OV2640_JPEG_INIT);
	ov2640_wrSensorRegs8_8(OV2640_YUV422);
	ov2640_wrSensorRegs8_8(OV2640_JPEG);
	ov2640_wrSensorReg8_8(0xff, 0x01);
	ov2640_wrSensorReg8_8(0x15, 0x00);
	ov2640_wrSensorRegs8_8(OV2640_320x240_JPEG);

	//Everything works
	return 1;
}

/* Set the format to JPEG or BMP */
void ov2640_setFormat(byte fmt) {
	//BMP
	if (fmt == BMP) {
		ov2640_wrSensorRegs8_8(OV2640_QVGA);
	}
	//JPEG
	else {
		ov2640_wrSensorRegs8_8(OV2640_JPEG_INIT);
		ov2640_wrSensorRegs8_8(OV2640_YUV422);
		ov2640_wrSensorRegs8_8(OV2640_JPEG);
		ov2640_wrSensorReg8_8(0xff, 0x01);
		ov2640_wrSensorReg8_8(0x15, 0x00);
		ov2640_wrSensorRegs8_8(OV2640_320x240_JPEG);
	}
}

/* Set the JPEG pixel size of the image */
void ov2640_setJPEGSize(uint8_t size) {
	switch (size)
	{
	case OV2640_160x120:
		ov2640_wrSensorRegs8_8(OV2640_160x120_JPEG);
		break;
	case OV2640_176x144:
		ov2640_wrSensorRegs8_8(OV2640_176x144_JPEG);
		break;
	case OV2640_320x240:
		ov2640_wrSensorRegs8_8(OV2640_320x240_JPEG);
		break;
	case OV2640_352x288:
		ov2640_wrSensorRegs8_8(OV2640_352x288_JPEG);
		break;
	case OV2640_640x480:
		ov2640_wrSensorRegs8_8(OV2640_640x480_JPEG);
		break;
	case OV2640_800x600:
		ov2640_wrSensorRegs8_8(OV2640_800x600_JPEG);
		break;
	case OV2640_1024x768:
		ov2640_wrSensorRegs8_8(OV2640_1024x768_JPEG);
		break;
	case OV2640_1280x1024:
		ov2640_wrSensorRegs8_8(OV2640_1280x1024_JPEG);
		break;
	case OV2640_1600x1200:
		ov2640_wrSensorRegs8_8(OV2640_1600x1200_JPEG);
		break;
	default:
		ov2640_wrSensorRegs8_8(OV2640_320x240_JPEG);
		break;
	}
}

/* Read Write FIFO length */
uint32_t ov2640_readFifoLength(void) {
	uint32_t len1, len2, len3, length;
	len1 = ov2640_readReg(FIFO_SIZE1);
	len2 = ov2640_readReg(FIFO_SIZE2);
	len3 = ov2640_readReg(FIFO_SIZE3) & 0x7f;
	length = ((len3 << 16) | (len2 << 8) | len1) & 0x07fffff;
	return length;
}

/* Start the FIFO burst mode */
void ov2640_startFifoBurst(void) {
	SPI.beginTransaction(SPISettings(4000000, MSBFIRST, SPI_MODE0));
	CORE_PIN8_CONFIG = PORT_PCR_MUX(2);
	CORE_PIN12_CONFIG = PORT_PCR_MUX(1);
	digitalWrite(pin_cam_cs, LOW);
	SPI.transfer(BURST_FIFO_READ);
}

/* End the FIFO burst mode */
void ov2640_endFifoBurst(void) {
	digitalWrite(pin_cam_cs, HIGH);
	CORE_PIN8_CONFIG = PORT_PCR_MUX(1);
	CORE_PIN12_CONFIG = PORT_PCR_MUX(2);
	SPI.endTransaction();
}

/* Set corresponding bit */
void ov2640_setBit(uint8_t addr, uint8_t bit) {
	uint8_t temp;
	temp = ov2640_readReg(addr);
	ov2640_writeReg(addr, temp | bit);
}

/* Clear corresponding bit */
void ov2640_clearBit(uint8_t addr, uint8_t bit) {
	uint8_t temp;
	temp = ov2640_readReg(addr);
	ov2640_writeReg(addr, temp & (~bit));
}

/* Get corresponding bit status */
uint8_t ov2640_getBit(uint8_t addr, uint8_t bit) {
	uint8_t temp;
	temp = ov2640_readReg(addr);
	temp = temp & bit;
	return temp;
}

/* Set ArduCAM working mode */
void ov2640_setMode(uint8_t mode) {
	switch (mode)
	{
		//MCU2LCD_MODE: MCU writes the LCD screen GRAM
	case MCU2LCD_MODE:
		ov2640_writeReg(ARDUCHIP_MODE, MCU2LCD_MODE);
		break;
		//CAM2LCD_MODE: Camera takes control of the LCD screen
	case CAM2LCD_MODE:
		ov2640_writeReg(ARDUCHIP_MODE, CAM2LCD_MODE);
		break;
		//LCD2MCU_MODE: MCU read the LCD screen GRAM
	case LCD2MCU_MODE:
		ov2640_writeReg(ARDUCHIP_MODE, LCD2MCU_MODE);
		break;
		//Default is MCU2LCD_MODE
	default:
		ov2640_writeReg(ARDUCHIP_MODE, MCU2LCD_MODE);
		break;
	}
}

/* Reset the FIFO pointer to zero */
void ov2640_flushFifo(void) {
	ov2640_writeReg(ARDUCHIP_FIFO, FIFO_CLEAR_MASK);
}

/* Send capture command */
void ov2640_startCapture(void) {
	ov2640_writeReg(ARDUCHIP_FIFO, FIFO_START_MASK);
}

/* Clear FIFO Complete flag */
void ov2640_clearFifoFlag(void) {
	ov2640_writeReg(ARDUCHIP_FIFO, FIFO_CLEAR_MASK);
}

/* Read FIFO single */
uint8_t ov2640_readFifo(void) {
	uint8_t data;
	data = ov2640_busRead(SINGLE_FIFO_READ);
	return data;
}

/* Send the capture command to the camera */
void ov2640_capture(void) {
	//Flush the FIFO
	ov2640_flushFifo();
	//Clear the capture done flag
	ov2640_clearFifoFlag();
	//Start capture
	ov2640_startCapture();
}

/* Transfer the JPEG data from the OV2640 */
void ov2640_transfer(uint8_t * jpegData, boolean stream)
{
	//Count variable
	uint32_t counter = 0;

	//Start FIFO Burst
	ov2640_startFifoBurst();

	//Transfer data
	uint8_t temp = 0xff, temp_last;
	boolean is_header = 0;
	while (1)
	{
		temp_last = temp;
		temp = SPI.transfer(0x00);
		if (is_header == 1)
		{
			jpegData[counter] = temp;
			counter++;
		}
		else if ((temp == 0xD8) & (temp_last == 0xFF))
		{
			is_header = 1;
			jpegData[0] = temp_last;
			jpegData[1] = temp;
			counter = 2;
		}

		if((counter == 20) && !stream)
		{
			for(uint8_t i=0;i < sizeof(exifHeader_ov2640); i++)
			{
				jpegData[counter + i] = exifHeader_ov2640[i];
			}
			counter += sizeof(exifHeader_ov2640);
		}

		if ((temp == 0xD9) && (temp_last == 0xFF))
			break;
	}

	//Stop FIFO Burst
	ov2640_endFifoBurst();
}