/*!
 * \file
 * \brief DS1302 header file
 * \author Dawid Babula
 * \email dbabula@adventurous.pl
 *
 * \par Copyright (C) Dawid Babula, 2019
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

#ifndef DS1302_H
#define DS1302_H

/*!
 *
 * \addtogroup ds1302
 * \ingroup drivers
 * \brief DS1302 implementation
 */

/*@{*/
#include <stdint.h>
#include <stdbool.h>

#define DS1302_CLK_IDX              (0U)
#define DS1302_IO_IDX               (1U)
#define DS1302_CE_IDX               (2U)

typedef struct
{
    uint8_t pins[3][2];
} DS1302_config_t;

typedef struct
{
    uint8_t secs;
    uint8_t min;
    uint8_t hours;
    uint8_t weekday;
    uint8_t date;
    uint8_t month;
    uint8_t year;
    uint8_t format;
} DS1302_datetime_t;

uint8_t DS1302_get_seconds(void);
uint8_t DS1302_get_minutes(void);
uint8_t DS1302_get_hours(void);
void DS1302_set_write_protection(bool val);
void DS1302_get(DS1302_datetime_t *config);
void DS1302_set(const DS1302_datetime_t *config);
void DS1302_configure(const DS1302_config_t *config);

/*@}*/
#endif
