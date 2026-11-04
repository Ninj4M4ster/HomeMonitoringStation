#ifndef HTU31_H
#define HTU31_H

#include "stm32l4xx_hal.h"
#include <stdint.h>

#define HTU31_I2C_ADDR    (0x40 << 1)

// Measurement data
typedef struct
{
    I2C_HandleTypeDef *hi2c;
    float temperature;   // Celsius
    float humidity;      // Relative humidity (%)
} HTU31;

// Functions
HAL_StatusTypeDef HTU31_Init(I2C_HandleTypeDef *hi2c);

HAL_StatusTypeDef HTU31_Read();
float HTU31_GetTemp(void);
float HTU31_GetHumidity(void);

#endif
