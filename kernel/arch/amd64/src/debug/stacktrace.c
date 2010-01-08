/*
 * Copyright (c) 2010 Jakub Jermar
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

/** @addtogroup amd64
 * @{
 */
/** @file
 */

#include <stacktrace.h>
#include <syscall/copy.h>
#include <arch/types.h>
#include <typedefs.h>

#define FRAME_OFFSET_FP_PREV	0
#define FRAME_OFFSET_RA		1

bool kernel_frame_pointer_validate(uintptr_t fp)
{
	return fp != 0;
}

bool kernel_frame_pointer_prev(uintptr_t fp, uintptr_t *prev)
{
	uint64_t *stack = (void *) fp;
	*prev = stack[FRAME_OFFSET_FP_PREV];
	return true;
}

bool kernel_return_address_get(uintptr_t fp, uintptr_t *ra)
{
	uint64_t *stack = (void *) fp;
	*ra = stack[FRAME_OFFSET_RA];
	return true;
}

bool uspace_frame_pointer_validate(uintptr_t fp)
{
	return fp != 0;
}

bool uspace_frame_pointer_prev(uintptr_t fp, uintptr_t *prev)
{
	return !copy_from_uspace((void *) prev,
	    (uint64_t *) fp + FRAME_OFFSET_FP_PREV, sizeof(*prev));
}

bool uspace_return_address_get(uintptr_t fp, uintptr_t *ra)
{
	return !copy_from_uspace((void *) ra, (uint64_t *) fp + FRAME_OFFSET_RA,
	    sizeof(*ra));
}

/** @}
 */
