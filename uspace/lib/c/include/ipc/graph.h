/*
 * Copyright (c) 2011 Petr Koupy
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

/** @addtogroup libc
 * @{
 */
/** @file
 */

#ifndef LIBC_IPC_GRAPH_H_
#define LIBC_IPC_GRAPH_H_

#include <ipc/common.h>

typedef enum {
	VISUALIZER_CLAIM = IPC_FIRST_USER_METHOD,
	VISUALIZER_YIELD,

	VISUALIZER_ENUMERATE_MODES,
	VISUALIZER_GET_DEFAULT_MODE,
	VISUALIZER_GET_CURRENT_MODE,
	VISUALIZER_GET_MODE,
	VISUALIZER_SET_MODE,

	VISUALIZER_UPDATE_DAMAGED_REGION,

	VISUALIZER_SUSPEND,
	VISUALIZER_WAKE_UP,
} visualizer_request_t;

typedef enum {
	VISUALIZER_MODE_CHANGE = IPC_FIRST_USER_METHOD,
	VISUALIZER_DISCONNECT
} visualizer_notif_t;

typedef enum {
	RENDERER_REQ_NONE = IPC_FIRST_USER_METHOD,
	// TODO: similar interface as provides libsoftrend
} renderer_request_t;

typedef enum {
	RENDERER_NOTIF_NONE = IPC_FIRST_USER_METHOD,
	// TODO: similar interface as provides libsoftrend
} renderer_notif_t;

#endif

/** @}
 */
