/*
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

/** @addtogroup console
 * @{ 
 */
/** @file
 */

#include <libc.h>
#include <fb.h>
#include <ipc/ipc.h>
#include <keys.h>
#include <ipc/fb.h>
#include <ipc/services.h>
#include <errno.h>
#include <key_buffer.h>
#include <ipc/console.h>
#include <unistd.h>
#include <async.h>
#include <libadt/fifo.h>
#include <screenbuffer.h>
#include <sys/mman.h>
#include <stdio.h>
#include <sysinfo.h>

#include "console.h"
#include "gcons.h"

#define MAX_KEYREQUESTS_BUFFERED 32

#define NAME "console"

/** Index of currently used virtual console.
 */
int active_console = 0;
int prev_console = 0;

/** Information about framebuffer
 */
struct {
	int phone;		/**< Framebuffer phone */
	ipcarg_t rows;		/**< Framebuffer rows */
	ipcarg_t cols;		/**< Framebuffer columns */
} fb_info;

typedef struct {
	keybuffer_t keybuffer;		/**< Buffer for incoming keys. */
	/** Buffer for unsatisfied request for keys. */
	FIFO_CREATE_STATIC(keyrequests, ipc_callid_t,
		MAX_KEYREQUESTS_BUFFERED);	
	int keyrequest_counter;		/**< Number of requests in buffer. */
	int client_phone;		/**< Phone to connected client. */
	int used;			/**< 1 if this virtual console is
					 * connected to some client.*/
	screenbuffer_t screenbuffer;	/**< Screenbuffer for saving screen
					 * contents and related settings. */
} connection_t;

static connection_t connections[CONSOLE_COUNT];	/**< Array of data for virtual
						 * consoles */
static keyfield_t *interbuffer = NULL;		/**< Pointer to memory shared
						 * with framebufer used for
						 * faster virtual console
						 * switching */


/** Find unused virtual console.
 *
 */
static int find_free_connection(void) 
{
	int i;
	
	for (i = 0; i < CONSOLE_COUNT; i++) {
		if (!connections[i].used)
			return i;
	}
	return -1;
}

static void clrscr(void)
{
	async_msg_0(fb_info.phone, FB_CLEAR);
}

static void curs_visibility(bool visible)
{
	async_msg_1(fb_info.phone, FB_CURSOR_VISIBILITY, visible); 
}

static void curs_hide_sync(void)
{
	ipc_call_sync_1_0(fb_info.phone, FB_CURSOR_VISIBILITY, false); 
}

static void curs_goto(int row, int col)
{
	async_msg_2(fb_info.phone, FB_CURSOR_GOTO, row, col); 
}

static void set_style(int style)
{
	async_msg_1(fb_info.phone, FB_SET_STYLE, style); 
}

static void set_color(int fgcolor, int bgcolor, int flags)
{
	async_msg_3(fb_info.phone, FB_SET_COLOR, fgcolor, bgcolor, flags);
}

static void set_rgb_color(int fgcolor, int bgcolor)
{
	async_msg_2(fb_info.phone, FB_SET_RGB_COLOR, fgcolor, bgcolor); 
}

static void set_attrs(attrs_t *attrs)
{
	switch (attrs->t) {
	case at_style:
		set_style(attrs->a.s.style);
		break;

	case at_idx:
		set_color(attrs->a.i.fg_color, attrs->a.i.bg_color,
		    attrs->a.i.flags);
		break;

	case at_rgb:
		set_rgb_color(attrs->a.r.fg_color, attrs->a.r.bg_color);
		break;
	}
}

static void prtchr(char c, int row, int col)
{
	async_msg_3(fb_info.phone, FB_PUTCHAR, c, row, col);
}

/** Check key and process special keys. 
 *
 *
 */
static void write_char(int console, char key)
{
	screenbuffer_t *scr = &(connections[console].screenbuffer);
	
	switch (key) {
	case '\n':
		scr->position_y++;
		scr->position_x = 0;
		break;
	case '\r':
		break;
	case '\t':
		scr->position_x += 8;
		scr->position_x -= scr->position_x % 8; 
		break;
	case '\b':
		if (scr->position_x == 0) 
			break;
		scr->position_x--;
		if (console == active_console)
			prtchr(' ', scr->position_y, scr->position_x);
		screenbuffer_putchar(scr, ' ');
		break;
	default:	
		if (console == active_console)
			prtchr(key, scr->position_y, scr->position_x);

		screenbuffer_putchar(scr, key);
		scr->position_x++;
	}
	
	scr->position_y += (scr->position_x >= scr->size_x);
	
	if (scr->position_y >= scr->size_y) {
		scr->position_y = scr->size_y - 1;
		screenbuffer_clear_line(scr, scr->top_line);
		scr->top_line = (scr->top_line + 1) % scr->size_y;
		if (console == active_console)
			async_msg_1(fb_info.phone, FB_SCROLL, 1);
	}
	
	scr->position_x = scr->position_x % scr->size_x;
	
	if (console == active_console)
		curs_goto(scr->position_y, scr->position_x);
	
}

/** Switch to new console */
static void change_console(int newcons)
{
	connection_t *conn;
	int i, j, rc;
	keyfield_t *field;
	attrs_t *attrs;
	
	if (newcons == active_console)
		return;
	
	if (newcons == KERNEL_CONSOLE) {
		async_serialize_start();
		curs_hide_sync();
		gcons_in_kernel();
		async_serialize_end();
		
		if (__SYSCALL0(SYS_DEBUG_ENABLE_CONSOLE)) {
			prev_console = active_console;
			active_console = KERNEL_CONSOLE;
		} else
			newcons = active_console;
	}
	
	if (newcons != KERNEL_CONSOLE) {
		async_serialize_start();
		
		if (active_console == KERNEL_CONSOLE)
			gcons_redraw_console();
		
		active_console = newcons;
		gcons_change_console(newcons);
		conn = &connections[active_console];
		
		set_attrs(&conn->screenbuffer.attrs);
		curs_visibility(false);
		if (interbuffer) {
			for (i = 0; i < conn->screenbuffer.size_x; i++)
				for (j = 0; j < conn->screenbuffer.size_y; j++) {
					unsigned int size_x;
					
					size_x = conn->screenbuffer.size_x; 
					interbuffer[i + j * size_x] =
					    *get_field_at(&conn->screenbuffer, i, j);
				}
			/* This call can preempt, but we are already at the end */
			rc = async_req_0_0(fb_info.phone, FB_DRAW_TEXT_DATA);
		}
		
		if ((!interbuffer) || (rc != 0)) {
			set_attrs(&conn->screenbuffer.attrs);
			clrscr();
			attrs = &conn->screenbuffer.attrs;
			
			for (j = 0; j < conn->screenbuffer.size_y; j++)
				for (i = 0; i < conn->screenbuffer.size_x; i++) {
					field = get_field_at(&conn->screenbuffer, i, j);
					if (!attrs_same(*attrs, field->attrs))
						set_attrs(&field->attrs);
					attrs = &field->attrs;
					if ((field->character == ' ') &&
					    (attrs_same(field->attrs,
					    conn->screenbuffer.attrs)))
						continue;

					prtchr(field->character, j, i);
				}
		}
		
		curs_goto(conn->screenbuffer.position_y,
		    conn->screenbuffer.position_x);
		curs_visibility(conn->screenbuffer.is_cursor_visible);
		
		async_serialize_end();
	}
}

/** Handler for keyboard */
static void keyboard_events(ipc_callid_t iid, ipc_call_t *icall)
{
	ipc_callid_t callid;
	ipc_call_t call;
	int retval;
	int c;
	connection_t *conn;
	int newcon;
	
	/* Ignore parameters, the connection is alread opened */
	while (1) {
		callid = async_get_call(&call);
		switch (IPC_GET_METHOD(call)) {
		case IPC_M_PHONE_HUNGUP:
			/* TODO: Handle hangup */
			return;
		case KBD_MS_LEFT:
			newcon = gcons_mouse_btn(IPC_GET_ARG1(call));
			if (newcon != -1)
				change_console(newcon);
			retval = 0;
			break;
		case KBD_MS_MOVE:
			gcons_mouse_move(IPC_GET_ARG1(call),
			    IPC_GET_ARG2(call));
			retval = 0;
			break;
		case KBD_PUSHCHAR:
			/* got key from keyboard driver */
			retval = 0;
			c = IPC_GET_ARG1(call);
			/* switch to another virtual console */
			
			conn = &connections[active_console];
/*
 *			if ((c >= KBD_KEY_F1) && (c < KBD_KEY_F1 +
 *				CONSOLE_COUNT)) {
 */
			if ((c >= 0x101) && (c < 0x101 + CONSOLE_COUNT)) {
				if (c == 0x112)
					change_console(KERNEL_CONSOLE);
				else
					change_console(c - 0x101);
				break;
			}
			
			/* if client is awaiting key, send it */
			if (conn->keyrequest_counter > 0) {		
				conn->keyrequest_counter--;
				ipc_answer_1(fifo_pop(conn->keyrequests), EOK,
				    c);
				break;
			}
			
			keybuffer_push(&conn->keybuffer, c);
			retval = 0;
			
			break;
		default:
			retval = ENOENT;
		}
		ipc_answer_0(callid, retval);
	}
}

/** Default thread for new connections */
static void client_connection(ipc_callid_t iid, ipc_call_t *icall)
{
	ipc_callid_t callid;
	ipc_call_t call;
	int consnum;
	ipcarg_t arg1, arg2, arg3;
	connection_t *conn;
	
	if ((consnum = find_free_connection()) == -1) {
		ipc_answer_0(iid, ELIMIT);
		return;
	}
	conn = &connections[consnum];
	conn->used = 1;
	
	async_serialize_start();
	gcons_notify_connect(consnum);
	conn->client_phone = IPC_GET_ARG5(*icall);
	screenbuffer_clear(&conn->screenbuffer);
	
	/* Accept the connection */
	ipc_answer_0(iid, EOK);
	
	while (1) {
		async_serialize_end();
		callid = async_get_call(&call);
		async_serialize_start();
		
		arg1 = 0;
		arg2 = 0;
		switch (IPC_GET_METHOD(call)) {
		case IPC_M_PHONE_HUNGUP:
			gcons_notify_disconnect(consnum);
			
			/* Answer all pending requests */
			while (conn->keyrequest_counter > 0) {
				conn->keyrequest_counter--;
				ipc_answer_0(fifo_pop(conn->keyrequests),
				    ENOENT);
				break;
			}
			conn->used = 0;
			return;
		case CONSOLE_PUTCHAR:
			write_char(consnum, IPC_GET_ARG1(call));
			gcons_notify_char(consnum);
			break;
		case CONSOLE_CLEAR:
			/* Send message to fb */
			if (consnum == active_console) {
				async_msg_0(fb_info.phone, FB_CLEAR); 
			}
			
			screenbuffer_clear(&conn->screenbuffer);
			
			break;
		case CONSOLE_GOTO:
			screenbuffer_goto(&conn->screenbuffer,
			    IPC_GET_ARG2(call), IPC_GET_ARG1(call));
			if (consnum == active_console)
				curs_goto(IPC_GET_ARG1(call),
				    IPC_GET_ARG2(call));
			break;
		case CONSOLE_GETSIZE:
			arg1 = fb_info.rows;
			arg2 = fb_info.cols;
			break;
		case CONSOLE_FLUSH:
			if (consnum == active_console)
				async_req_0_0(fb_info.phone, FB_FLUSH);
			break;
		case CONSOLE_SET_STYLE:
			arg1 = IPC_GET_ARG1(call);
			screenbuffer_set_style(&conn->screenbuffer, arg1);
			if (consnum == active_console)
				set_style(arg1);
			break;
		case CONSOLE_SET_COLOR:
			arg1 = IPC_GET_ARG1(call);
			arg2 = IPC_GET_ARG2(call);
			arg3 = IPC_GET_ARG3(call);
			screenbuffer_set_color(&conn->screenbuffer, arg1,
			    arg2, arg3);
			if (consnum == active_console)
				set_color(arg1, arg2, arg3);
			break;
		case CONSOLE_SET_RGB_COLOR:
			arg1 = IPC_GET_ARG1(call);
			arg2 = IPC_GET_ARG2(call);
			screenbuffer_set_rgb_color(&conn->screenbuffer, arg1,
			    arg2);
			if (consnum == active_console)
				set_rgb_color(arg1, arg2);
			break;
		case CONSOLE_CURSOR_VISIBILITY:
			arg1 = IPC_GET_ARG1(call);
			conn->screenbuffer.is_cursor_visible = arg1;
			if (consnum == active_console)
				curs_visibility(arg1);
			break;
		case CONSOLE_GETCHAR:
			if (keybuffer_empty(&conn->keybuffer)) {
				/* buffer is empty -> store request */
				if (conn->keyrequest_counter <
					MAX_KEYREQUESTS_BUFFERED) {	
					fifo_push(conn->keyrequests, callid);
					conn->keyrequest_counter++;
				} else {
					/*
					 * No key available and too many
					 * requests => fail.
					*/
					ipc_answer_0(callid, ELIMIT);
				}
				continue;
			}
			int ch;
			keybuffer_pop(&conn->keybuffer, &ch);
			arg1 = ch;
			break;
		}
		ipc_answer_2(callid, EOK, arg1, arg2);
	}
}

static void interrupt_received(ipc_callid_t callid, ipc_call_t *call)
{
	change_console(prev_console);
}

int main(int argc, char *argv[])
{
	printf(NAME ": HelenOS Console service\n");
	
	ipcarg_t phonehash;
	int kbd_phone;
	size_t ib_size;
	int i;
	
	async_set_client_connection(client_connection);
	
	/* Connect to keyboard driver */
	kbd_phone = ipc_connect_me_to(PHONE_NS, SERVICE_KEYBOARD, 0, 0);
	while (kbd_phone < 0) {
		usleep(10000);
		kbd_phone = ipc_connect_me_to(PHONE_NS, SERVICE_KEYBOARD, 0, 0);
	}
	
	if (ipc_connect_to_me(kbd_phone, SERVICE_CONSOLE, 0, 0, &phonehash) != 0)
		return -1;
	async_new_connection(phonehash, 0, NULL, keyboard_events);
	
	/* Connect to framebuffer driver */
	fb_info.phone = ipc_connect_me_to(PHONE_NS, SERVICE_VIDEO, 0, 0);
	while (fb_info.phone < 0) {
		usleep(10000);
		fb_info.phone = ipc_connect_me_to(PHONE_NS, SERVICE_VIDEO, 0, 0);
	}
	
	/* Disable kernel output to the console */
	__SYSCALL0(SYS_DEBUG_DISABLE_CONSOLE);
	
	/* Initialize gcons */
	gcons_init(fb_info.phone);
	/* Synchronize, the gcons can have something in queue */
	async_req_0_0(fb_info.phone, FB_FLUSH);
	
	async_req_0_2(fb_info.phone, FB_GET_CSIZE, &fb_info.rows,
	    &fb_info.cols); 
	set_rgb_color(DEFAULT_FOREGROUND, DEFAULT_BACKGROUND);
	clrscr();
	
	/* Init virtual consoles */
	for (i = 0; i < CONSOLE_COUNT; i++) {
		connections[i].used = 0;
		keybuffer_init(&connections[i].keybuffer);
		
		connections[i].keyrequests.head = 0;
		connections[i].keyrequests.tail = 0;
		connections[i].keyrequests.items = MAX_KEYREQUESTS_BUFFERED;
		connections[i].keyrequest_counter = 0;
		
		if (screenbuffer_init(&connections[i].screenbuffer,
		    fb_info.cols, fb_info.rows) == NULL) {
			/* FIXME: handle error */
			return -1;
		}
	}
	connections[KERNEL_CONSOLE].used = 1;

	/* Set up shared memory buffer. */
	ib_size = sizeof(keyfield_t) * fb_info.cols * fb_info.rows;
	interbuffer = as_get_mappable_page(ib_size);

	if (as_area_create(interbuffer, ib_size, AS_AREA_READ |
	    AS_AREA_WRITE | AS_AREA_CACHEABLE) != interbuffer) {
		interbuffer = NULL;
	}

	if (interbuffer) {
		if (ipc_share_out_start(fb_info.phone, interbuffer,
		    AS_AREA_READ) != EOK) {
			as_area_destroy(interbuffer);
			interbuffer = NULL;
		}
	}
	
	curs_goto(0, 0);
	curs_visibility(
	    connections[active_console].screenbuffer.is_cursor_visible);
	
	/* Register at NS */
	if (ipc_connect_to_me(PHONE_NS, SERVICE_CONSOLE, 0, 0, &phonehash) != 0)
		return -1;
	
	/* Receive kernel notifications */
	if (sysinfo_value("kconsole.present")) {
		int devno = sysinfo_value("kconsole.devno");
		int inr = sysinfo_value("kconsole.inr");
		if (ipc_register_irq(inr, devno, 0, NULL) != EOK)
			printf(NAME ": Error registering kconsole notifications\n");
		
		async_set_interrupt_received(interrupt_received);
	}
	
	// FIXME: avoid connectiong to itself, keep using klog
	// printf(NAME ": Accepting connections\n");
	async_manager();
	
	return 0;
}

/** @}
 */
