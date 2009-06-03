/*
 * Copyright (c) 2006 Josef Cejka
 * Copyright (c) 2006 Jakub Vana
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

/** @addtogroup libc
 * @{
 */
/** @file
 */

#include <libc.h>
#include <async.h>
#include <io/console.h>
#include <ipc/console.h>

void console_clear(int phone)
{
	async_msg_0(phone, CONSOLE_CLEAR);
}

int console_get_size(int phone, ipcarg_t *rows, ipcarg_t *cols)
{
	return async_req_0_2(phone, CONSOLE_GET_SIZE, rows, cols);
}

void console_set_style(int phone, int style)
{
	async_msg_1(phone, CONSOLE_SET_STYLE, style);
}

void console_set_color(int phone, int fg_color, int bg_color, int flags)
{
	async_msg_3(phone, CONSOLE_SET_COLOR, fg_color, bg_color, flags);
}

void console_set_rgb_color(int phone, int fg_color, int bg_color)
{
	async_msg_2(phone, CONSOLE_SET_RGB_COLOR, fg_color, bg_color);
}

void console_cursor_visibility(int phone, bool show)
{
	async_msg_1(phone, CONSOLE_CURSOR_VISIBILITY, show != false);
}

void console_kcon_enable(int phone)
{
	async_msg_0(phone, CONSOLE_KCON_ENABLE);
}

void console_goto(int phone, ipcarg_t row, ipcarg_t col)
{
	async_msg_2(phone, CONSOLE_GOTO, row, col);
}

bool console_get_event(int phone, console_event_t *event)
{
	ipcarg_t type;
	ipcarg_t key;
	ipcarg_t mods;
	ipcarg_t c;
	
	int rc = async_req_0_4(phone, CONSOLE_GET_EVENT, &type, &key, &mods, &c);
	if (rc < 0)
		return false;
	
	event->type = type;
	event->key = key;
	event->mods = mods;
	event->c = c;
	
	return true;
}

/** @}
 */
