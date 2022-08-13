/*
 * SPDX-FileCopyrightText: 2012 Jiri Svoboda
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup nterm
 * @{
 */
/** @file Network serial terminal emulator.
 */

#include <stdbool.h>
#include <errno.h>
#include <io/console.h>
#include <stdio.h>
#include <str.h>

#include "conn.h"
#include "nterm.h"

#define NAME "nterm"

static console_ctrl_t *con;
static bool done;

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

static void send_char(char32_t c)
{
	char cbuf[STR_BOUNDS(1)];
	size_t offs;
	errno_t rc;

	offs = 0;
	chr_encode(c, cbuf, &offs, STR_BOUNDS(1));

	rc = conn_send(cbuf, offs);
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

void nterm_received(void *data, size_t size)
{
	fwrite(data, size, 1, stdout);
	fflush(stdout);
}

static void print_syntax(void)
{
	printf("syntax: nterm <host>:<port>\n");
}

int main(int argc, char *argv[])
{
	cons_event_t ev;
	errno_t rc;

	if (argc != 2) {
		print_syntax();
		return 1;
	}

	rc = conn_open(argv[1]);
	if (rc != EOK) {
		printf("Error connecting.\n");
		return 1;
	}

	printf("Connection established.\n");

	con = console_init(stdin, stdout);

	done = false;
	while (!done) {
		rc = console_get_event(con, &ev);
		if (rc != EOK)
			break;

		if (ev.type == CEV_KEY && ev.ev.key.type == KEY_PRESS)
			key_handle(&ev.ev.key);
	}

	return 0;
}

/** @}
 */
