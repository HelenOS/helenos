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

#include <async.h>
#include <io/stream.h>
#include <ipc/console.h>
#include <ipc/services.h>
#include <errno.h>
#include <string.h>
#include <console.h>

static int console_phone = -1;

/** Size of cbuffer. */
#define CBUFFER_SIZE 256

/** Buffer for writing characters to the console. */
static char cbuffer[CBUFFER_SIZE];

/** Pointer to end of cbuffer. */
static char *cbuffer_end = cbuffer + CBUFFER_SIZE;

/** Pointer to first available field in cbuffer. */
static char *cbp = cbuffer;

static ssize_t cons_write(const char *buf, size_t nbyte);
static void cons_putchar(wchar_t c);

static void cbuffer_flush(void);
static void cbuffer_drain(void);
static inline void cbuffer_putc(int c);


void console_open(bool blocking)
{
	if (console_phone < 0) {
		int phone;
		if (blocking) {
			phone = ipc_connect_me_to_blocking(PHONE_NS,
			    SERVICE_CONSOLE, 0, 0);
		} else {
			phone = ipc_connect_me_to(PHONE_NS, SERVICE_CONSOLE, 0,
			    0);
		}
		if (phone >= 0)
			console_phone = phone;
	}
}

void console_close(void)
{
	if (console_phone >= 0) {
		if (ipc_hangup(console_phone) == 0) {
			console_phone = -1;
		}
	}
}

int console_phone_get(bool blocking)
{
	if (console_phone < 0)
		console_open(blocking);
	
	return console_phone;
}

void console_wait(void)
{
	while (console_phone < 0)
		console_open(true);
}

void console_clear(void)
{
	int cons_phone = console_phone_get(true);

	cbuffer_drain();
	async_msg_0(cons_phone, CONSOLE_CLEAR);
}

void console_goto(int row, int col)
{
	int cons_phone = console_phone_get(true);

	cbuffer_flush();
	async_msg_2(cons_phone, CONSOLE_GOTO, row, col);
}

void console_putchar(wchar_t c)
{
//	cbuffer_putc(c);
	cbuffer_flush();
	cons_putchar(c);
}

/** Write all data from output buffer to the console. */
static void cbuffer_flush(void)
{
	int rc;
	int len;

	len = cbp - cbuffer;

	while (len > 0) {
		rc = cons_write(cbuffer, cbp - cbuffer);
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
static inline void cbuffer_putc(int c)
{
	if (cbp == cbuffer_end)
		cbuffer_flush();

	*cbp++ = c;

	if (c == '\n')
		cbuffer_flush();
}

/** Write one character to the console via IPC. */
static void cons_putchar(wchar_t c)
{
	int cons_phone = console_phone_get(true);
	async_msg_1(cons_phone, CONSOLE_PUTCHAR, c);
}

/** Write characters to the console via IPC. */
static ssize_t cons_write(const char *buf, size_t nbyte) 
{
	int cons_phone = console_phone_get(true);
	ipcarg_t rc;
	ipc_call_t answer;
	aid_t req;

	async_serialize_start();
	
	req = async_send_0(cons_phone, CONSOLE_WRITE, &answer);
	rc = ipc_data_write_start(cons_phone, (void *) buf, nbyte);

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
}

/** Write characters to the console. */
ssize_t console_write(const char *buf, size_t nbyte) 
{
	size_t left;

	left = nbyte;

	while (left > 0) {
		cbuffer_putc(*buf++);
		--left;
	}

	return nbyte;
}

/** Write a NULL-terminated string to the console. */
void console_putstr(const char *s)
{
	size_t len;
	ssize_t rc;

	len = str_size(s);
	while (len > 0) {
		rc = console_write(s, len);
		if (rc < 0)
			return; /* Error */
		s += rc;
		len -= rc;
	}
}

/** Flush all output to the console. */
void console_flush(void)
{
	int cons_phone = console_phone_get(false);

	cbuffer_flush();
	async_msg_0(cons_phone, CONSOLE_FLUSH);
}

void console_flush_optional(void)
{
	if (console_phone >= 0)
		console_flush();
}

int console_get_size(int *rows, int *cols)
{
	int cons_phone = console_phone_get(true);
	ipcarg_t r, c;
	int rc;

	rc = async_req_0_2(cons_phone, CONSOLE_GETSIZE, &r, &c);

	*rows = (int) r;
	*cols = (int) c;

	return rc;
}

void console_set_style(int style)
{
	int cons_phone = console_phone_get(true);

	cbuffer_flush();
	async_msg_1(cons_phone, CONSOLE_SET_STYLE, style);
}

void console_set_color(int fg_color, int bg_color, int flags)
{
	int cons_phone = console_phone_get(true);

	cbuffer_flush();
	async_msg_3(cons_phone, CONSOLE_SET_COLOR, fg_color, bg_color, flags);
}

void console_set_rgb_color(int fg_color, int bg_color)
{
	int cons_phone = console_phone_get(true);

	cbuffer_flush();
	async_msg_2(cons_phone, CONSOLE_SET_RGB_COLOR, fg_color, bg_color);
}

void console_cursor_visibility(int show)
{
	int cons_phone = console_phone_get(true);

	cbuffer_flush();
	async_msg_1(cons_phone, CONSOLE_CURSOR_VISIBILITY, show != 0);
}

void console_kcon_enable(void)
{
	int cons_phone = console_phone_get(true);

	cbuffer_flush();
	async_msg_0(cons_phone, CONSOLE_KCON_ENABLE);
}

/** @}
 */
