/*
 * SPDX-FileCopyrightText: 2019 Jiri Svoboda
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

#include <assert.h>
#include <genarch/drivers/bcm2835/timer.h>
#include <time/clock.h>

void bcm2835_timer_start(bcm2835_timer_t *timer)
{
	assert(timer);
	/* Clear pending interrupt on channel 1 */
	timer->cs |= BCM2835_TIMER_CS_M1;
	/* Initialize compare value for match channel 1 */
	timer->c1 = timer->clo + (BCM2835_CLOCK_FREQ / HZ);
}

void bcm2835_timer_irq_ack(bcm2835_timer_t *timer)
{
	assert(timer);
	/* Clear pending interrupt on channel 1 */
	timer->cs |= BCM2835_TIMER_CS_M1;
	/* Reprogram compare value for match channel 1 */
	timer->c1 = timer->clo + (BCM2835_CLOCK_FREQ / HZ);
}

/**
 * @}
 */
