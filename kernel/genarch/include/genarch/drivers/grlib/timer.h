/*
 * Copyrihgt (c) 2013 Jakub Klama
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
 * @brief Gaisler GRLIB gptimer driver.
 */

#ifndef KERN_GRLIB_TIMER_H_
#define KERN_GRLIB_TIMER_H_

#include <typedefs.h>

/** GRLIB gptimer registers */
typedef struct {
	uint32_t scaler;
	uint32_t scaler_reload;
	uint32_t config;
	uint32_t latch_config;
	struct {
		uint32_t counter;
		uint32_t reload;
		uint32_t control;
		uint32_t latch;
	} timers[7];
} grlib_timer_t;

struct grlib_timer_config_t {
	unsigned int : 20;
	unsigned int es : 1;
	unsigned int el : 1;
	unsigned int ee : 1;
	unsigned int df : 1;
	unsigned int si : 1;
	unsigned int irq : 5;
	unsigned int timers : 2;
};

struct grlib_timer_control_t {
	unsigned int : 25;
	unsigned int dh : 1;
	unsigned int ch : 1;
	unsigned int ip : 1;
	unsigned int ie : 1;
	unsigned int ld : 1;
	unsigned int rs : 1;
	unsigned int en : 1;
};

#endif

/** @}
 */
