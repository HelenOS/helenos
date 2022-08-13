/*
 * SPDX-FileCopyrightText: 2007 Petr Stepan
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup kernel_arm32
 * @{
 */
/** @file
 *  @brief Interrupts controlling routines.
 */

#include <arch/asm.h>
#include <arch/regutils.h>
#include <arch/machine_func.h>
#include <ddi/irq.h>
#include <interrupt.h>

/** Disable interrupts.
 *
 * @return Old interrupt priority level.
 */
ipl_t interrupts_disable(void)
{
	ipl_t ipl = current_status_reg_read();

	current_status_reg_control_write(STATUS_REG_IRQ_DISABLED_BIT | ipl);

	return ipl;
}

/** Enable interrupts.
 *
 * @return Old interrupt priority level.
 */
ipl_t interrupts_enable(void)
{
	ipl_t ipl = current_status_reg_read();

	current_status_reg_control_write(ipl & ~STATUS_REG_IRQ_DISABLED_BIT);

	return ipl;
}

/** Restore interrupt priority level.
 *
 * @param ipl Saved interrupt priority level.
 */
void interrupts_restore(ipl_t ipl)
{
	current_status_reg_control_write(
	    (current_status_reg_read() & ~STATUS_REG_IRQ_DISABLED_BIT) |
	    (ipl & STATUS_REG_IRQ_DISABLED_BIT));
}

/** Read interrupt priority level.
 *
 * @return Current interrupt priority level.
 */
ipl_t interrupts_read(void)
{
	return current_status_reg_read();
}

/** Check interrupts state.
 *
 * @return True if interrupts are disabled.
 *
 */
bool interrupts_disabled(void)
{
	return current_status_reg_read() & STATUS_REG_IRQ_DISABLED_BIT;
}

/** Initialize basic tables for exception dispatching
 * and starts the timer.
 */
void interrupt_init(void)
{
	size_t irq_count;

	irq_count = machine_get_irq_count();
	irq_init(irq_count, irq_count);

	machine_timer_irq_start();
}

/** @}
 */
