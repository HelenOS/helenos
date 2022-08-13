/*
 * SPDX-FileCopyrightText: 2006 Ondrej Palkovsky
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup kernel_amd64
 * @{
 */
/** @file
 */

#include <syscall/syscall.h>
#include <arch/syscall.h>
#include <panic.h>
#include <arch/cpu.h>
#include <arch/pm.h>
#include <arch/asm.h>

extern void syscall_entry(void);

/** Enable & setup support for SYSCALL/SYSRET */
void syscall_setup_cpu(void)
{
	/* Enable SYSCALL/SYSRET */
	write_msr(AMD_MSR_EFER, read_msr(AMD_MSR_EFER) | AMD_SCE);

	/* Setup syscall entry address */

	/*
	 * This is _mess_ - the 64-bit CS is argument + 16,
	 * the SS is argument + 8. The order is:
	 * +0(KDATA_DES), +8(UDATA_DES), +16(UTEXT_DES)
	 */
	write_msr(AMD_MSR_STAR,
	    ((uint64_t) (GDT_SELECTOR(KDATA_DES) | PL_USER) << 48) |
	    ((uint64_t) (GDT_SELECTOR(KTEXT_DES) | PL_KERNEL) << 32));
	write_msr(AMD_MSR_LSTAR, (uint64_t)syscall_entry);
	/*
	 * Mask RFLAGS on syscall
	 * - disable interrupts, until we exchange the stack register
	 *   (mask the IF bit)
	 * - clear DF so that the string instructions operate in
	 *   the right direction
	 * - clear NT to prevent a #GP should the flag proliferate to an IRET
	 * - clear TF to prevent an immediate #DB if TF is set
	 */
	write_msr(AMD_MSR_SFMASK,
	    RFLAGS_IF | RFLAGS_DF | RFLAGS_NT | RFLAGS_TF);
}

/** @}
 */
