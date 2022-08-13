/*
 * SPDX-FileCopyrightText: 2016 Jakub Jermar
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup kernel_amd64
 * @{
 */
/** @file
 */

#include <arch/kseg.h>
#include <arch/asm.h>
#include <panic.h>
#include <arch/kseg_struct.h>
#include <stdlib.h>

/**
 * Allocate and initialize a per-CPU structure to be accessible via the
 * GS_KERNEL segment register.
 */
void kseg_init(void)
{
	kseg_t *kseg;

	kseg = (kseg_t *) malloc(sizeof(kseg_t));
	if (!kseg)
		panic("Cannot allocate kseg.");

	kseg->ustack_rsp = 0;
	kseg->kstack_rsp = 0;
	kseg->fsbase = read_msr(AMD_MSR_FS);

	write_msr(AMD_MSR_GS_KERNEL, (uintptr_t) kseg);
}

/** @}
 */
