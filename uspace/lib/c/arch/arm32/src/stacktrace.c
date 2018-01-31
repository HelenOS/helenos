/*
 * Copyright (c) 2010 Jakub Jermar
 * Copyright (c) 2010 Jiri Svoboda
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

/** @addtogroup libcarm32 arm32
 * @ingroup lc
 * @{
 */
/** @file
 */

#include <stdint.h>
#include <stdbool.h>

#include <stacktrace.h>

#define FRAME_OFFSET_FP_PREV	-12
#define FRAME_OFFSET_RA		-4

bool stacktrace_fp_valid(stacktrace_t *st, uintptr_t fp)
{
	(void) st;
	return fp != 0;
}

errno_t stacktrace_fp_prev(stacktrace_t *st, uintptr_t fp, uintptr_t *prev)
{
	return (*st->ops->read_uintptr)(st->op_arg, fp + FRAME_OFFSET_FP_PREV, prev);
}

errno_t stacktrace_ra_get(stacktrace_t *st, uintptr_t fp, uintptr_t *ra)
{
	return (*st->ops->read_uintptr)(st->op_arg, fp + FRAME_OFFSET_RA, ra);
}

/** @}
 */
