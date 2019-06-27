/*
 * Copyright (c) 2013 Beniamino Galvani
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
/** @addtogroup kernel_genarch
 * @{
 */
/**
 * @file
 * @brief Broadcom BCM2835 system timer driver.
 */

#ifndef KERN_BCM2835_TIMER_H_

#include <assert.h>
#include <typedefs.h>
#include <mm/km.h>

#define BCM2835_TIMER_ADDR 0x20003000
#define BCM2835_CLOCK_FREQ 1000000

typedef struct {
	/** System Timer Control/Status */
	ioport32_t cs;
#define BCM2835_TIMER_CS_M0 (1 << 0)
#define BCM2835_TIMER_CS_M1 (1 << 1)
#define BCM2835_TIMER_CS_M2 (1 << 2)
#define BCM2835_TIMER_CS_M3 (1 << 3)
	/** System Timer Counter Lower 32 bits */
	ioport32_t clo;
	/** System Timer Counter Higher 32 bits */
	ioport32_t chi;
	/** System Timer Compare 0 */
	ioport32_t c0;
	/** System Timer Compare 1 */
	ioport32_t c1;
	/** System Timer Compare 2 */
	ioport32_t c2;
	/** System Timer Compare 3 */
	ioport32_t c3;
} bcm2835_timer_t;

extern void bcm2835_timer_start(bcm2835_timer_t *);
extern void bcm2835_timer_irq_ack(bcm2835_timer_t *);

#endif /* KERN_BCM2835_TIMER_H_ */

/**
 * @}
 */
