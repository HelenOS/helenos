/*
 * SPDX-FileCopyrightText: 2006 Jakub Jermar
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/**
 * @file
 * @brief	MMU trap handlers that do not fit into the trap table.
 */

.register %g2, #scratch
.register %g3, #scratch

.text

#include <abi/asmtool.h>
#include <arch/trap/sun4v/mmu.h>
#include <arch/trap/trap_table.h>
#include <arch/sun4v/regdef.h>

/*
 * Install mapping for the kernel. The mapping obeys this formula:
 * 	virtual address = real address + start of physical memory
 *
 * The routine expects the following values of registers:
 *	 %g1	virtual address that has caused the miss
 */
SYMBOL(install_identity_mapping)
	/* output registers mustn't be clobbered during the hypercall, SAVE is too risky */
	mov %o0, %g3
	mov %o1, %g4
	mov %o2, %g5
	mov %o3, %g6

	/* install mapping for kernel */
	mov %g1, %o0
	set 0, %o1					! set context
	setx kernel_8k_tlb_data_template, %g1, %g2	! g2 <= template of TTE Data
	ldx [%g2], %g2					! read the TTE Data template
	add %g2, %o0, %o2				! template + VA = TTE Data entry
	set MMU_FLAG_DTLB, %o3				! map in DTLB only
	ta MMU_MAP_ADDR

	/* restore output registers */
	mov %g6, %o3
	mov %g5, %o2
	mov %g4, %o1
	mov %g3, %o0

	retry
