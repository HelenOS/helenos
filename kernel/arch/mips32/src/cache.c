/*
 * SPDX-FileCopyrightText: 2003-2004 Jakub Jermar
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup kernel_mips32
 * @{
 */
/** @file
 */

#include <arch/cache.h>
#include <arch/exception.h>
#include <panic.h>

void cache_error(istate_t *istate)
{
	panic("cache_error exception (epc=%p).", (void *) istate->epc);
}

/** @}
 */
