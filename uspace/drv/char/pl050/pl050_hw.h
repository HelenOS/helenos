/*
 * Copyright (c) 2009 Vineeth Pillai
 * Copyright (c) 2014 Jiri Svoboda
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

/** @addtogroup pl050
 * @{
 */
/** @file ARM PrimeCell PS2 Keyboard/Mouse Interface (PL050) registers
 */

#ifndef PL050_HW_H
#define PL050_HW_H

#include <stdint.h>

typedef struct {
	/** Control register */
	uint8_t cr;
	/** Padding */
	uint8_t pad1[3];
	/** Status register */
	uint8_t stat;
	/** Padding */
	uint8_t pad5[3];
	/** Received data */
	uint8_t data;
	/** Padding */
	uint8_t pad9[3];
	/** Clock divisor */
	uint8_t clkdiv;
	/** Padding */
	uint8_t pad13[3];
	/** Interrupt status register */
	uint8_t ir;
	/** Padding */
	uint8_t pad17[3];
} kmi_regs_t;

typedef enum {
	/** 0 = PS2 mode, 1 = No line control bit mode */
	kmi_cr_type = 5,
	/** Enable receiver interrupt */
	kmi_cr_rxintr = 4,
	/** Enable transmitter interrupt */
	kmi_cr_txintr = 3,
	/** Enable PrimeCell KMI */
	kmi_cr_enable = 2,
	/** Force KMI data LOW */
	kmi_cr_forcedata = 1,
	/** Force KMI clock LOW */
	kmi_cr_forceclock = 0
} kmi_cr_bits_t;

typedef enum {
	kmi_stat_txempty = 6,
	kmi_stat_txbusy = 5,
	kmi_stat_rxfull = 4,
	kmi_stat_rxbusy = 3,
	kmi_stat_rxparity = 2,
	kmi_stat_clkin = 1,
	kmi_stat_datain = 0
} kmi_stat_bits_t;

#endif

/** @}
 */
