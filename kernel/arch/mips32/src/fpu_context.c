/*
 * SPDX-FileCopyrightText: 2005 Jakub Vana
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup kernel_mips32
 * @{
 */
/** @file
 *
 */

#include <fpu_context.h>
#include <arch.h>
#include <arch/cp0.h>
#include <proc/thread.h>

void fpu_disable(void)
{
#ifdef CONFIG_FPU
	cp0_status_write(cp0_status_read() & ~cp0_status_fpu_bit);
#endif
}

void fpu_enable(void)
{
#ifdef CONFIG_FPU
	cp0_status_write(cp0_status_read() | cp0_status_fpu_bit);
#endif
}

void fpu_init(void)
{
	/* TODO: Zero all registers */
}

/** @}
 */
