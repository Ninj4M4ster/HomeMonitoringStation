#include "sh1106.h"
#include <stdio.h>
#include <string.h>
#include <stdbool.h>

typedef struct
{
    uint8_t command;
    uint8_t parameter;
    uint8_t has_parameter;
} SH1106_InitCommand;

static const SH1106_InitCommand init_sequence[] =
{
    { SH1106_CMD_DISPLAY_OFF,          0, false },

    { SH1106_CMD_SET_CLOCK_DIVIDE,     SH1106_CLOCK_DIVIDE_DEFAULT,   true },
    { SH1106_CMD_SET_MULTIPLEX,        SH1106_MULTIPLEX_64_ROWS,      true },
    { SH1106_CMD_SET_DISPLAY_OFFSET,   SH1106_DISPLAY_OFFSET_DEFAULT, true },
    { SH1106_CMD_SET_START_LINE,       0, false },
    { SH1106_CMD_SET_DC_DC,            SH1106_DC_DC_ENABLE,           true },

    { SH1106_CMD_SEGMENT_REMAP,        0, false },
    { SH1106_CMD_COM_SCAN_REMAP,       0, false },

    { SH1106_CMD_SET_COM_PINS,         SH1106_COM_PINS_128X64,        true },
    { SH1106_CMD_SET_CONTRAST,         SH1106_CONTRAST_DEFAULT,       true },
    { SH1106_CMD_SET_PRECHARGE,        SH1106_PRECHARGE_DEFAULT,      true },
    { SH1106_CMD_SET_VCOM_DESELECT,    SH1106_VCOM_DESELECT_DEFAULT,  true },

    { SH1106_CMD_DISPLAY_RAM,          0, false },
    { SH1106_CMD_DISPLAY_NORMAL,       0, false },
};

static uint8_t framebuffer[SH1106_WIDTH * SH1106_HEIGHT / 8];

static HAL_StatusTypeDef write_command(
    I2C_HandleTypeDef *hi2c,
    uint8_t command)
{
    uint8_t packet[2] = {
        SH1106_I2C_CONTROL_COMMAND,
        command
    };

    return HAL_I2C_Master_Transmit(
        hi2c,
        SH1106_I2C_ADDRESS,
        packet,
        sizeof(packet),
        SH1106_I2C_TIMEOUT_MS
    );
}

static HAL_StatusTypeDef write_command_with_param(
    I2C_HandleTypeDef *hi2c,
    uint8_t command,
    uint8_t parameter)
{
    HAL_StatusTypeDef status = write_command(hi2c, command);

    if (status != HAL_OK)
        return status;

    return write_command(hi2c, parameter);
}

static HAL_StatusTypeDef set_page(
    I2C_HandleTypeDef *hi2c,
    uint8_t page)
{
    if (page >= SH1106_PAGE_COUNT)
        return HAL_ERROR;

    return write_command(
        hi2c,
        SH1106_CMD_SET_PAGE_ADDRESS |
        (page & SH1106_PAGE_ADDRESS_MASK)
    );
}

static HAL_StatusTypeDef set_column(
    I2C_HandleTypeDef *hi2c,
    uint8_t column)
{
    uint16_t address = column + SH1106_COLUMN_OFFSET;

    if (column >= SH1106_WIDTH || address >= 132U)
        return HAL_ERROR;

    HAL_StatusTypeDef status = write_command(
        hi2c,
        SH1106_CMD_SET_LOWER_COLUMN |
        (address & SH1106_COLUMN_NIBBLE_MASK)
    );

    if (status != HAL_OK)
        return status;

    return write_command(
        hi2c,
        SH1106_CMD_SET_HIGHER_COLUMN |
        ((address >> 4) & SH1106_COLUMN_HIGH_MASK)
    );
}

HAL_StatusTypeDef SH1106_Init(I2C_HandleTypeDef *hi2c)
{
    HAL_Delay(100);

    for (uint32_t i = 0;
         i < sizeof(init_sequence) / sizeof(init_sequence[0]);
         i++)
    {
        const SH1106_InitCommand *cmd = &init_sequence[i];

        HAL_StatusTypeDef status;

        if (cmd->has_parameter)
        {
            status = write_command_with_param(
                hi2c,
                cmd->command,
                cmd->parameter
            );
        }
        else
        {
            status = write_command(hi2c, cmd->command);
        }

        if (status != HAL_OK)
            return status;
    }

    memset(framebuffer, 0, sizeof(framebuffer));

    HAL_StatusTypeDef status = SH1106_Update(hi2c);

    if (status != HAL_OK)
        return status;

    return write_command(hi2c, SH1106_CMD_DISPLAY_ON);
}


HAL_StatusTypeDef SH1106_Update(I2C_HandleTypeDef *hi2c)
{
    uint8_t packet[1U + SH1106_WIDTH];

    for (uint8_t page = 0; page < SH1106_PAGE_COUNT; page++)
    {
        HAL_StatusTypeDef status = set_page(hi2c, page);
        if (status != HAL_OK)
            return status;

        status = set_column(hi2c, 0U);
        if (status != HAL_OK)
            return status;

        packet[0] = SH1106_I2C_CONTROL_DATA;

        memcpy(
            &packet[1],
            &framebuffer[page * SH1106_WIDTH],
            SH1106_WIDTH
        );

        status = HAL_I2C_Master_Transmit(
            hi2c,
            SH1106_I2C_ADDRESS,
            packet,
            sizeof(packet),
            SH1106_I2C_TIMEOUT_MS
        );

        if (status != HAL_OK)
            return status;
    }

    return HAL_OK;
}

HAL_StatusTypeDef SH1106_Clear(I2C_HandleTypeDef *hi2c)
{
    memset(framebuffer, 0, sizeof(framebuffer));
    return SH1106_Update(hi2c);
}

void SH1106_Fill(uint8_t pattern)
{
    memset(framebuffer, pattern, sizeof(framebuffer));
}
