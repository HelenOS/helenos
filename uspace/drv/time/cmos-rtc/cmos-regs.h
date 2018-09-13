/*
 * Copyright (c) 2012 Maurizio Lombardi
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * - Redistributions of source code must retain the above copyright
 *   notice, this list of conditions and the following disclaimer.
 * - Redistributions in binary form must reproduce the above copyright
 *   notice, this list of conditions and the following disclaimer in the
 *   documentation and/or other materials provided with the distribution.
 * - The name of the author may not be used to endorse or promote products
 *   derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
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
