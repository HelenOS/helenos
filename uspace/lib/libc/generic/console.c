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

/** @addtogroup libc
 * @{
 */
/** @file
 */ 

#include <async.h>
#include <io/stream.h>
#include <ipc/console.h>
#include <console.h>

void console_clear(void)
{
	int cons_phone = get_console_phone();
	async_msg_0(cons_phone, CONSOLE_CLEAR);
}

void console_goto(int row, int col)
{
	int cons_phone = get_console_phone();
	async_msg_2(cons_phone, CONSOLE_GOTO, row, col);
}

void console_flush(void)
{
	int cons_phone = get_console_phone();
	async_msg_0(cons_phone, CONSOLE_FLUSH);
}

int console_get_size(int *rows, int *cols)
{
	int cons_phone = get_console_phone();
	ipcarg_t r, c;
	int rc;

	rc = async_req_0_2(cons_phone, CONSOLE_GETSIZE, &r, &c);

	*rows = (int) r;
	*cols = (int) c;

	return rc;
}

void console_set_style(int style)
{
	int cons_phone = get_console_phone();
	async_msg_1(cons_phone, CONSOLE_SET_STYLE, style);
}

void console_set_color(int fg_color, int bg_color, int flags)
{
	int cons_phone = get_console_phone();
	async_msg_3(cons_phone, CONSOLE_SET_COLOR, fg_color, bg_color, flags);
}

void console_set_rgb_color(int fg_color, int bg_color)
{
	int cons_phone = get_console_phone();
	async_msg_2(cons_phone, CONSOLE_SET_RGB_COLOR, fg_color, bg_color);
}

void console_cursor_visibility(int show)
{
	int cons_phone = get_console_phone();
	async_msg_1(cons_phone, CONSOLE_CURSOR_VISIBILITY, show != 0);
}

/** @}
 */
