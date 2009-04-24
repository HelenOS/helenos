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
#include <io/stream.h>
#include <ipc/console.h>
#include <ipc/services.h>
#include <errno.h>
#include <string.h>
#include <console.h>

static int console_phone = -1;

/** Size of cbuffer. */
#define CBUFFER_SIZE  256

/** Buffer for writing characters to the console. */
static char cbuffer[CBUFFER_SIZE];

/** Pointer to end of cbuffer. */
static char *cbuffer_end = cbuffer + CBUFFER_SIZE;

/** Pointer to first available field in cbuffer. */
static char *cbp = cbuffer;


/** Write one character to the console via IPC. */
static void cons_putchar(wchar_t c)
{
	console_wait();
	async_msg_1(console_phone, CONSOLE_PUTCHAR, c);
}

/** Write characters to the console via IPC or to klog */
static ssize_t cons_write(const char *buf, size_t size)
{
	console_open(false);
	
	if (console_phone >= 0) {
		async_serialize_start();
		
		ipc_call_t answer;
		aid_t req = async_send_0(console_phone, CONSOLE_WRITE, &answer);
		ipcarg_t rc = ipc_data_write_start(console_phone, (void *) buf, size);
		
		if (rc != EOK) {
			async_wait_for(req, NULL);
			async_serialize_end();
			return (ssize_t) rc;
		}
		
		async_wait_for(req, &rc);
		async_serialize_end();
		
		if (rc == EOK)
			return (ssize_t) IPC_GET_ARG1(answer);
		else
			return -1;
	} else
		return __SYSCALL3(SYS_KLOG, 1, (sysarg_t) buf, size);
}

/** Write all data from output buffer to the console. */
static void cbuffer_flush(void)
{
	size_t len = cbp - cbuffer;
	
	while (len > 0) {
		ssize_t rc = cons_write(cbuffer, cbp - cbuffer);
		if (rc < 0)
			return;
		
		len -= rc;
	}
	
	cbp = cbuffer;
}

/** Drop all data in console output buffer. */
static void cbuffer_drain(void)
{
	cbp = cbuffer;
}

/** Write one character to the output buffer. */
static inline void cbuffer_putc(char c)
{
	if (cbp == cbuffer_end)
		cbuffer_flush();
	
	*cbp++ = c;
	
	if (c == '\n')
		cbuffer_flush();
}

int console_open(bool blocking)
{
	if (console_phone < 0) {
		int phone;
		
		if (blocking)
			phone = ipc_connect_me_to_blocking(PHONE_NS,
			    SERVICE_CONSOLE, 0, 0);
		else
			phone = ipc_connect_me_to(PHONE_NS,
			    SERVICE_CONSOLE, 0, 0);
		
		if (phone >= 0)
			console_phone = phone;
	}
	
	return console_phone;
}

void console_close(void)
{
	if (console_phone >= 0) {
		if (ipc_hangup(console_phone) == 0)
			console_phone = -1;
	}
}

void console_wait(void)
{
	while (console_phone < 0)
		console_open(true);
}

void console_clear(void)
{
	console_wait();
	cbuffer_drain();
	async_msg_0(console_phone, CONSOLE_CLEAR);
}

int console_get_size(int *rows, int *cols)
{
	console_wait();
	
	ipcarg_t r;
	ipcarg_t c;
	int rc = async_req_0_2(console_phone, CONSOLE_GETSIZE, &r, &c);
	
	*rows = (int) r;
	*cols = (int) c;
	
	return rc;
}

void console_set_style(int style)
{
	console_wait();
	cbuffer_flush();
	async_msg_1(console_phone, CONSOLE_SET_STYLE, style);
}

void console_set_color(int fg_color, int bg_color, int flags)
{
	console_wait();
	cbuffer_flush();
	async_msg_3(console_phone, CONSOLE_SET_COLOR, fg_color, bg_color, flags);
}

void console_set_rgb_color(int fg_color, int bg_color)
{
	console_wait();
	cbuffer_flush();
	async_msg_2(console_phone, CONSOLE_SET_RGB_COLOR, fg_color, bg_color);
}

void console_cursor_visibility(int show)
{
	console_wait();
	cbuffer_flush();
	async_msg_1(console_phone, CONSOLE_CURSOR_VISIBILITY, show != 0);
}

void console_kcon_enable(void)
{
	console_wait();
	cbuffer_flush();
	async_msg_0(console_phone, CONSOLE_KCON_ENABLE);
}

void console_goto(int row, int col)
{
	console_wait();
	cbuffer_flush();
	async_msg_2(console_phone, CONSOLE_GOTO, row, col);
}

void console_putchar(wchar_t c)
{
	console_wait();
	cbuffer_flush();
	cons_putchar(c);
}

/** Write characters to the console. */
ssize_t console_write(const char *buf, size_t size)
{
	size_t left = size;
	
	while (left > 0) {
		cbuffer_putc(*buf++);
		left--;
	}
	
	return size;
}

/** Write a NULL-terminated string to the console. */
void console_putstr(const char *str)
{
	size_t left = str_size(str);
	
	while (left > 0) {
		ssize_t rc = console_write(str, left);
		
		if (rc < 0) {
			/* Error */
			return;
		}
		
		str += rc;
		left -= rc;
	}
}

/** Flush all output to the console or klog. */
void console_flush(void)
{
	cbuffer_flush();
	if (console_phone >= 0)
		async_msg_0(console_phone, CONSOLE_FLUSH);
}

/** @}
 */
