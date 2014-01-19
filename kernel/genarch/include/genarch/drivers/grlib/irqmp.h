/*
 * Copyright (c) 2013 Jakub Klama
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
 * @brief Gaisler GRLIB interrupt controller driver.
 */

#ifndef KERN_GRLIB_IRQMP_H_
#define KERN_GRLIB_IRQMP_H_

#include <typedefs.h>
#include <arch.h>

#define GRLIB_IRQMP_MASK_OFFSET   0x40
#define GRLIB_IRQMP_FORCE_OFFSET  0x80

/** IRQMP registers */
typedef struct {
	uint32_t level;
	uint32_t pending;
	uint32_t force;
	uint32_t clear;
	uint32_t mp_status;
	uint32_t broadcast;
} grlib_irqmp_regs_t;

/** LEON3 interrupt assignments */
enum grlib_irq_source {
	GRLIB_INT_AHBERROR = 1,
	GRLIB_INT_UART1    = 2,
	GRLIB_INT_PCIDMA   = 4,
	GRLIB_INT_CAN      = 5,
	GRLIB_INT_TIMER0   = 6,
	GRLIB_INT_TIMER1   = 7,
	GRLIB_INT_TIMER2   = 8,
	GRLIB_INT_TIMER3   = 9,
	GRLIB_INT_ETHERNET = 14
};

typedef struct {
	grlib_irqmp_regs_t *regs;
} grlib_irqmp_t;

extern void grlib_irqmp_init(grlib_irqmp_t *, bootinfo_t *);
extern int grlib_irqmp_inum_get(grlib_irqmp_t *);
extern void grlib_irqmp_clear(grlib_irqmp_t *, unsigned int);
extern void grlib_irqmp_mask(grlib_irqmp_t *, unsigned int);
extern void grlib_irqmp_unmask(grlib_irqmp_t *, unsigned int);

#endif

/** @}
 */
