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

#define DEBUG_APP_ID "DS13"
#define DEBUG_ENABLED   DEBUG_DS1302_ENABLED
#define DEBUG_LEVEL     DEBUG_DS1302_LEVEL

#include "ds1302.h"
#include "hardware.h"
#include "gpio.h"
#include <util/delay.h>
#include <stddef.h>
#include <string.h>
#include <limits.h>
#include "debug.h"

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

#define WRITE_PROTECTION_MASK   (0x80u)
#define HOURS_UNIT_MASK         (0x1Fu)
#define WEEKDAY_UNIT_MASK       (0x07u)
#define OTHER_UNIT_MASK         (0x0Fu)

#define SEC_MIN_TENS_MASK       (0x70u)
#define DATE_TENS_MASK          (0x30u)
#define MONTH_TENS_MASK         (0x10u)
#define YEAR_TENS_MASK          (0xF0u)

#define TENS_SHIFT              (4u)

#define UNIT_FACTOR             (1u)
#define TENS_FACTOR             (10u)

#define CLK_DELAY               (2u)
#define MSB_SHIFT               (7u)

typedef struct
{
    uint8_t min;
    uint8_t max;
} DS1302_range_t;

static const DS1302_range_t ranges[7] PROGMEM =
{
    [DS1302_SECONDS]    = { .min = 0u, .max = 59u },
    [DS1302_MINUTES]    = { .min = 0u, .max = 59u },
    [DS1302_HOURS]      = { .min = 0u, .max = 23u },
    [DS1302_WEEKDAY]    = { .min = 1u, .max = 7u  },
    [DS1302_DATE]       = { .min = 1u, .max = 31u },
    [DS1302_MONTH]      = { .min = 1u, .max = 12u },
    [DS1302_YEAR]       = { .min = 0u, .max = 99u }
};

static inline bool is_leap_year(uint8_t year)
{
    if((year % 4u) != 0u)
    {
        return false;
    }
    else if((year % 100u) != 0u)
    {
        return true;
    }
    else if((year % 400u) != 0u)
    {
        return false;
    }
    else
    {
        return true;
    }
}

static bool is_get_range_type_valid(uint8_t type)
{
    switch(type)
    {
        case DS1302_SECONDS:
        case DS1302_MINUTES:
        case DS1302_HOURS:
        case DS1302_WEEKDAY:
        case DS1302_DATE:
        case DS1302_MONTH:
        case DS1302_YEAR:
            return true;
        default:
            return false;
    }
}

static uint8_t get_value_to_store(uint8_t entry, uint8_t val)
{
    switch(entry)
    {
        case DS1302_SECONDS:
        case DS1302_MINUTES:
            return ((((val / TENS_FACTOR) << TENS_SHIFT) & SEC_MIN_TENS_MASK) |
                    (val % TENS_FACTOR));
        case DS1302_HOURS:
            return (val & HOURS_UNIT_MASK);
        case DS1302_WEEKDAY:
            return (val & WEEKDAY_UNIT_MASK);
        case DS1302_DATE:
            return (((val / TENS_FACTOR) << TENS_SHIFT) & DATE_TENS_MASK) |
                (val % TENS_FACTOR);
        case DS1302_MONTH:
            return (((val / TENS_FACTOR) << TENS_SHIFT) & MONTH_TENS_MASK) |
                (val % TENS_FACTOR);
        case DS1302_YEAR:
            return (((val / TENS_FACTOR) << TENS_SHIFT) & YEAR_TENS_MASK) |
                (val % TENS_FACTOR);
        default:
            ASSERT(false);
            break;

    }

    return 0;
}

static uint8_t get_value_to_load(uint8_t entry, uint8_t val)
{
    switch(entry)
    {
        case DS1302_SECONDS:
        case DS1302_MINUTES:
            return (val & OTHER_UNIT_MASK)*UNIT_FACTOR +
                ((val & SEC_MIN_TENS_MASK) >> TENS_SHIFT) * TENS_FACTOR;
        case DS1302_HOURS:
            return (val & HOURS_UNIT_MASK)*UNIT_FACTOR;;
        case DS1302_WEEKDAY:
            return (val & WEEKDAY_UNIT_MASK)*UNIT_FACTOR;
        case DS1302_DATE:
            return (val & OTHER_UNIT_MASK)*UNIT_FACTOR +
                ((val & DATE_TENS_MASK) >> TENS_SHIFT) * TENS_FACTOR;
        case DS1302_MONTH:
            return (val & OTHER_UNIT_MASK)*UNIT_FACTOR +
                ((val & MONTH_TENS_MASK) >> TENS_SHIFT) * TENS_FACTOR;
        case DS1302_YEAR:
            return (val & OTHER_UNIT_MASK)*UNIT_FACTOR +
                ((val & YEAR_TENS_MASK) >> TENS_SHIFT) * TENS_FACTOR;
        default:
            ASSERT(false);
            break;
    }

    return 0;
}

static inline void stop(void)
{
    GPIO_write_pin(GPIO_CHANNEL_RTC_CE, false);
    GPIO_write_pin(GPIO_CHANNEL_RTC_CLK, false);
}

static inline void reset(void)
{
    stop();

    GPIO_write_pin(GPIO_CHANNEL_RTC_CE, true);
}

static void write_byte(uint8_t data)
{
    uint8_t tmp = data;
    GPIO_config_pin(GPIO_CHANNEL_RTC_IO, GPIO_OUTPUT_PUSH_PULL);

    for(uint8_t i = 0u; i < CHAR_BIT; i++)
    {
        if((tmp & 0x01) != 0U)
        {
            GPIO_write_pin(GPIO_CHANNEL_RTC_IO, true);
        }
        else
        {
            GPIO_write_pin(GPIO_CHANNEL_RTC_IO, false);
        }

        GPIO_write_pin(GPIO_CHANNEL_RTC_CLK, false);
        _delay_us(CLK_DELAY);
        GPIO_write_pin(GPIO_CHANNEL_RTC_CLK, true);
        _delay_us(CLK_DELAY);

        tmp >>= 1U;
    }
}

static uint8_t read_byte(void)
{
    uint8_t ret = 0;

    GPIO_config_pin(GPIO_CHANNEL_RTC_IO, GPIO_INPUT_FLOATING);

    for(uint8_t i = 0U; i < 8U; i++)
    {
        GPIO_write_pin(GPIO_CHANNEL_RTC_CLK, true);
        _delay_us(CLK_DELAY);
        GPIO_write_pin(GPIO_CHANNEL_RTC_CLK, false);
        _delay_us(CLK_DELAY);

        ret >>= 1U;

        if(GPIO_read_pin(GPIO_CHANNEL_RTC_IO))
        {
            ret |= (1U << MSB_SHIFT);
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

void DS1302_get(DS1302_datetime_t *config)
{
    if(config != NULL)
    {
        config->year = get_value_to_load(DS1302_YEAR, read(READ_YEAR));
        config->month = get_value_to_load(DS1302_MONTH, read(READ_MONTH));
        config->date = get_value_to_load(DS1302_DATE, read(READ_DATE));
        config->weekday = get_value_to_load(DS1302_WEEKDAY, read(READ_WEEKDAY));
        config->hours = get_value_to_load(DS1302_HOURS, read(READ_HOURS));
        config->min = get_value_to_load(DS1302_MINUTES, read(READ_MINUTES));
        config->secs = get_value_to_load(DS1302_SECONDS, read(READ_SECONDS));
    }
}

void DS1302_set(const DS1302_datetime_t *config)
{
    if(config != NULL)
    {
        write(WRITE_YEAR, get_value_to_store(DS1302_YEAR, config->year));
        write(WRITE_MONTH, get_value_to_store(DS1302_MONTH, config->month));
        write(WRITE_DATE, get_value_to_store(DS1302_DATE, config->date));
        write(WRITE_WEEKDAY, get_value_to_store(DS1302_WEEKDAY, config->weekday));
        write(WRITE_HOURS, get_value_to_store(DS1302_HOURS, config->hours));
        write(WRITE_MINUTES, get_value_to_store(DS1302_MINUTES, config->min));
        write(WRITE_SECONDS, get_value_to_store(DS1302_SECONDS, config->secs));
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

uint8_t DS1302_get_range_minimum(uint8_t type)
{
    ASSERT(is_get_range_type_valid(type));
    return pgm_read_byte(&ranges[type].min);
}

uint8_t DS1302_get_range_maximum(uint8_t type)
{
    ASSERT(is_get_range_type_valid(type) && (type != DS1302_DATE));
    return pgm_read_byte(&ranges[type].max);
}

uint8_t DS1302_get_date_range_maximum(uint8_t year, uint8_t month)
{
    const bool is_leap = is_leap_year(year);

    switch(month)
    {
        case 1:
        case 3:
        case 5:
        case 7:
        case 8:
        case 10:
        case 12:
            return 31u;
        case 4:
        case 6:
        case 9:
        case 11:
            return 30u;
        case 2:
            return (is_leap ? 29u : 28u);
        default:
            ASSERT(false);
            return 0u;
    }
}

void DS1302_set_write_protection(bool val)
{
    if(val)
    {
        write(WRITE_WP, WRITE_PROTECTION_MASK);
    }
    else
    {
        write(WRITE_WP, 0U);
    }
}


void DS1302_configure(void)
{
    /* is going to be implemented later */
}
