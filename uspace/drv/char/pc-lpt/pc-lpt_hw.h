/*
 * Copyright (c) 2018 Jiri Svoboda
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

/** @addtogroup uspace_drv_pc_lpt
 * @{
 */
/** @file
 */

#ifndef PC_LPT_HW_H
#define PC_LPT_HW_H

#include <ddi.h>

/** PC parallel port registers */
typedef struct {
	/** Data out register */
	ioport8_t data;
	/** Status register */
	ioport8_t status;
	/** Control register */
	ioport8_t control;
} pc_lpt_regs_t;

/** Printer control register bits */
typedef enum {
	/** Strobe */
	ctl_strobe = 0,
	/** Auto linefeed */
	ctl_autofd = 1,
	/** -Init */
	ctl_ninit = 2,
	/** Select */
	ctl_select = 3,
	/** IRQ Enable */
	ctl_irq_enable = 4
} pc_lpt_ctl_bits_t;

/** Printer status register bits */
typedef enum {
	/** -Error */
	sts_nerror = 3,
	/** Select */
	sts_select = 4,
	/** Init */
	sts_paper_end = 5,
	/** -Ack */
	sts_nack = 6,
	/** -Busy */
	sts_nbusy = 7
} pc_lpt_sts_bits_t;

#endif

/** @}
 */
