/*
 * Copyright (c) 2012 Martin Decky
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

/** @addtogroup libcipc
 * @{
 */
/** @file
 */

#ifndef LIBC_IPC_OUTPUT_H_
#define LIBC_IPC_OUTPUT_H_

#include <ipc/common.h>

typedef sysarg_t frontbuf_handle_t;

typedef enum {
	OUTPUT_YIELD = IPC_FIRST_USER_METHOD,
	OUTPUT_CLAIM,
	OUTPUT_GET_DIMENSIONS,
	OUTPUT_GET_CAPS,

	OUTPUT_FRONTBUF_CREATE,
	OUTPUT_FRONTBUF_DESTROY,

	OUTPUT_CURSOR_UPDATE,
	OUTPUT_SET_STYLE,
	OUTPUT_SET_COLOR,
	OUTPUT_SET_RGB_COLOR,

	OUTPUT_UPDATE,
	OUTPUT_DAMAGE
} output_request_t;

#endif

/**
 * @}
 */
