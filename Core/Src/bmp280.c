#include "bmp280.h"

static BMP280 bmp280;

HAL_StatusTypeDef BMP280_Init(I2C_HandleTypeDef *hi2c) {
    bmp280.hi2c = hi2c;

    uint8_t data[2] = {
        BMP280_REG_CTRL_MEAS,
        0b00100111
    };

    return HAL_I2C_Master_Transmit(
        hi2c,
        BMP280_WRITE_ADDR,
        data,
        sizeof(data),
        BMP280_I2C_TIMEOUT_MS
    );
}

BMP280_S32_t t_fine; 
BMP280_S32_t bmp280_compensate_T_int32(BMP280_S32_t adc_T) {  
    BMP280_S32_t var1, var2, T;
    var1  = ((((adc_T>>3) - ((BMP280_S32_t)dig_T1<<1))) * ((BMP280_S32_t)dig_T2)) >> 11;
    var2  = (((((adc_T>>4) - ((BMP280_S32_t)dig_T1)) * ((adc_T>>4) - ((BMP280_S32_t)dig_T1))) >> 12) *    ((BMP280_S32_t)dig_T3)) >> 14;
    t_fine = var1 + var2;  T  = (t_fine * 5 + 128) >> 8;  
    return T; 
} 
// Returns pressure in Pa as unsigned 32 bit integer in Q24.8 format (24 integer bits and 8 fractional bits). 
// Output value of “24674867” represents 24674867/256 = 96386.2 Pa = 963.862 hPa 
BMP280_U32_t bmp280_compensate_P_int64(BMP280_S32_t adc_P) {  
    BMP280_S64_t var1, var2, p;
    var1 = ((BMP280_S64_t)t_fine) - 128000;
    var2 = var1 * var1 * (BMP280_S64_t)dig_P6;
    var2 = var2 + ((var1*(BMP280_S64_t)dig_P5)<<17);
    var2 = var2 + (((BMP280_S64_t)dig_P4)<<35);
    var1 = ((var1 * var1 * (BMP280_S64_t)dig_P3)>>8) + ((var1 * (BMP280_S64_t)dig_P2)<<12);
    var1 = (((((BMP280_S64_t)1)<<47)+var1))*((BMP280_S64_t)dig_P1)>>33;
    if (var1 == 0)  {   
        return 0; // avoid exception caused by division by zero  
    }  
    p = 1048576-adc_P;
    p = (((p<<31)-var2)*3125)/var1;
    var1 = (((BMP280_S64_t)dig_P9) * (p>>13) * (p>>13)) >> 25;
    var2 = (((BMP280_S64_t)dig_P8) * p) >> 19;
    p = ((p + var1 + var2) >> 8) + (((BMP280_S64_t)dig_P7)<<4);  
    return (BMP280_U32_t)p; 
}

HAL_StatusTypeDef BMP280_ReadTemp() {
    uint8_t data[1] = {BMP280_REG_TEMP_1};
    HAL_I2C_Master_Transmit(
        bmp280.hi2c,
        BMP280_WRITE_ADDR,
        data,
        sizeof(data),
        BMP280_I2C_TIMEOUT_MS
    );

    uint8_t received_data[3];

    HAL_I2C_Master_Receive(
        bmp280.hi2c,
        BMP280_READ_ADDR,
        received_data,
        sizeof(received_data),
        BMP280_I2C_TIMEOUT_MS
    );
}

float BMP280_GetTemp() {

}
