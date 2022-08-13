/*
 * SPDX-FileCopyrightText: 2011 Jakub Jermar
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup kernel_abs32le_mm
 * @{
 */

#include <arch/mm/km.h>
#include <typedefs.h>

void km_identity_arch_init(void)
{
}

void km_non_identity_arch_init(void)
{
}

bool km_is_non_identity_arch(uintptr_t addr)
{
	return false;
}

/** @}
 */
