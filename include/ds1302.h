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

/*!
 *
 * \addtogroup ds1302_data_types
 * \ingroup ds1302
 * \brief DS1302 data types
 */
/*@{*/
#define DS1302_SECONDS          (0u)
#define DS1302_MINUTES          (1u)
#define DS1302_HOURS_24H        (2u)
#define DS1302_HOURS_12H        (3u)
#define DS1302_WEEKDAY          (4u)
#define DS1302_DATE             (5u)
#define DS1302_MONTH            (6u)
#define DS1302_YEAR             (7u)
#define DS1302_FORMAT           (8u)
#define DS1302_AM_PM            (9U)
/*@}*/

/*!
 * \brief Aggregate of DS1302 data types \ref ds1302_data_types
 */
typedef struct
{
    uint8_t secs; /*!< Seconds */
    uint8_t min; /*!< Minutes */
    uint8_t hours; /*!< Hours */
    uint8_t weekday; /*!< Day of the week */
    uint8_t date; /*!< Day of the month */
    uint8_t month; /*!< Month */
    uint8_t year; /*!< Year */
    bool is_12h_mode; /*!< 24h/12h mode */
    bool is_pm; /*!< AM/PM in case 12h mode*/
} DS1302_datetime_t;

/*!
 * \todo (DB) refactor below getters into one function and rename it into something like 
 * DS1302_load_data(type)
 * */

/*!
 * \brief Gets seconds
 *
 * \returns Seconds
 */
uint8_t DS1302_get_seconds(void);

/*!
 * \brief Gets minutes
 *
 * \returns Minutes
 */
uint8_t DS1302_get_minutes(void);

/*!
 * \brief Gets hours
 *
 * \returns Hours
 */
uint8_t DS1302_get_hours(bool is_12h_mode);

/*!
 * \brief Gets minimum allowed setting of the data type
 *
 * \param type type of data
 *
 * \returns Minumum value
 */
uint8_t DS1302_get_range_minimum(uint8_t type);

/*!
 * \brief Gets maximum allowed setting of the data type
 *
 * \param type type of data
 *
 * \note in case data type \ref DS1302_DATE use \ref DS1302_get_date_range_maximum,
 * as \ref DS1302_DATE is not allowed
 *
 * \returns Maximum value
 */
uint8_t DS1302_get_range_maximum(uint8_t type);

/*!
 * \brief Gets maximum allowed setting for the day of the month \ref DS1302_DATE
 *
 * \param year year as input to calculate proper maximum value
 * \param month month as input to calculate proper maximum value
 *
 * \returns Maximum value of the day of the month
 */
uint8_t DS1302_get_date_range_maximum(uint8_t year, uint8_t month);

/*!
 * \brief Enables/disables write protection of the DS1302
 *
 * \param val write protection setting, true enables write protection
 */
void DS1302_set_write_protection(bool val);

/*! \todo (DB) change name of the function to DS1302_load */
/*!
 * \brief Retrieves aggregate with all DS1302 data types
 *
 * \param config storage for the retrieved data
 */
void DS1302_get(DS1302_datetime_t *config);

/*! \todo (DB) change name of the function to DS1302_store */
/*!
 * \brief Setups aggregate with all DS1302 data types
 *
 * \param config storage for data to be stored
 */
void DS1302_set(const DS1302_datetime_t *config);

/*!
 * \brief Configures DS1302 device
 */
void DS1302_configure(void);

/*@}*/
#endif
