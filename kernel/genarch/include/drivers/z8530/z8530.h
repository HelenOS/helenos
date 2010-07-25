/*
 * Copyright (c) 2009 Jakub Jermar
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
 * @brief Headers for Zilog 8530 serial controller.
 */

#ifndef KERN_Z8530_H_
#define KERN_Z8530_H_

#include <ddi/irq.h>
#include <typedefs.h>
#include <console/chardev.h>

#define WR0   0
#define WR1   1
#define WR2   2
#define WR3   3
#define WR4   4
#define WR5   5
#define WR6   6
#define WR7   7
#define WR8   8
#define WR9   9
#define WR10  10
#define WR11  11
#define WR12  12
#define WR13  13
#define WR14  14
#define WR15  15

#define RR0   0
#define RR1   1
#define RR2   2
#define RR3   3
#define RR8   8
#define RR10  10
#define RR12  12
#define RR13  13
#define RR14  14
#define RR15  15

/** Reset pending TX interrupt. */
#define WR0_TX_IP_RST  (0x5 << 3)
#define WR0_ERR_RST    (0x6 << 3)

/** Receive Interrupts Disabled. */
#define WR1_RID     (0x0 << 3)
/** Receive Interrupt on First Character or Special Condition. */
#define WR1_RIFCSC  (0x1 << 3)
/** Interrupt on All Receive Characters or Special Conditions. */
#define WR1_IARCSC  (0x2 << 3)
/** Receive Interrupt on Special Condition. */
#define WR1_RISC    (0x3 << 3)
/** Parity Is Special Condition. */
#define WR1_PISC    (0x1 << 2)

/** Rx Enable. */
#define WR3_RX_ENABLE  (0x1 << 0)
/** 8-bits per character. */
#define WR3_RX8BITSCH  (0x3 << 6)

/** Master Interrupt Enable. */
#define WR9_MIE  (0x1 << 3)

/** Receive Character Available. */
#define RR0_RCA  (0x1 << 0)

/** z8530's registers. */
typedef struct {
	union {
		ioport8_t ctl_b;
		ioport8_t status_b;
	} __attribute__ ((packed));
	uint8_t pad1;
	ioport8_t data_b;
	uint8_t pad2;
	union {
		ioport8_t ctl_a;
		ioport8_t status_a;
	} __attribute__ ((packed));
	uint8_t pad3;
	ioport8_t data_a;
} __attribute__ ((packed)) z8530_t;

/** Structure representing the z8530 device. */
typedef struct {
	irq_t irq;
	z8530_t *z8530;
	indev_t *kbrdin;
} z8530_instance_t;

extern z8530_instance_t *z8530_init(z8530_t *, inr_t, cir_t, void *);
extern void z8530_wire(z8530_instance_t *, indev_t *);

#endif

/** @}
 */
