/*
 * Copyright (c) 2007 Petr Stepan
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

/** @addtogroup arm32
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
