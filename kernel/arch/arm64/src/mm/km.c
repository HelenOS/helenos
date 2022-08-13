/*
 * SPDX-FileCopyrightText: 2015 Petr Pavlu
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup kernel_arm64_mm
 * @{
 */

#include <arch/mm/km.h>
#include <config.h>
#include <macros.h>
#include <mm/km.h>
#include <typedefs.h>

void km_identity_arch_init(void)
{
	config.identity_base = KM_ARM64_IDENTITY_START;
	config.identity_size = KM_ARM64_IDENTITY_SIZE;
}

void km_non_identity_arch_init(void)
{
	km_non_identity_span_add(KM_ARM64_NON_IDENTITY_START,
	    KM_ARM64_NON_IDENTITY_SIZE);
}

bool km_is_non_identity_arch(uintptr_t addr)
{
	return iswithin(KM_ARM64_NON_IDENTITY_START, KM_ARM64_NON_IDENTITY_SIZE,
	    addr, 1);
}

/** @}
 */
