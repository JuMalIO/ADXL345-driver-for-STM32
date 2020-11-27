#ifndef __adxl345_H
#define __adxl345_H


#include <stdint.h>

typedef struct __ADXL345_Position
{
    float X;
    float Y;
    float Z;
} ADXL345_Position;

_Bool ADXL345_Init(void);
void ADXL345_SetSleep(_Bool enabled);
ADXL345_Position* ADXL345_GetAccelerometerPosition(void);
void ADXL345_GetCalibratedOffsets(uint8_t* offsets);
void ADXL345_SetOffsets(uint8_t* offsets);


#endif /*__ adxl345_H */
