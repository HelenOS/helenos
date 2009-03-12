/*
 * Copyright (c) 2007 Michal Kebrt
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

/** @addtogroup arm32gxemul GXemul
 *  @brief GXemul machine specific parts.
 *  @ingroup arm32
 * @{
 */
/** @file
 *  @brief GXemul peripheries drivers declarations.
 */

#ifndef KERN_arm32_GXEMUL_H_
#define KERN_arm32_GXEMUL_H_

/** Last interrupt number (beginning from 0) whose status is probed
 * from interrupt controller
 */
#define GXEMUL_IRQC_MAX_IRQ  8
#define GXEMUL_KBD_IRQ       2
#define GXEMUL_TIMER_IRQ     4

/** Timer frequency */
#define GXEMUL_TIMER_FREQ  100

#define GXEMUL_KBD_ADDRESS   0x10000000
#define GXEMUL_MP_ADDRESS    0x11000000
#define GXEMUL_FB_ADDRESS    0x12000000
#define GXEMUL_RTC_ADDRESS   0x15000000
#define GXEMUL_IRQC_ADDRESS  0x16000000

extern void *gxemul_kbd;
extern void *gxemul_rtc;
extern void *gxemul_irqc;

#define GXEMUL_HALT_OFFSET        0x010
#define GXEMUL_RTC_FREQ_OFFSET    0x100
#define GXEMUL_MP_MEMSIZE_OFFSET  0x090
#define GXEMUL_RTC_ACK_OFFSET     0x110

extern void gxemul_init(void);

#endif

/** @}
 */
