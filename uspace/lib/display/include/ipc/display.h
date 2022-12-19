/*
 * Copyright (c) 2022 Jiri Svoboda
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

/** @addtogroup libdisplay
 * @{
 */
/** @file
 */

#ifndef LIBDISPLAY_IPC_DISPLAY_H
#define LIBDISPLAY_IPC_DISPLAY_H

#include <ipc/common.h>

typedef enum {
	DISPLAY_CALLBACK_CREATE = IPC_FIRST_USER_METHOD,
	DISPLAY_WINDOW_CREATE,
	DISPLAY_WINDOW_DESTROY,
	DISPLAY_WINDOW_MINIMIZE,
	DISPLAY_WINDOW_MAXIMIZE,
	DISPLAY_WINDOW_MOVE,
	DISPLAY_WINDOW_MOVE_REQ,
	DISPLAY_WINDOW_GET_POS,
	DISPLAY_WINDOW_GET_MAX_RECT,
	DISPLAY_WINDOW_RESIZE,
	DISPLAY_WINDOW_RESIZE_REQ,
	DISPLAY_WINDOW_SET_CURSOR,
	DISPLAY_WINDOW_SET_CAPTION,
	DISPLAY_WINDOW_UNMAXIMIZE,
	DISPLAY_GET_EVENT,
	DISPLAY_GET_INFO
} display_request_t;

typedef enum {
	DISPLAY_EV_PENDING = IPC_FIRST_USER_METHOD
} display_event_t;

#endif

/** @}
 */
