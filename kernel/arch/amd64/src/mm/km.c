/*
 * SPDX-FileCopyrightText: 2011 Jakub Jermar
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup kernel_amd64_mm
 * @{
 */

#include <arch/mm/km.h>
#include <mm/km.h>
#include <config.h>
#include <macros.h>
#include <stdbool.h>

void km_identity_arch_init(void)
{
	config.identity_base = KM_AMD64_IDENTITY_START;
	config.identity_size = KM_AMD64_IDENTITY_SIZE;
}

void km_non_identity_arch_init(void)
{
	km_non_identity_span_add(KM_AMD64_NON_IDENTITY_START,
	    KM_AMD64_NON_IDENTITY_SIZE);
}

bool km_is_non_identity_arch(uintptr_t addr)
{
	return iswithin(KM_AMD64_NON_IDENTITY_START,
	    KM_AMD64_NON_IDENTITY_SIZE, addr, 1);
}

/** @}
 */
