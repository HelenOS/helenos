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
 * @brief Definitions of machine specific functions.
 *
 * These functions enable to differentiate more kinds of ARM platforms.
 */

#include <arch/machine_func.h>
#include <arch/mach/hikey960/hikey960.h>
#include <arch/mach/virt/virt.h>

/** Pointer to machine_ops structure being used. */
static struct arm_machine_ops *machine_ops;

/** Initialize machine_ops pointer. */
void machine_ops_init(void)
{
#if defined(MACHINE_virt)
	machine_ops = &virt_machine_ops;
#elif defined(MACHINE_hikey960)
	machine_ops = &hikey960_machine_ops;
#else
#error Machine type not defined.
#endif
}

/** Perform machine-specific initialization. */
void machine_init(void)
{
	machine_ops->machine_init();
}

/** Interrupt exception handler.
 *
 * @param exc_no Interrupt exception number.
 * @param istate Saved processor state.
 */
void machine_irq_exception(unsigned int exc_no, istate_t *istate)
{
	machine_ops->machine_irq_exception(exc_no, istate);
}

/** Configure the output device. */
void machine_output_init(void)
{
	machine_ops->machine_output_init();
}

/** Configure the input device. */
void machine_input_init(void)
{
	machine_ops->machine_input_init();
}

/** Get IRQ number range used by machine. */
size_t machine_get_irq_count(void)
{
	return machine_ops->machine_get_irq_count();
}

/** Enable virtual timer interrupt and return its number. */
inr_t machine_enable_vtimer_irq(void)
{
	return machine_ops->machine_enable_vtimer_irq();
}

/** Get platform identifier. */
const char *machine_get_platform_name(void)
{
	return machine_ops->machine_get_platform_name();
}

/** Early debugging output. */
void machine_early_uart_output(char32_t c)
{
	if (machine_ops->machine_early_uart_output != NULL)
		machine_ops->machine_early_uart_output(c);
}

/** @}
 */
