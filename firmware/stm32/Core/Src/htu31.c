
#include "htu31.h"

#define HTU31_CMD_CONVERSION   0x5E
#define HTU31_CMD_READ_T_RH    0x00

#define HTU31_TIMEOUT          100

static HTU31 htu31 = {NULL, 0.0f, 0.0f};

// CRC-8: polynomial 0x31, initial value 0x00
static uint8_t HTU31_CRC8(uint8_t *data, uint8_t len)
{
    uint8_t crc = 0x00;

    for (uint8_t i = 0; i < len; i++)
    {
        crc ^= data[i];

        for (uint8_t bit = 0; bit < 8; bit++)
        {
            if (crc & 0x80)
                crc = (crc << 1) ^ 0x31;
            else
                crc <<= 1;
        }
    }

    return crc;
}


// Check sensor communication
HAL_StatusTypeDef HTU31_Init(I2C_HandleTypeDef *hi2c)
{
    htu31.hi2c = hi2c;
    return HAL_I2C_IsDeviceReady(
        hi2c,
        HTU31_I2C_ADDR,
        3,
        HTU31_TIMEOUT
    );
}


// Read temperature and humidity
HAL_StatusTypeDef HTU31_Read()
{
    HAL_StatusTypeDef status;

    uint8_t command;
    uint8_t rx_data[6];

    uint16_t raw_temp;
    uint16_t raw_humidity;

    // 1. Start temperature and humidity conversion
    command = HTU31_CMD_CONVERSION;

    status = HAL_I2C_Master_Transmit(
        htu31.hi2c,
        HTU31_I2C_ADDR,
        &command,
        1,
        HTU31_TIMEOUT
    );

    if (status != HAL_OK)
        return status;

    // Wait for maximum conversion time at OSR 3
    HAL_Delay(25);

    // 2. Request temperature and humidity data
    command = HTU31_CMD_READ_T_RH;

    status = HAL_I2C_Master_Transmit(
        htu31.hi2c,
        HTU31_I2C_ADDR,
        &command,
        1,
        HTU31_TIMEOUT
    );

    if (status != HAL_OK)
        return status;

    // 3. Read 6 bytes:
    // Temperature MSB, LSB, CRC
    // Humidity MSB, LSB, CRC

    status = HAL_I2C_Master_Receive(
        htu31.hi2c,
        HTU31_I2C_ADDR,
        rx_data,
        6,
        HTU31_TIMEOUT
    );

    if (status != HAL_OK)
        return status;

    // 4. Verify temperature CRC
    if (HTU31_CRC8(&rx_data[0], 2) != rx_data[2])
        return HAL_ERROR;

    // 5. Verify humidity CRC
    if (HTU31_CRC8(&rx_data[3], 2) != rx_data[5])
        return HAL_ERROR;

    // 6. Convert raw values
    raw_temp = ((uint16_t)rx_data[0] << 8) | rx_data[1];

    raw_humidity = ((uint16_t)rx_data[3] << 8) | rx_data[4];

    htu31.temperature =
        -40.0f + 165.0f * ((float)raw_temp / 65536.0f);

    htu31.humidity =
        100.0f * ((float)raw_humidity / 65536.0f);

    return HAL_OK;
}

float HTU31_GetTemp(void) {    
    return htu31.temperature;
}

float HTU31_GetHumidity(void) {
    return htu31.humidity;
}
