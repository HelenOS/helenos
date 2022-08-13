/*
 * SPDX-FileCopyrightText: 2008 Jiri Svoboda
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup kernel_sync
 * @{
 */

/**
 * @file
 * @brief	Self-modifying code barriers.
 */

#include <arch.h>
#include <macros.h>
#include <errno.h>
#include <barrier.h>
#include <synch/smc.h>
#include <mm/as.h>

sys_errno_t sys_smc_coherence(uintptr_t va, size_t size)
{
	if (overlaps(va, size, (uintptr_t) NULL, PAGE_SIZE))
		return EINVAL;

	if (!KERNEL_ADDRESS_SPACE_SHADOWED) {
		if (overlaps(va, size, KERNEL_ADDRESS_SPACE_START,
		    KERNEL_ADDRESS_SPACE_END - KERNEL_ADDRESS_SPACE_START))
			return EINVAL;
	}

	smc_coherence((void *) va, size);
	return 0;
}

/** @}
 */
