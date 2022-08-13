/*
 * SPDX-FileCopyrightText: 2008 Jakub Jermar
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup kernel_ia32
 * @{
 */
/** @file
 */

#include <arch/syscall.h>
#include <arch/cpu.h>
#include <arch/asm.h>
#include <arch/pm.h>
#include <stdint.h>

#ifndef PROCESSOR_i486

/** Enable & setup support for SYSENTER/SYSEXIT */
void syscall_setup_cpu(void)
{
	extern void sysenter_handler(void);

	/* set kernel mode CS selector */
	write_msr(IA32_MSR_SYSENTER_CS, GDT_SELECTOR(KTEXT_DES));
	/* set kernel mode entry point */
	write_msr(IA32_MSR_SYSENTER_EIP, (uint32_t) sysenter_handler);
}

#endif /* PROCESSOR_i486 */

/** @}
 */
