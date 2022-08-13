/*
 * SPDX-FileCopyrightText: 2016 Petr Pavlu
 *
 * SPDX-License-Identifier: BSD-3-Clause
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
