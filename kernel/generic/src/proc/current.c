/*
 * SPDX-FileCopyrightText: 2005 Jakub Jermar
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup kernel_generic_proc
 * @{
 */

/**
 * @file
 * @brief CURRENT structure functions.
 *
 * This file contains functions to manage the CURRENT structure.
 * The CURRENT structure exists at the base address of every kernel
 * stack and carries information about current settings
 * (e.g. current CPU, current thread, task and address space
 * and current preemption counter).
 */

#include <arch.h>
#include <assert.h>

/** Initialize CURRENT structure
 *
 * Initialize CURRENT structure passed as argument.
 *
 * @param the CURRENT structure to be initialized.
 *
 */
void current_initialize(current_t *the)
{
	the->preemption = 0;
	the->cpu = NULL;
	the->thread = NULL;
	the->task = NULL;
	the->as = NULL;
	the->magic = MAGIC;
}

/** Copy CURRENT structure
 *
 * Copy the source CURRENT structure to the destination CURRENT structure.
 *
 * @param src The source CURRENT structure.
 * @param dst The destination CURRENT structure.
 *
 */
_NO_TRACE void current_copy(current_t *src, current_t *dst)
{
	assert(src->magic == MAGIC);
	*dst = *src;
}

/** @}
 */
