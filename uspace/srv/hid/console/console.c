/*
 * Copyright (c) 2011 Martin Decky
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

/** @addtogroup console
 * @{
 */
/** @file
 */

#include <async.h>
#include <stdio.h>
#include <adt/prodcons.h>
#include <io/input.h>
#include <ipc/console.h>
#include <ipc/vfs.h>
#include <errno.h>
#include <str_error.h>
#include <loc.h>
#include <event.h>
#include <io/kbd_event.h>
#include <io/keycode.h>
#include <io/chargrid.h>
#include <io/console.h>
#include <io/output.h>
#include <align.h>
#include <malloc.h>
#include <as.h>
#include <fibril_synch.h>
#include "console.h"

#define NAME       "console"
#define NAMESPACE  "term"

#define UTF8_CHAR_BUFFER_SIZE  (STR_BOUNDS(1) + 1)

typedef struct {
	atomic_t refcnt;      /**< Connection reference count */
	prodcons_t input_pc;  /**< Incoming keyboard events */
	
	/**
	 * Not yet sent bytes of last char event.
	 */
	char char_remains[UTF8_CHAR_BUFFER_SIZE];
	size_t char_remains_len;  /**< Number of not yet sent bytes. */
	
	fibril_mutex_t mtx;  /**< Lock protecting mutable fields */
	
	size_t index;           /**< Console index */
	service_id_t dsid;      /**< Service handle */
	
	sysarg_t cols;         /**< Number of columns */
	sysarg_t rows;         /**< Number of rows */
	console_caps_t ccaps;  /**< Console capabilities */
	
	chargrid_t *frontbuf;    /**< Front buffer */
	frontbuf_handle_t fbid;  /**< Front buffer handle */
} console_t;

/** Input server proxy */
static input_t *input;

/** Session to the output server */
static async_sess_t *output_sess;

/** Output dimensions */
static sysarg_t cols;
static sysarg_t rows;

/** Array of data for virtual consoles */
static console_t consoles[CONSOLE_COUNT];

/** Mutex for console switching */
static FIBRIL_MUTEX_INITIALIZE(switch_mtx);

static console_t *prev_console = &consoles[0];
static console_t *active_console = &consoles[0];
static console_t *kernel_console = &consoles[KERNEL_CONSOLE];

static int input_ev_key(input_t *, kbd_event_type_t, keycode_t, keymod_t, wchar_t);
static int input_ev_move(input_t *, int, int);
static int input_ev_abs_move(input_t *, unsigned, unsigned, unsigned, unsigned);
static int input_ev_button(input_t *, int, int);

static input_ev_ops_t input_ev_ops = {
	.key = input_ev_key,
	.move = input_ev_move,
	.abs_move = input_ev_abs_move,
	.button = input_ev_button
};

static void cons_update(console_t *cons)
{
	fibril_mutex_lock(&switch_mtx);
	fibril_mutex_lock(&cons->mtx);
	
	if ((cons == active_console) && (active_console != kernel_console)) {
		output_update(output_sess, cons->fbid);
		output_cursor_update(output_sess, cons->fbid);
	}
	
	fibril_mutex_unlock(&cons->mtx);
	fibril_mutex_unlock(&switch_mtx);
}

static void cons_update_cursor(console_t *cons)
{
	fibril_mutex_lock(&switch_mtx);
	fibril_mutex_lock(&cons->mtx);
	
	if ((cons == active_console) && (active_console != kernel_console))
		output_cursor_update(output_sess, cons->fbid);
	
	fibril_mutex_unlock(&cons->mtx);
	fibril_mutex_unlock(&switch_mtx);
}

static void cons_clear(console_t *cons)
{
	fibril_mutex_lock(&cons->mtx);
	chargrid_clear(cons->frontbuf);
	fibril_mutex_unlock(&cons->mtx);
	
	cons_update(cons);
}

static void cons_damage(console_t *cons)
{
	fibril_mutex_lock(&switch_mtx);
	fibril_mutex_lock(&cons->mtx);
	
	if ((cons == active_console) && (active_console != kernel_console)) {
		output_damage(output_sess, cons->fbid, 0, 0, cons->cols,
		    cons->rows);
		output_cursor_update(output_sess, cons->fbid);
	}
	
	fibril_mutex_unlock(&cons->mtx);
	fibril_mutex_unlock(&switch_mtx);
}

static void cons_switch(console_t *cons)
{
	fibril_mutex_lock(&switch_mtx);
	
	if (cons == active_console) {
		fibril_mutex_unlock(&switch_mtx);
		return;
	}
	
	if (cons == kernel_console) {
		output_yield(output_sess);
		if (!console_kcon()) {
			output_claim(output_sess);
			fibril_mutex_unlock(&switch_mtx);
			return;
		}
	}
	
	if (active_console == kernel_console)
		output_claim(output_sess);
	
	prev_console = active_console;
	active_console = cons;
	
	fibril_mutex_unlock(&switch_mtx);
	
	cons_damage(cons);
}

static console_t *cons_get_active_uspace(void)
{
	fibril_mutex_lock(&switch_mtx);
	
	console_t *active_uspace = active_console;
	if (active_uspace == kernel_console)
		active_uspace = prev_console;
	
	assert(active_uspace != kernel_console);
	
	fibril_mutex_unlock(&switch_mtx);
	
	return active_uspace;
}

static int input_ev_key(input_t *input, kbd_event_type_t type, keycode_t key,
    keymod_t mods, wchar_t c)
{
	if ((key >= KC_F1) && (key < KC_F1 + CONSOLE_COUNT) &&
	    ((mods & KM_CTRL) == 0)) {
		cons_switch(&consoles[key - KC_F1]);
	} else {
		/* Got key press/release event */
		kbd_event_t *event =
		    (kbd_event_t *) malloc(sizeof(kbd_event_t));
		if (event == NULL) {
			return ENOMEM;
		}
		
		link_initialize(&event->link);
		event->type = type;
		event->key = key;
		event->mods = mods;
		event->c = c;
		
		/*
		 * Kernel console does not read events
		 * from us, so we will redirect them
		 * to the (last) active userspace console
		 * if necessary.
		 */
		console_t *target_console = cons_get_active_uspace();
		
		prodcons_produce(&target_console->input_pc,
		    &event->link);
	}
	
	return EOK;
}

static int input_ev_move(input_t *input, int dx, int dy)
{
	return EOK;
}

static int input_ev_abs_move(input_t *input, unsigned x , unsigned y,
    unsigned max_x, unsigned max_y)
{
	return EOK;
}

static int input_ev_button(input_t *input, int bnum, int bpress)
{
	return EOK;
}

/** Process a character from the client (TTY emulation). */
static void cons_write_char(console_t *cons, wchar_t ch)
{
	sysarg_t updated = 0;
	
	fibril_mutex_lock(&cons->mtx);
	
	switch (ch) {
	case '\n':
		updated = chargrid_newline(cons->frontbuf);
		break;
	case '\r':
		break;
	case '\t':
		updated = chargrid_tabstop(cons->frontbuf, 8);
		break;
	case '\b':
		updated = chargrid_backspace(cons->frontbuf);
		break;
	default:
		updated = chargrid_putchar(cons->frontbuf, ch, true);
	}
	
	fibril_mutex_unlock(&cons->mtx);
	
	if (updated > 1)
		cons_update(cons);
}

static void cons_set_cursor(console_t *cons, sysarg_t col, sysarg_t row)
{
	fibril_mutex_lock(&cons->mtx);
	chargrid_set_cursor(cons->frontbuf, col, row);
	fibril_mutex_unlock(&cons->mtx);
	
	cons_update_cursor(cons);
}

static void cons_set_cursor_visibility(console_t *cons, bool visible)
{
	fibril_mutex_lock(&cons->mtx);
	chargrid_set_cursor_visibility(cons->frontbuf, visible);
	fibril_mutex_unlock(&cons->mtx);
	
	cons_update_cursor(cons);
}

static void cons_get_cursor(console_t *cons, ipc_callid_t iid, ipc_call_t *icall)
{
	sysarg_t col;
	sysarg_t row;
	
	fibril_mutex_lock(&cons->mtx);
	chargrid_get_cursor(cons->frontbuf, &col, &row);
	fibril_mutex_unlock(&cons->mtx);
	
	async_answer_2(iid, EOK, col, row);
}

static void cons_write(console_t *cons, ipc_callid_t iid, ipc_call_t *icall)
{
	void *buf;
	size_t size;
	int rc = async_data_write_accept(&buf, false, 0, 0, 0, &size);
	
	if (rc != EOK) {
		async_answer_0(iid, rc);
		return;
	}
	
	size_t off = 0;
	while (off < size)
		cons_write_char(cons, str_decode(buf, &off, size));
	
	async_answer_1(iid, EOK, size);
	free(buf);
}

static void cons_read(console_t *cons, ipc_callid_t iid, ipc_call_t *icall)
{
	ipc_callid_t callid;
	size_t size;
	if (!async_data_read_receive(&callid, &size)) {
		async_answer_0(callid, EINVAL);
		async_answer_0(iid, EINVAL);
		return;
	}
	
	char *buf = (char *) malloc(size);
	if (buf == NULL) {
		async_answer_0(callid, ENOMEM);
		async_answer_0(iid, ENOMEM);
		return;
	}
	
	size_t pos = 0;
	
	/*
	 * Read input from keyboard and copy it to the buffer.
	 * We need to handle situation when wchar is split by 2 following
	 * reads.
	 */
	while (pos < size) {
		/* Copy to the buffer remaining characters. */
		while ((pos < size) && (cons->char_remains_len > 0)) {
			buf[pos] = cons->char_remains[0];
			pos++;
			
			/* Unshift the array. */
			for (size_t i = 1; i < cons->char_remains_len; i++)
				cons->char_remains[i - 1] = cons->char_remains[i];
			
			cons->char_remains_len--;
		}
		
		/* Still not enough? Then get another key from the queue. */
		if (pos < size) {
			link_t *link = prodcons_consume(&cons->input_pc);
			kbd_event_t *event = list_get_instance(link, kbd_event_t, link);
			
			/* Accept key presses of printable chars only. */
			if ((event->type == KEY_PRESS) && (event->c != 0)) {
				wchar_t tmp[2] = { event->c, 0 };
				wstr_to_str(cons->char_remains, UTF8_CHAR_BUFFER_SIZE, tmp);
				cons->char_remains_len = str_size(cons->char_remains);
			}
			
			free(event);
		}
	}
	
	(void) async_data_read_finalize(callid, buf, size);
	async_answer_1(iid, EOK, size);
	free(buf);
}

static void cons_set_style(console_t *cons, console_style_t style)
{
	fibril_mutex_lock(&cons->mtx);
	chargrid_set_style(cons->frontbuf, style);
	fibril_mutex_unlock(&cons->mtx);
}

static void cons_set_color(console_t *cons, console_color_t bgcolor,
    console_color_t fgcolor, console_color_attr_t attr)
{
	fibril_mutex_lock(&cons->mtx);
	chargrid_set_color(cons->frontbuf, bgcolor, fgcolor, attr);
	fibril_mutex_unlock(&cons->mtx);
}

static void cons_set_rgb_color(console_t *cons, pixel_t bgcolor,
    pixel_t fgcolor)
{
	fibril_mutex_lock(&cons->mtx);
	chargrid_set_rgb_color(cons->frontbuf, bgcolor, fgcolor);
	fibril_mutex_unlock(&cons->mtx);
}

static void cons_get_event(console_t *cons, ipc_callid_t iid, ipc_call_t *icall)
{
	link_t *link = prodcons_consume(&cons->input_pc);
	kbd_event_t *event = list_get_instance(link, kbd_event_t, link);
	
	async_answer_4(iid, EOK, event->type, event->key, event->mods, event->c);
	free(event);
}

static void client_connection(ipc_callid_t iid, ipc_call_t *icall, void *arg)
{
	console_t *cons = NULL;
	
	for (size_t i = 0; i < CONSOLE_COUNT; i++) {
		if (i == KERNEL_CONSOLE)
			continue;
		
		if (consoles[i].dsid == (service_id_t) IPC_GET_ARG1(*icall)) {
			cons = &consoles[i];
			break;
		}
	}
	
	if (cons == NULL) {
		async_answer_0(iid, ENOENT);
		return;
	}
	
	if (atomic_postinc(&cons->refcnt) == 0)
		cons_set_cursor_visibility(cons, true);
	
	/* Accept the connection */
	async_answer_0(iid, EOK);
	
	while (true) {
		ipc_call_t call;
		ipc_callid_t callid = async_get_call(&call);
		
		if (!IPC_GET_IMETHOD(call))
			return;
		
		switch (IPC_GET_IMETHOD(call)) {
		case VFS_OUT_READ:
			cons_read(cons, callid, &call);
			break;
		case VFS_OUT_WRITE:
			cons_write(cons, callid, &call);
			break;
		case VFS_OUT_SYNC:
			cons_update(cons);
			async_answer_0(callid, EOK);
			break;
		case CONSOLE_CLEAR:
			cons_clear(cons);
			async_answer_0(callid, EOK);
			break;
		case CONSOLE_GOTO:
			cons_set_cursor(cons, IPC_GET_ARG1(call), IPC_GET_ARG2(call));
			async_answer_0(callid, EOK);
			break;
		case CONSOLE_GET_POS:
			cons_get_cursor(cons, callid, &call);
			break;
		case CONSOLE_GET_SIZE:
			async_answer_2(callid, EOK, cons->cols, cons->rows);
			break;
		case CONSOLE_GET_COLOR_CAP:
			async_answer_1(callid, EOK, cons->ccaps);
			break;
		case CONSOLE_SET_STYLE:
			cons_set_style(cons, IPC_GET_ARG1(call));
			async_answer_0(callid, EOK);
			break;
		case CONSOLE_SET_COLOR:
			cons_set_color(cons, IPC_GET_ARG1(call), IPC_GET_ARG2(call),
			    IPC_GET_ARG3(call));
			async_answer_0(callid, EOK);
			break;
		case CONSOLE_SET_RGB_COLOR:
			cons_set_rgb_color(cons, IPC_GET_ARG1(call), IPC_GET_ARG2(call));
			async_answer_0(callid, EOK);
			break;
		case CONSOLE_CURSOR_VISIBILITY:
			cons_set_cursor_visibility(cons, IPC_GET_ARG1(call));
			async_answer_0(callid, EOK);
			break;
		case CONSOLE_GET_EVENT:
			cons_get_event(cons, callid, &call);
			break;
		default:
			async_answer_0(callid, EINVAL);
		}
	}
}

static int input_connect(const char *svc)
{
	async_sess_t *sess;
	service_id_t dsid;
	
	int rc = loc_service_get_id(svc, &dsid, 0);
	if (rc != EOK) {
		printf("%s: Input service %s not found\n", NAME, svc);
		return rc;
	}

	sess = loc_service_connect(EXCHANGE_ATOMIC, dsid, 0);
	if (sess == NULL) {
		printf("%s: Unable to connect to input service %s\n", NAME,
		    svc);
		return EIO;
	}
	
	rc = input_open(sess, &input_ev_ops, NULL, &input);
	if (rc != EOK) {
		async_hangup(sess);
		printf("%s: Unable to communicate with service %s (%s)\n",
		    NAME, svc, str_error(rc));
		return rc;
	}
	
	return EOK;
}

static void interrupt_received(ipc_callid_t callid, ipc_call_t *call)
{
	cons_switch(prev_console);
}

static async_sess_t *output_connect(const char *svc)
{
	async_sess_t *sess;
	service_id_t dsid;
	
	int rc = loc_service_get_id(svc, &dsid, 0);
	if (rc == EOK) {
		sess = loc_service_connect(EXCHANGE_SERIALIZE, dsid, 0);
		if (sess == NULL) {
			printf("%s: Unable to connect to output service %s\n",
			    NAME, svc);
			return NULL;
		}
	} else
		return NULL;
	
	return sess;
}

static bool console_srv_init(char *input_svc, char *output_svc)
{
	int rc;
	
	/* Connect to input service */
	rc = input_connect(input_svc);
	if (rc != EOK)
		return false;
	
	/* Connect to output service */
	output_sess = output_connect(output_svc);
	if (output_sess == NULL)
		return false;
	
	/* Register server */
	async_set_client_connection(client_connection);
	rc = loc_server_register(NAME);
	if (rc != EOK) {
		printf("%s: Unable to register server (%s)\n", NAME,
		    str_error(rc));
		return false;
	}
	
	output_get_dimensions(output_sess, &cols, &rows);
	output_set_style(output_sess, STYLE_NORMAL);
	
	console_caps_t ccaps;
	output_get_caps(output_sess, &ccaps);
	
	/* Inititalize consoles */
	for (size_t i = 0; i < CONSOLE_COUNT; i++) {
		consoles[i].index = i;
		atomic_set(&consoles[i].refcnt, 0);
		fibril_mutex_initialize(&consoles[i].mtx);
		prodcons_initialize(&consoles[i].input_pc);
		consoles[i].char_remains_len = 0;
		
		if (i == KERNEL_CONSOLE)
			continue;
		
		consoles[i].cols = cols;
		consoles[i].rows = rows;
		consoles[i].ccaps = ccaps;
		consoles[i].frontbuf =
		    chargrid_create(cols, rows, CHARGRID_FLAG_SHARED);
		
		if (consoles[i].frontbuf == NULL) {
			printf("%s: Unable to allocate frontbuffer %zu\n", NAME, i);
			return false;
		}
		
		consoles[i].fbid = output_frontbuf_create(output_sess,
		    consoles[i].frontbuf);
		if (consoles[i].fbid == 0) {
			printf("%s: Unable to create frontbuffer %zu\n", NAME, i);
			return false;
		}
		
		char vc[LOC_NAME_MAXLEN + 1];
		snprintf(vc, LOC_NAME_MAXLEN, "%s/vc%zu", NAMESPACE, i);
		
		if (loc_service_register(vc, &consoles[i].dsid) != EOK) {
			printf("%s: Unable to register device %s\n", NAME, vc);
			return false;
		}
	}
	
	cons_damage(active_console);
	
	/* Receive kernel notifications */
	async_set_interrupt_received(interrupt_received);
	rc = event_subscribe(EVENT_KCONSOLE, 0);
	if (rc != EOK)
		printf("%s: Failed to register kconsole notifications (%s)\n",
		    NAME, str_error(rc));
	
	return true;
}

static void usage(char *name)
{
	printf("Usage: %s <input_dev> <output_dev>\n", name);
}

int main(int argc, char *argv[])
{
	if (argc < 3) {
		usage(argv[0]);
		return -1;
	}
	
	printf("%s: HelenOS Console service\n", NAME);
	
	if (!console_srv_init(argv[1], argv[2]))
		return -1;
	
	printf("%s: Accepting connections\n", NAME);
	task_retval(0);
	async_manager();
	
	/* Never reached */
	return 0;
}

/** @}
 */
