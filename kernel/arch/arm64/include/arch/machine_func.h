/*
 * Copyright (c) 2016 Petr Pavlu
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

/** @addtogroup kernel_arm64
 * @{
 */
/** @file
 * @brief Declarations of machine specific functions.
 *
 * These functions enable to differentiate more kinds of ARM platforms.
 */

#ifndef KERN_arm64_MACHINE_FUNC_H_
#define KERN_arm64_MACHINE_FUNC_H_

#include <arch/istate.h>
#include <typedefs.h>

struct arm_machine_ops {
	void (*machine_init)(void);
	void (*machine_irq_exception)(unsigned int, istate_t *);
	void (*machine_output_init)(void);
	void (*machine_input_init)(void);
	inr_t (*machine_enable_vtimer_irq)(void);
	size_t (*machine_get_irq_count)(void);
	const char *(*machine_get_platform_name)(void);
	void (*machine_early_uart_output)(char32_t);
};

extern void machine_ops_init(void);
extern void machine_init(void);
void machine_irq_exception(unsigned int, istate_t *);
extern void machine_output_init(void);
extern void machine_input_init(void);
extern inr_t machine_enable_vtimer_irq(void);
extern size_t machine_get_irq_count(void);
extern const char *machine_get_platform_name(void);
extern void machine_early_uart_output(char32_t);

#endif

/** @}
 */
