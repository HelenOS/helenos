/*
 * SPDX-FileCopyrightText: 2012 Maurizio Lombardi
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup cmos-rtc
 * @{
 */

#ifndef _CMOS_RTC_H_
#define _CMOS_RTC_H_

#define RTC_SEC           0x00
#define RTC_MIN           0x02
#define RTC_HOUR          0x04
#define RTC_DAY           0x07
#define RTC_MON           0x08
#define RTC_YEAR          0x09

#define RTC_STATUS_B      0x0B
#define RTC_B_24H         0x02 /* 24h mode */
#define RTC_B_BCD         0x04 /* BCD mode */
#define RTC_B_INH         0x80 /* Inhibit updates */

#define RTC_STATUS_D      0x0D
#define RTC_D_BATTERY_OK  0x80 /* Battery status */

#define RTC_STATUS_A      0x0A
#define RTC_A_UPDATE      0x80 /* Update in progress */
#define RTC_A_CLK_STOP    0x70 /* Stop the clock */

#endif

/**
 * @}
 */
