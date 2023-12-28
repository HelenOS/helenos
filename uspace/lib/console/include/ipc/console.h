/*
 * Copyright (c) 2023 Jiri Svoboda
 * Copyright (c) 2006 Josef Cejka
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

/** @addtogroup libconsole
 * @{
 */
/** @file
 */

#ifndef _LIBCONSOLE_IPC_CONSOLE_H_
#define _LIBCONSOLE_IPC_CONSOLE_H_

#include <ipc/vfs.h>

typedef enum {
	CONSOLE_GET_SIZE = VFS_OUT_LAST,
	CONSOLE_GET_COLOR_CAP,
	CONSOLE_GET_EVENT,
	CONSOLE_GET_POS,
	CONSOLE_SET_POS,
	CONSOLE_CLEAR,
	CONSOLE_SET_STYLE,
	CONSOLE_SET_COLOR,
	CONSOLE_SET_RGB_COLOR,
	CONSOLE_SET_CURSOR_VISIBILITY,
	CONSOLE_SET_CAPTION,
	CONSOLE_MAP,
	CONSOLE_UNMAP,
	CONSOLE_UPDATE
} console_request_t;

#endif

/** @}
 */
