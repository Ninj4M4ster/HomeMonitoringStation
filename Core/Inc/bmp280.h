#ifndef BMP280_H
#define BMP280_H

#include "stm32l4xx_hal.h"
#include <stdint.h>

#define BMP280_WRITE_ADDR       0xEC
#define BMP280_READ_ADDR        0xED

#define BMP280_REG_CALIB        0x88
#define BMP280_REG_CTRL_MEAS    0xF4
#define BMP280_REG_PRESS_MSB    0xF7
#define BMP280_REG_TEMP_MSB     0xFA

#define BMP280_I2C_TIMEOUT_MS   100U

typedef struct {
    I2C_HandleTypeDef *hi2c;

    uint16_t dig_T1;
    int16_t  dig_T2;
    int16_t  dig_T3;

    uint16_t dig_P1;
    int16_t  dig_P2;
    int16_t  dig_P3;
    int16_t  dig_P4;
    int16_t  dig_P5;
    int16_t  dig_P6;
    int16_t  dig_P7;
    int16_t  dig_P8;
    int16_t  dig_P9;

    int32_t t_fine;

    int32_t adc_T;
    int32_t adc_P;

    float temperature;
    float pressure;
} BMP280;

HAL_StatusTypeDef BMP280_Init(I2C_HandleTypeDef *hi2c);

HAL_StatusTypeDef BMP280_ReadMeasurements(void);

float BMP280_GetTemp(void);
float BMP280_GetPressure(void);

#endif