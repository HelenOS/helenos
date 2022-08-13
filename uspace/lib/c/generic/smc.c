/*
 * SPDX-FileCopyrightText: 2008 Jiri Svoboda
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup libc
 * @{
 */
/** @file
 */

#include <libc.h>
#include <stddef.h>
#include <smc.h>

errno_t smc_coherence(void *address, size_t size)
{
	return (errno_t) __SYSCALL2(SYS_SMC_COHERENCE, (sysarg_t) address,
	    (sysarg_t) size);
}

/** @}
 */
