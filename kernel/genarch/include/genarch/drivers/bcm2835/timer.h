/*
 * SPDX-FileCopyrightText: 2013 Beniamino Galvani
 *
 * SPDX-License-Identifier: BSD-3-Clause
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
