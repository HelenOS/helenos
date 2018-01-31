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
/** @addtogroup genarch
 * @{
 */
/**
 * @file
 * @brief Texas Instruments AM335x timer driver.
 */

#ifndef _KERN_AM335X_TIMER_H_
#define _KERN_AM335X_TIMER_H_

#include <genarch/drivers/am335x/timer_regs.h>

#define AM335x_DMTIMER0_BASE_ADDRESS    0x44E05000
#define AM335x_DMTIMER0_SIZE            4096
#define AM335x_DMTIMER0_IRQ             66

#define AM335x_DMTIMER2_BASE_ADDRESS    0x48040000
#define AM335x_DMTIMER2_SIZE            4096
#define AM335x_DMTIMER2_IRQ             68

#define AM335x_DMTIMER3_BASE_ADDRESS    0x48042000
#define AM335x_DMTIMER3_SIZE            4096
#define AM335x_DMTIMER3_IRQ             69

#define AM335x_DMTIMER4_BASE_ADDRESS    0x48044000
#define AM335x_DMTIMER4_SIZE            4096
#define AM335x_DMTIMER4_IRQ             92

#define AM335x_DMTIMER5_BASE_ADDRESS    0x48046000
#define AM335x_DMTIMER5_SIZE            4096
#define AM335x_DMTIMER5_IRQ             93

#define AM335x_DMTIMER6_BASE_ADDRESS    0x48048000
#define AM335x_DMTIMER6_SIZE            4096
#define AM335x_DMTIMER6_IRQ             94

#define AM335x_DMTIMER7_BASE_ADDRESS    0x4804A000
#define AM335x_DMTIMER7_SIZE            4096
#define AM335x_DMTIMER7_IRQ             95

typedef enum {
	DMTIMER0 = 0,
	DMTIMER1_1MS,
	DMTIMER2,
	DMTIMER3,
	DMTIMER4,
	DMTIMER5,
	DMTIMER6,
	DMTIMER7,

	TIMERS_MAX
} am335x_timer_id_t;

typedef struct am335x_timer {
	am335x_timer_regs_t *regs;
	am335x_timer_id_t id;
} am335x_timer_t;

extern errno_t am335x_timer_init(am335x_timer_t *timer, am335x_timer_id_t id,
    unsigned hz, unsigned srcclk_hz);
extern void am335x_timer_intr_ack(am335x_timer_t *timer);
extern void am335x_timer_reset(am335x_timer_t *timer);
extern void am335x_timer_start(am335x_timer_t *timer);
extern void am335x_timer_stop(am335x_timer_t *timer);


#endif

/**
 * @}
 */

