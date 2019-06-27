/*
 * Copyright (c) 2019 Jiri Svoboda
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
