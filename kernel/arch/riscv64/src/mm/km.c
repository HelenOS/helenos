/*
 * SPDX-FileCopyrightText: 2016 Martin Decky
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup kernel_riscv64_mm
 * @{
 */

#include <arch/mm/km.h>
#include <mm/km.h>
#include <stdbool.h>
#include <typedefs.h>
#include <macros.h>

void km_identity_arch_init(void)
{
	config.identity_base = KM_RISCV64_IDENTITY_START;
	config.identity_size = KM_RISCV64_IDENTITY_SIZE;
}

void km_non_identity_arch_init(void)
{
	km_non_identity_span_add(KM_RISCV64_NON_IDENTITY_START,
	    KM_RISCV64_NON_IDENTITY_SIZE);
}

bool km_is_non_identity_arch(uintptr_t addr)
{
	return iswithin(KM_RISCV64_NON_IDENTITY_START,
	    KM_RISCV64_NON_IDENTITY_SIZE, addr, 1);
}

/** @}
 */
