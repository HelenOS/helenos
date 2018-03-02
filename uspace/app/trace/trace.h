/*
 * Copyright (c) 2008 Jiri Svoboda
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

/** @addtogroup trace
 * @{
 */
/** @file
 */

#ifndef TRACE_H_
#define TRACE_H_

#include <types/common.h>

/**
 * Classes of events that can be displayed. Can be or-ed together.
 */
typedef enum {

	DM_THREAD	= 1,	/**< Thread creation and termination events */
	DM_SYSCALL	= 2,	/**< System calls */
	DM_IPC		= 4,	/**< Low-level IPC */
	DM_SYSTEM	= 8,	/**< Sysipc protocol */
	DM_USER		= 16	/**< User IPC protocols */

} display_mask_t;

typedef enum {
	V_VOID,
	V_INTEGER,
	V_PTR,
	V_HASH,
	V_ERRNO,
	V_INT_ERRNO,
	V_CHAR
} val_type_t;

/** Combination of events to print. */
extern display_mask_t display_mask;

void val_print(sysarg_t val, val_type_t v_type);

#endif

/** @}
 */
