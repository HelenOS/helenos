/*
 * Copyright (C) 2006 Jakub Jermar
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

/** @addtogroup sparc64	
 * @{
 */
/** @file
 */

#ifndef KERN_sparc64_Z8530_H_
#define KERN_sparc64_Z8530_H_

#include <arch/types.h>
#include <typedefs.h>
#include <arch/drivers/kbd.h>

#define Z8530_CHAN_A	4
#define Z8530_CHAN_B	0

#define WR0	0
#define WR1	1
#define WR2	2
#define WR3	3
#define WR4	4
#define WR5	5
#define WR6	6
#define WR7	7
#define WR8	8
#define WR9	9
#define WR10	10
#define WR11	11
#define WR12	12
#define WR13	13
#define WR14	14
#define WR15	15

#define RR0	0
#define RR1	1
#define RR2	2
#define RR3	3
#define RR8	8
#define RR10	10
#define RR12	12
#define RR13	13
#define RR14	14
#define RR15	15

/* Write Register 0 */
#define WR0_ERR_RST	(0x6<<3)

/* Write Register 1 */
#define WR1_RID		(0x0<<3)	/** Receive Interrupts Disabled. */
#define WR1_RIFCSC	(0x1<<3)	/** Receive Interrupt on First Character or Special Condition. */
#define WR1_IARCSC	(0x2<<3)	/** Interrupt on All Receive Characters or Special Conditions. */
#define WR1_RISC	(0x3<<3)	/** Receive Interrupt on Special Condition. */
#define WR1_PISC	(0x1<<2)	/** Parity Is Special Condition. */

/* Write Register 3 */
#define WR3_RX_ENABLE	(0x1<<0)	/** Rx Enable. */
#define WR3_RX8BITSCH	(0x3<<6)	/** 8-bits per character. */

/* Write Register 9 */
#define WR9_MIE		(0x1<<3)	/** Master Interrupt Enable. */

/* Read Register 0 */
#define RR0_RCA		(0x1<<0)	/** Receive Character Available. */

static inline void z8530_write(index_t chan, uint8_t reg, uint8_t val)
{
	/*
	 * Registers 8-15 will automatically issue the Point High
	 * command as their bit 3 is 1.
	 */
	kbd_virt_address[WR0+chan] = reg;	/* select register */
	kbd_virt_address[WR0+chan] = val;	/* write value */
}

static inline void z8530_write_a(uint8_t reg, uint8_t val)
{
	z8530_write(Z8530_CHAN_A, reg, val);
}
static inline void z8530_write_b(uint8_t reg, uint8_t val)
{
	z8530_write(Z8530_CHAN_B, reg, val);
}

static inline uint8_t z8530_read(index_t chan, uint8_t reg) 
{
	/*
	 * Registers 8-15 will automatically issue the Point High
	 * command as their bit 3 is 1.
	 */
	kbd_virt_address[WR0+chan] = reg;	/* select register */
	return kbd_virt_address[WR0+chan];
}

static inline uint8_t z8530_read_a(uint8_t reg)
{
	return z8530_read(Z8530_CHAN_A, reg);
}
static inline uint8_t z8530_read_b(uint8_t reg)
{
	return z8530_read(Z8530_CHAN_B, reg);
}

#endif

/** @}
 */
