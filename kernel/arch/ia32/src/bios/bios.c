/*
 * SPDX-FileCopyrightText: 2005 Jakub Jermar
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup kernel_ia32
 * @{
 */
/** @file
 */

#include <arch/bios/bios.h>
#include <typedefs.h>

#define BIOS_EBDA_PTR  0x40eU

uintptr_t ebda = 0;

void bios_init(void)
{
	/* Copy the EBDA address out from BIOS Data Area */
	ebda = *((uint16_t *) BIOS_EBDA_PTR) * 0x10U;
}

/** @}
 */
