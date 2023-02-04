/*
 * Copyright (c) 2005 Jakub Jermar
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * - Redistributions of source code must retain the above copyright
 *   notice, this list of conditions and the following disclaimer.
 * - Redistributions in binary form must reproduce the above copyright
 *   notice, this list of conditions and the following disclaimer in the
 *   documentation and/or other materials provided with the distribution.
 * - The name of the author may not be used to endorse or promote products
 *   derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
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
	the->mutex_locks = 0;
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
