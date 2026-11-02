#ifndef BMP280_H
#define BMP280_H

#include "stm32l4xx_hal.h"
#include "stm32l4xx_hal_def.h"

#define BMP280_WRITE_ADDR 0b11101100
#define BMP280_READ_ADDR 0b11101101

#define BMP280_REG_CTRL_MEAS 0xF4
#define BMP280_REG_TEMP_1 0xFA

#define BMP280_I2C_TIMEOUT_MS 100U

typedef struct {
    I2C_HandleTypeDef *hi2c;
} BMP280;

HAL_StatusTypeDef BMP280_Init(I2C_HandleTypeDef *hi2c);
HAL_StatusTypeDef BMP280_ReadTemp();
float BMP280_GetTemp();

#endif
