/*
 * Copyright (c) 2016 Jiri Svoboda
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

/** @addtogroup netecho
 * @{
 */
/** @file Network UDP echo diagnostic utility.
 */

#include <stdbool.h>
#include <errno.h>
#include <io/console.h>
#include <stdio.h>
#include <str.h>

#include "comm.h"
#include "netecho.h"

#define NAME "netecho"

static console_ctrl_t *con;
static bool done;

void netecho_received(void *data, size_t size)
{
	char *p;
	size_t i;

	printf("Received message '");
	p = data;

	for (i = 0; i < size; i++)
		putchar(p[i]);
	printf("'.\n");
}

static void key_handle_ctrl(kbd_event_t *ev)
{
	switch (ev->key) {
	case KC_Q:
		done = true;
		break;
	default:
		break;
	}
}

static void send_char(wchar_t c)
{
	char cbuf[STR_BOUNDS(1)];
	size_t offs;
	errno_t rc;

	offs = 0;
	chr_encode(c, cbuf, &offs, STR_BOUNDS(1));

	rc = comm_send(cbuf, offs);
	if (rc != EOK) {
		printf("[Failed sending data]\n");
	}
}

static void key_handle_unmod(kbd_event_t *ev)
{
	switch (ev->key) {
	case KC_ENTER:
		send_char('\n');
		break;
	default:
		if (ev->c >= 32 || ev->c == '\t' || ev->c == '\b') {
			send_char(ev->c);
		}
	}
}

static void key_handle(kbd_event_t *ev)
{
	if ((ev->mods & KM_ALT) == 0 &&
	    (ev->mods & KM_SHIFT) == 0 &&
	    (ev->mods & KM_CTRL) != 0) {
		key_handle_ctrl(ev);
	} else if ((ev->mods & (KM_CTRL | KM_ALT)) == 0) {
		key_handle_unmod(ev);
	}
}


static void print_syntax(void)
{
	printf("syntax:\n");
	printf("\t%s -l <port>\n", NAME);
	printf("\t%s -d <host>:<port> [<message> [<message...>]]\n", NAME);
}

/* Interactive mode */
static void netecho_interact(void)
{
	cons_event_t ev;

	printf("Communication started. Press Ctrl-Q to quit.\n");

	con = console_init(stdin, stdout);

	done = false;
	while (!done) {
		console_get_event(con, &ev);
		if (ev.type == CEV_KEY && ev.ev.key.type == KEY_PRESS)
			key_handle(&ev.ev.key);
	}
}

static void netecho_send_messages(char **msgs)
{
	errno_t rc;

	while (*msgs != NULL) {
		rc = comm_send(*msgs, str_size(*msgs));
		if (rc != EOK) {
			printf("[Failed sending data]\n");
		}

		++msgs;
	}
}

int main(int argc, char *argv[])
{
	char *hostport;
	char *port;
	char **msgs;
	errno_t rc;

	if (argc < 2) {
		print_syntax();
		return 1;
	}

	if (str_cmp(argv[1], "-l") == 0) {
		if (argc != 3) {
			print_syntax();
			return 1;
		}

		port = argv[2];
		msgs = NULL;

		rc = comm_open_listen(port);
		if (rc != EOK) {
			printf("Error setting up communication.\n");
			return 1;
		}
	} else if (str_cmp(argv[1], "-d") == 0) {
		if (argc < 3) {
			print_syntax();
			return 1;
		}

		hostport = argv[2];
		port = NULL;
		msgs = argv + 3;

		rc = comm_open_talkto(hostport);
		if (rc != EOK) {
			printf("Error setting up communication.\n");
			return 1;
		}
	} else {
		print_syntax();
		return 1;
	}

	if (msgs != NULL && *msgs != NULL) {
		/* Just send messages and quit */
		netecho_send_messages(msgs);
	} else {
		/* Interactive mode */
		netecho_interact();
	}

	comm_close();
	return 0;
}

/** @}
 */
