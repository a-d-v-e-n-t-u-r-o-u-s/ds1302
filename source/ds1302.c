/*!
 * \file
 * \brief DS1302 implementation file
 * \author Dawid Babula
 * \email dbabula@adventurous.pl
 *
 * \par Copyright (C) Dawid Babula, 2020
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#include "ds1302.h"
#include "gpio.h"
#include <util/delay.h>
#include <stddef.h>
#include <string.h>

#define READ_SECONDS            (0x81)
#define WRITE_SECONDS           (0x80)

#define READ_MINUTES            (0x83)
#define WRITE_MINUTES           (0x82)

#define READ_HOURS              (0x85)
#define WRITE_HOURS             (0x84)

#define READ_DATE               (0x87)
#define WRITE_DATE              (0x86)

#define READ_MONTH              (0x89)
#define WRITE_MONTH             (0x88)

#define READ_WEEKDAY            (0x8B)
#define WRITE_WEEKDAY           (0x8A)

#define READ_YEAR               (0x8D)
#define WRITE_YEAR              (0x8C)

#define READ_WP                 (0x8F)
#define WRITE_WP                (0x8E)

typedef enum
{
    DS1302_SECONDS,
    DS1302_MINUTES,
    DS1302_HOURS,
    DS1302_WEEKDAY,
    DS1302_DATE,
    DS1302_MONTH,
    DS1302_YEAR,
    DS1302_FORMAT,
} DS1302_entry_t;

static uint8_t pins[3][2];

static uint8_t get_value_to_store(DS1302_entry_t entry, uint8_t val)
{
    switch(entry)
    {
        case DS1302_SECONDS:
        case DS1302_MINUTES:
        case DS1302_HOURS:
            return ((((val/10u) << 4u) & 0x70u) | (val % 10U));
            break;
        case DS1302_WEEKDAY:
            return (val &= 0x03);
        case DS1302_DATE:
            return  (((val / 10) << 4) & 0x30) | (val % 10);
        case DS1302_MONTH:
            return (((val / 10) << 4) & 0x10) | (val % 10);
        case DS1302_YEAR:
            return (((val / 10) << 4) & 0xF0) | (val % 10);
        case DS1302_FORMAT:
            break;
    }

    return 0;
}

static uint8_t get_value_to_load(DS1302_entry_t entry, uint8_t val)
{
    switch(entry)
    {
        case DS1302_SECONDS:
        case DS1302_MINUTES:
        case DS1302_HOURS:
            return (val & 0x0F) + ((val & 0x70) >> 4) * 10;
        case DS1302_WEEKDAY:
            return (val &= 0x07);
        case DS1302_DATE:
            return (val & 0x0F) + ((val & 0x30) >> 4) * 10;
        case DS1302_MONTH:
            return (val & 0x0F) + ((val & 0x10) >> 4) * 10;
        case DS1302_YEAR:
            return (val & 0x0F) + ((val & 0xF0) >> 4) * 10;
        case DS1302_FORMAT:
            break;
    }

    return 0;
}

static inline void stop(void)
{
    const uint8_t ce_port = pins[DS1302_CE_IDX][0];
    const uint8_t ce_pin = pins[DS1302_CE_IDX][1];
    const uint8_t clk_port = pins[DS1302_CLK_IDX][0];
    const uint8_t clk_pin = pins[DS1302_CLK_IDX][1];

    GPIO_write_pin(ce_port, ce_pin, 0U);
    GPIO_write_pin(clk_port, clk_pin, 0U);
}

static inline void reset(void)
{
    const uint8_t ce_port = pins[DS1302_CE_IDX][0];
    const uint8_t ce_pin = pins[DS1302_CE_IDX][1];

    stop();

    GPIO_write_pin(ce_port, ce_pin, 1U);
}

static void write_byte(uint8_t data)
{
    const uint8_t io_port = pins[DS1302_IO_IDX][0];
    const uint8_t io_pin = pins[DS1302_IO_IDX][1];
    const uint8_t clk_port = pins[DS1302_CLK_IDX][0];
    const uint8_t clk_pin = pins[DS1302_CLK_IDX][1];

    GPIO_config_pin(io_port, io_pin, GPIO_OUTPUT_PUSH_PULL);

    for(uint8_t i = 0u; i < 8U; i++)
    {
        if((data & 0x01) != 0U)
        {
            GPIO_write_pin(io_port, io_pin, 1U);
        }
        else
        {
            GPIO_write_pin(io_port, io_pin, 0U);
        }

        GPIO_write_pin(clk_port, clk_pin, 0U);
        _delay_us(2);
        GPIO_write_pin(clk_port, clk_pin, 1U);
        _delay_us(2);

        data >>= 1U;
    }
}

static uint8_t read_byte(void)
{
    uint8_t ret = 0;
    const uint8_t io_port = pins[DS1302_IO_IDX][0];
    const uint8_t io_pin = pins[DS1302_IO_IDX][1];
    const uint8_t clk_port = pins[DS1302_CLK_IDX][0];
    const uint8_t clk_pin = pins[DS1302_CLK_IDX][1];

    GPIO_config_pin(io_port, io_pin, GPIO_INPUT_FLOATING);

    for(uint8_t i = 0U; i < 8U; i++)
    {
        GPIO_write_pin(clk_port, clk_pin, 1U);
        _delay_us(2);
        GPIO_write_pin(clk_port, clk_pin, 0U);
        _delay_us(2);

        ret >>= 1U;

        if(GPIO_read_pin(io_port, io_pin))
        {
            ret |= (1U << 7U);
        }
    }

    return ret;
}

static void start(uint8_t type)
{
    reset();
    write_byte(type);
}

static void write(uint8_t reg, uint8_t value)
{
    start(reg);
    write_byte(value);
    stop();
}

static uint8_t read(uint8_t reg)
{
    uint8_t ret = 0u;
    start(reg);
    ret = read_byte();
    stop();
    return ret;
}

void DS1302_set(DS1302_datetime_t *config)
{
    if(config != NULL)
    {
        write(WRITE_SECONDS, get_value_to_store(DS1302_SECONDS, config->secs));
        write(WRITE_MINUTES, get_value_to_store(DS1302_MINUTES, config->min));
        write(WRITE_HOURS, get_value_to_store(DS1302_HOURS, config->hours));
    }
}

uint8_t DS1302_get_seconds(void)
{
    uint8_t ret = read(READ_SECONDS);

    ret = get_value_to_load(DS1302_SECONDS, ret);

    return ret;
}

uint8_t DS1302_get_minutes(void)
{
    uint8_t ret = read(READ_MINUTES);

    ret = get_value_to_load(DS1302_MINUTES, ret);

    return ret;
}

uint8_t DS1302_get_hours(void)
{
    uint8_t ret = read(READ_HOURS);

    ret = get_value_to_load(DS1302_HOURS, ret);

    return ret;
}

void DS1302_set_write_protection(bool val)
{
    if(val)
    {
        write(WRITE_WP, 1U << 7U);
    }
    else
    {
        write(WRITE_WP, 0U);
    }
}


int8_t DS1302_configure(const DS1302_config_t *config)
{
    if(config == NULL)
    {
        return -1;
    }

    memcpy(pins, config->pins, sizeof(config->pins));

    for(uint8_t i = 0; i < 3u; i++)
    {
        GPIO_config_pin(pins[i][0], pins[i][1], GPIO_OUTPUT_PUSH_PULL);
        GPIO_write_pin(pins[i][0], pins[i][1], 0U);
    }

    return 0;
}
