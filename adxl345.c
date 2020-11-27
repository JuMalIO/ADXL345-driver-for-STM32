#include "adxl345.h"
#include "main.h"

#define ACCELELEROMETER_TO_G .0078

#define ADXL345_ADDR 0x53<<1

//ADXL Register Map
#define	DEVID						0x00	//Device ID Register
#define THRESH_TAP			0x1D	//Tap Threshold
#define	OFSX						0x1E	//X-axis offset
#define	OFSY						0x1F	//Y-axis offset
#define	OFSZ						0x20	//Z-axis offset
#define	DUR							0x21	//Tap Duration
#define	Latent					0x22	//Tap latency
#define	Window					0x23	//Tap window
#define	THRESH_ACT			0x24	//Activity Threshold
#define	THRESH_INACT		0x25	//Inactivity Threshold
#define	TIME_INACT			0x26	//Inactivity Time
#define	ACT_INACT_CTL		0x27	//Axis enable control for activity and inactivity detection
#define	THRESH_FF				0x28	//free-fall threshold
#define	TIME_FF					0x29	//Free-Fall Time
#define	TAP_AXES				0x2A	//Axis control for tap/double tap
#define ACT_TAP_STATUS	0x2B	//Source of tap/double tap
#define	BW_RATE					0x2C	//Data rate and power mode control
#define POWER_CTL				0x2D	//Power Control Register
#define	INT_ENABLE			0x2E	//Interrupt Enable Control
#define	INT_MAP					0x2F	//Interrupt Mapping Control
#define	INT_SOURCE			0x30	//Source of interrupts
#define	DATA_FORMAT			0x31	//Data format control
#define DATAX0					0x32	//X-Axis Data 0
#define DATAX1					0x33	//X-Axis Data 1
#define DATAY0					0x34	//Y-Axis Data 0
#define DATAY1					0x35	//Y-Axis Data 1
#define DATAZ0					0x36	//Z-Axis Data 0
#define DATAZ1					0x37	//Z-Axis Data 1
#define	FIFO_CTL				0x38	//FIFO control
#define	FIFO_STATUS			0x39	//FIFO status

//Power Control Register Bits
#define WU_0		 	 (1<<0)	//Wake Up Mode - Bit 0
#define	WU_1		 	 (1<<1)	//Wake Up mode - Bit 1
#define SLEEP		 	 (1<<2)	//Sleep Mode
#define	MEASURE	 	 (1<<3)	//Measurement Mode
#define AUTO_SLP 	 (1<<4)	//Auto Sleep Mode bit
#define LINK		 	 (1<<5)	//Link bit

//Interrupt Enable/Interrupt Map/Interrupt Source Register Bits
#define	OVERRUN		 (1<<0)
#define	WATERMARK	 (1<<1)
#define FREE_FALL	 (1<<2)
#define	INACTIVITY (1<<3)
#define	ACTIVITY	 (1<<4)
#define DOUBLE_TAP (1<<5)
#define	SINGLE_TAP (1<<6)
#define	DATA_READY (1<<7)

//Data Format Bits
#define RANGE_0		 (1<<0)
#define	RANGE_1		 (1<<1)
#define JUSTIFY		 (1<<2)
#define	FULL_RES	 (1<<3)

#define	INT_INVERT (1<<5)
#define	SPI			   (1<<6)
#define	SELF_TEST	 (1<<7)

static ADXL345_Position Position;

extern I2C_HandleTypeDef hi2c1;

uint8_t ADXL345_GetDeviceId(void)
{
	uint8_t data;
	
	HAL_I2C_Mem_Read(&hi2c1, ADXL345_ADDR, DEVID, 1, &data, 1, 100);
	
	return data;
}

void ADXL345_SetDataFormat(uint8_t dataFormat)
{
	HAL_I2C_Mem_Write(&hi2c1, ADXL345_ADDR, DATA_FORMAT, 1, &dataFormat, 1, 100);
}

_Bool ADXL345_Init(void)
{
	uint32_t p = 250000;
	while(p > 0)
	{
		p--;
	}
	
	if (HAL_I2C_IsDeviceReady(&hi2c1, ADXL345_ADDR, 1, 20000) != HAL_OK)
	{
		return 0;
	}
	
	if (ADXL345_GetDeviceId() == 0xE5)
	{
		return 0;
	}
	
	ADXL345_SetDataFormat(0x00);
	ADXL345_SetSleep(1);
	
	return 1;
}

void ADXL345_SetSleep(_Bool enabled)
{
	uint8_t data = enabled ? 0x00 : 0x08;
	
  HAL_I2C_Mem_Write(&hi2c1, ADXL345_ADDR, POWER_CTL, 1, &data, 1, 100);
}

ADXL345_Position* ADXL345_GetAccelerometerPosition(void)
{
	uint8_t data[6];
	
	HAL_I2C_Mem_Read(&hi2c1, ADXL345_ADDR, DATAX0, 1, data, 6, 10);
	
	Position.X = (int16_t)((data[1] << 8) | data[0]) * ACCELELEROMETER_TO_G;
	Position.Y = (int16_t)((data[3] << 8) | data[2]) * ACCELELEROMETER_TO_G;
	Position.Z = (int16_t)((data[5] << 8) | data[4]) * ACCELELEROMETER_TO_G;

	return &Position;
}

int16_t ADXL345_GetAccelerometerAxisDataRaw(uint8_t reg)
{
	uint8_t data[2];
	
	HAL_I2C_Mem_Read(&hi2c1, ADXL345_ADDR, reg, 1, data, 2, 10);

	return (int16_t)((data[1] << 8) | data[0]);
}

#define CALIBRATE_SIZE 100

int8_t ADXL345_GetCalibratedAxisOffset(uint8_t reg)
{
	int32_t values;
	
	for (uint8_t i; i < CALIBRATE_SIZE; i++)
	{
		values += ADXL345_GetAccelerometerAxisDataRaw(reg);
		
		HAL_Delay(1);
	}
	
	return -(values / (float)CALIBRATE_SIZE) / 4;
}

void ADXL345_GetCalibratedOffsets(uint8_t* offsets)
{
	offsets[0] = ADXL345_GetCalibratedAxisOffset(DATAX0);
	offsets[1] = ADXL345_GetCalibratedAxisOffset(DATAY0);
	offsets[2] = ADXL345_GetCalibratedAxisOffset(DATAZ0);
}

void ADXL345_SetOffsets(uint8_t* offsets)
{
  HAL_I2C_Mem_Write(&hi2c1, ADXL345_ADDR, OFSX, 1, offsets, 3, 100);
}
