/*
 * SPDX-FileCopyrightText: 2011 Jakub Jermar
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup kernel_mips32_mm
 * @{
 */

#include <arch/mm/km.h>
#include <mm/km.h>
#include <config.h>
#include <typedefs.h>
#include <macros.h>

void km_identity_arch_init(void)
{
	config.identity_base = KM_MIPS32_KSEG0_START;
	config.identity_size = KM_MIPS32_KSEG0_SIZE;
}

void km_non_identity_arch_init(void)
{
	km_non_identity_span_add(KM_MIPS32_KSSEG_START, KM_MIPS32_KSSEG_SIZE);
	km_non_identity_span_add(KM_MIPS32_KSEG3_START, KM_MIPS32_KSEG3_SIZE);
}

bool km_is_non_identity_arch(uintptr_t addr)
{
	return iswithin(KM_MIPS32_KSSEG_START, KM_MIPS32_KSSEG_SIZE, addr, 1) ||
	    iswithin(KM_MIPS32_KSEG3_START, KM_MIPS32_KSEG3_SIZE, addr, 1);
}

/** @}
 */
