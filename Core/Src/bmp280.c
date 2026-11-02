#include "bmp280.h"

static BMP280 bmp280;

static uint16_t BMP280_ReadU16(const uint8_t *data)
{
    return (uint16_t)data[0] | ((uint16_t)data[1] << 8);
}

static int16_t BMP280_ReadS16(const uint8_t *data)
{
    return (int16_t)BMP280_ReadU16(data);
}

static HAL_StatusTypeDef BMP280_ReadCalibration(void)
{
    uint8_t data[24];

    HAL_StatusTypeDef status = HAL_I2C_Mem_Read(
        bmp280.hi2c,
        BMP280_WRITE_ADDR,
        BMP280_REG_CALIB,
        I2C_MEMADD_SIZE_8BIT,
        data,
        sizeof(data),
        BMP280_I2C_TIMEOUT_MS
    );

    if (status != HAL_OK)
        return status;

    bmp280.dig_T1 = BMP280_ReadU16(&data[0]);
    bmp280.dig_T2 = BMP280_ReadS16(&data[2]);
    bmp280.dig_T3 = BMP280_ReadS16(&data[4]);

    bmp280.dig_P1 = BMP280_ReadU16(&data[6]);
    bmp280.dig_P2 = BMP280_ReadS16(&data[8]);
    bmp280.dig_P3 = BMP280_ReadS16(&data[10]);
    bmp280.dig_P4 = BMP280_ReadS16(&data[12]);
    bmp280.dig_P5 = BMP280_ReadS16(&data[14]);
    bmp280.dig_P6 = BMP280_ReadS16(&data[16]);
    bmp280.dig_P7 = BMP280_ReadS16(&data[18]);
    bmp280.dig_P8 = BMP280_ReadS16(&data[20]);
    bmp280.dig_P9 = BMP280_ReadS16(&data[22]);

    if (bmp280.dig_P1 == 0)
        return HAL_ERROR;

    return HAL_OK;
}

HAL_StatusTypeDef BMP280_Init(I2C_HandleTypeDef *hi2c)
{
    if (hi2c == NULL)
        return HAL_ERROR;

    bmp280.hi2c = hi2c;

    HAL_StatusTypeDef status = BMP280_ReadCalibration();

    if (status != HAL_OK)
        return status;

    uint8_t config[2] = {
        BMP280_REG_CTRL_MEAS,
        0x27
    };

    status = HAL_I2C_Master_Transmit(
        bmp280.hi2c,
        BMP280_WRITE_ADDR,
        config,
        sizeof(config),
        BMP280_I2C_TIMEOUT_MS
    );

    return status;
}

int32_t BMP280_CompensateTemp(int32_t adc_T)
{
    int32_t var1, var2, T;
    var1 = ((((adc_T >> 3) -
              ((int32_t)bmp280.dig_T1 << 1))) *
              ((int32_t)bmp280.dig_T2)) >> 11;
    var2 = (((((adc_T >> 4) -
               ((int32_t)bmp280.dig_T1)) *
               ((adc_T >> 4) -
               ((int32_t)bmp280.dig_T1))) >> 12) *
               ((int32_t)bmp280.dig_T3)) >> 14;

    bmp280.t_fine = var1 + var2;
    T = (bmp280.t_fine * 5 + 128) >> 8;
    return T;
}

uint32_t BMP280_CompensatePressure(int32_t adc_P)
{
    int64_t var1, var2, p;

    var1 = ((int64_t)bmp280.t_fine) - 128000;
    var2 = var1 * var1 * (int64_t)bmp280.dig_P6;
    var2 = var2 + ((var1 *
                    (int64_t)bmp280.dig_P5) << 17);
    var2 = var2 + (((int64_t)bmp280.dig_P4) << 35);
    var1 = ((var1 * var1 *
             (int64_t)bmp280.dig_P3) >> 8) +
           ((var1 *
             (int64_t)bmp280.dig_P2) << 12);
    var1 = (((((int64_t)1) << 47) + var1) *
             (int64_t)bmp280.dig_P1) >> 33;

    if (var1 == 0)
        return 0;

    p = 1048576 - adc_P;
    p = (((p << 31) - var2) * 3125) / var1;
    var1 = (((int64_t)bmp280.dig_P9) *
            (p >> 13) * (p >> 13)) >> 25;
    var2 = (((int64_t)bmp280.dig_P8) * p) >> 19;
    p = ((p + var1 + var2) >> 8) +
        (((int64_t)bmp280.dig_P7) << 4);

    return (uint32_t)p;
}

HAL_StatusTypeDef BMP280_ReadMeasurements(void)
{
    uint8_t data[6];

    HAL_StatusTypeDef status = HAL_I2C_Mem_Read(
        bmp280.hi2c,
        BMP280_WRITE_ADDR,
        BMP280_REG_PRESS_MSB,
        I2C_MEMADD_SIZE_8BIT,
        data,
        sizeof(data),
        BMP280_I2C_TIMEOUT_MS
    );

    if (status != HAL_OK)
        return status;

    bmp280.adc_P =
        ((int32_t)data[0] << 12) |
        ((int32_t)data[1] << 4)  |
        ((int32_t)data[2] >> 4);

    bmp280.adc_T =
        ((int32_t)data[3] << 12) |
        ((int32_t)data[4] << 4)  |
        ((int32_t)data[5] >> 4);

    int32_t temperature_raw =
        BMP280_CompensateTemp(bmp280.adc_T);
    bmp280.temperature = temperature_raw / 100.0f;

    uint32_t pressure_raw =
        BMP280_CompensatePressure(bmp280.adc_P);
    bmp280.pressure = pressure_raw / 256.0f;

    return HAL_OK;
}

float BMP280_GetTemp(void)
{
    return bmp280.temperature;
}

float BMP280_GetPressure(void)
{
    return bmp280.pressure;
}
