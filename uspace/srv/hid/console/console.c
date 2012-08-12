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
#include <ipc/input.h>
#include <ipc/console.h>
#include <ipc/vfs.h>
#include <errno.h>
#include <str_error.h>
#include <loc.h>
#include <event.h>
#include <io/keycode.h>
#include <screenbuffer.h>
#include <fb.h>
#include <imgmap.h>
#include <align.h>
#include <malloc.h>
#include <as.h>
#include <fibril_synch.h>
#include "images.h"
#include "console.h"

#define NAME       "console"
#define NAMESPACE  "term"

#define CONSOLE_TOP     66
#define CONSOLE_MARGIN  12

#define STATE_START   100
#define STATE_TOP     8
#define STATE_SPACE   4
#define STATE_WIDTH   48
#define STATE_HEIGHT  48

typedef enum {
	CONS_DISCONNECTED = 0,
	CONS_DISCONNECTED_SELECTED,
	CONS_SELECTED,
	CONS_IDLE,
	CONS_DATA,
	CONS_KERNEL,
	CONS_LAST
} console_state_t;

#define UTF8_CHAR_BUFFER_SIZE (STR_BOUNDS(1) + 1)

typedef struct {
	atomic_t refcnt;           /**< Connection reference count */
	prodcons_t input_pc;       /**< Incoming keyboard events */
	char char_remains[UTF8_CHAR_BUFFER_SIZE]; /**< Not yet sent bytes of last char event. */
	size_t char_remains_len;   /**< Number of not yet sent bytes. */
	
	fibril_mutex_t mtx;        /**< Lock protecting mutable fields */
	
	size_t index;              /**< Console index */
	console_state_t state;     /**< Console state */
	service_id_t dsid;         /**< Service handle */
	
	vp_handle_t state_vp;      /**< State icon viewport */
	sysarg_t cols;             /**< Number of columns */
	sysarg_t rows;             /**< Number of rows */
	console_caps_t ccaps;      /**< Console capabilities */
	
	screenbuffer_t *frontbuf;  /**< Front buffer */
	frontbuf_handle_t fbid;    /**< Front buffer handle */
} console_t;

typedef enum {
	GRAPHICS_NONE = 0,
	GRAPHICS_BASIC = 1,
	GRAPHICS_FULL = 2
} graphics_state_t;

/** Current console state */
static graphics_state_t graphics_state = GRAPHICS_NONE;

/** State icons */
static imagemap_handle_t state_icons[CONS_LAST];

/** Session to the input server */
static async_sess_t *input_sess;

/** Session to the framebuffer server */
static async_sess_t *fb_sess;

/** Framebuffer resolution */
static sysarg_t xres;
static sysarg_t yres;

/** Array of data for virtual consoles */
static console_t consoles[CONSOLE_COUNT];

/** Mutex for console switching */
static FIBRIL_MUTEX_INITIALIZE(switch_mtx);

static console_t *prev_console = &consoles[0];
static console_t *active_console = &consoles[0];
static console_t *kernel_console = &consoles[KERNEL_CONSOLE];

static imgmap_t *logo_img;
static imgmap_t *nameic_img;

static imgmap_t *anim_1_img;
static imgmap_t *anim_2_img;
static imgmap_t *anim_3_img;
static imgmap_t *anim_4_img;

static imagemap_handle_t anim_1;
static imagemap_handle_t anim_2;
static imagemap_handle_t anim_3;
static imagemap_handle_t anim_4;

static sequence_handle_t anim_seq;

static imgmap_t *cons_data_img;
static imgmap_t *cons_dis_img;
static imgmap_t *cons_dis_sel_img;
static imgmap_t *cons_idle_img;
static imgmap_t *cons_kernel_img;
static imgmap_t *cons_sel_img;

static vp_handle_t logo_vp;
static imagemap_handle_t logo_handle;

static vp_handle_t nameic_vp;
static imagemap_handle_t nameic_handle;

static vp_handle_t screen_vp;
static vp_handle_t console_vp;

struct {
	sysarg_t x;
	sysarg_t y;
	
	sysarg_t btn_x;
	sysarg_t btn_y;
	
	bool pressed;
} mouse;

static void cons_redraw_state(console_t *cons)
{
	if (graphics_state == GRAPHICS_FULL) {
		fibril_mutex_lock(&cons->mtx);
		
		fb_vp_imagemap_damage(fb_sess, cons->state_vp,
		    state_icons[cons->state], 0, 0, STATE_WIDTH, STATE_HEIGHT);
		
		if ((cons->state != CONS_DISCONNECTED) &&
		    (cons->state != CONS_KERNEL) &&
		    (cons->state != CONS_DISCONNECTED_SELECTED)) {
			char data[5];
			snprintf(data, 5, "%zu", cons->index + 1);
			
			for (size_t i = 0; data[i] != 0; i++)
				fb_vp_putchar(fb_sess, cons->state_vp, i + 2, 1, data[i]);
		}
		
		fibril_mutex_unlock(&cons->mtx);
	}
}

static void cons_kernel_sequence_start(console_t *cons)
{
	if (graphics_state == GRAPHICS_FULL) {
		fibril_mutex_lock(&cons->mtx);
		
		fb_vp_sequence_start(fb_sess, cons->state_vp, anim_seq);
		fb_vp_imagemap_damage(fb_sess, cons->state_vp,
		    state_icons[cons->state], 0, 0, STATE_WIDTH, STATE_HEIGHT);
		
		fibril_mutex_unlock(&cons->mtx);
	}
}

static void cons_update_state(console_t *cons, console_state_t state)
{
	bool update = false;
	
	fibril_mutex_lock(&cons->mtx);
	
	if (cons->state != state) {
		cons->state = state;
		update = true;
	}
	
	fibril_mutex_unlock(&cons->mtx);
	
	if (update)
		cons_redraw_state(cons);
}

static void cons_notify_data(console_t *cons)
{
	fibril_mutex_lock(&switch_mtx);
	
	if (cons != active_console)
		cons_update_state(cons, CONS_DATA);
	
	fibril_mutex_unlock(&switch_mtx);
}

static void cons_notify_connect(console_t *cons)
{
	fibril_mutex_lock(&switch_mtx);
	
	if (cons == active_console)
		cons_update_state(cons, CONS_SELECTED);
	else
		cons_update_state(cons, CONS_IDLE);
	
	fibril_mutex_unlock(&switch_mtx);
}

static void cons_notify_disconnect(console_t *cons)
{
	fibril_mutex_lock(&switch_mtx);
	
	if (cons == active_console)
		cons_update_state(cons, CONS_DISCONNECTED_SELECTED);
	else
		cons_update_state(cons, CONS_DISCONNECTED);
	
	fibril_mutex_unlock(&switch_mtx);
}

static void cons_update(console_t *cons)
{
	fibril_mutex_lock(&switch_mtx);
	fibril_mutex_lock(&cons->mtx);
	
	if ((cons == active_console) && (active_console != kernel_console)) {
		fb_vp_update(fb_sess, console_vp, cons->fbid);
		fb_vp_cursor_update(fb_sess, console_vp, cons->fbid);
	}
	
	fibril_mutex_unlock(&cons->mtx);
	fibril_mutex_unlock(&switch_mtx);
}

static void cons_update_cursor(console_t *cons)
{
	fibril_mutex_lock(&switch_mtx);
	fibril_mutex_lock(&cons->mtx);
	
	if ((cons == active_console) && (active_console != kernel_console))
		fb_vp_cursor_update(fb_sess, console_vp, cons->fbid);
	
	fibril_mutex_unlock(&cons->mtx);
	fibril_mutex_unlock(&switch_mtx);
}

static void cons_clear(console_t *cons)
{
	fibril_mutex_lock(&cons->mtx);
	screenbuffer_clear(cons->frontbuf);
	fibril_mutex_unlock(&cons->mtx);
	
	cons_update(cons);
}

static void cons_damage_all(console_t *cons)
{
	fibril_mutex_lock(&switch_mtx);
	fibril_mutex_lock(&cons->mtx);
	
	if ((cons == active_console) && (active_console != kernel_console)) {
		fb_vp_damage(fb_sess, console_vp, cons->fbid, 0, 0, cons->cols,
		    cons->rows);
		fb_vp_cursor_update(fb_sess, console_vp, cons->fbid);
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
		fb_yield(fb_sess);
		if (!console_kcon()) {
			fb_claim(fb_sess);
			fibril_mutex_unlock(&switch_mtx);
			return;
		}
	}
	
	if (active_console == kernel_console)
		fb_claim(fb_sess);
	
	prev_console = active_console;
	active_console = cons;
	
	if (prev_console->state == CONS_DISCONNECTED_SELECTED)
		cons_update_state(prev_console, CONS_DISCONNECTED);
	else
		cons_update_state(prev_console, CONS_IDLE);
	
	if ((cons->state == CONS_DISCONNECTED) ||
	    (cons->state == CONS_DISCONNECTED_SELECTED))
		cons_update_state(cons, CONS_DISCONNECTED_SELECTED);
	else
		cons_update_state(cons, CONS_SELECTED);
	
	fibril_mutex_unlock(&switch_mtx);
	
	cons_damage_all(cons);
}

static console_t *cons_get_active_uspace(void)
{
	fibril_mutex_lock(&switch_mtx);

	console_t *active_uspace = active_console;
	if (active_uspace == kernel_console) {
		active_uspace = prev_console;
	}
	assert(active_uspace != kernel_console);

	fibril_mutex_unlock(&switch_mtx);

	return active_uspace;
}

static ssize_t limit(ssize_t val, ssize_t lo, ssize_t hi)
{
	if (val > hi)
		return hi;
	
	if (val < lo)
		return lo;
	
	return val;
}

static void cons_mouse_move(sysarg_t dx, sysarg_t dy)
{
	ssize_t sx = (ssize_t) dx;
	ssize_t sy = (ssize_t) dy;
	
	mouse.x = limit(mouse.x + sx, 0, xres);
	mouse.y = limit(mouse.y + sy, 0, yres);
	
	fb_pointer_update(fb_sess, mouse.x, mouse.y, true);
}

static console_t *cons_find_icon(sysarg_t x, sysarg_t y)
{
	sysarg_t status_start =
	    STATE_START + (xres - 800) / 2 + CONSOLE_MARGIN;
	
	if ((y < STATE_TOP) || (y >= STATE_TOP + STATE_HEIGHT))
		return NULL;
	
	if (x < status_start)
		return NULL;
	
	if (x >= status_start + (STATE_WIDTH + STATE_SPACE) * CONSOLE_COUNT)
		return NULL;
	
	if (((x - status_start) % (STATE_WIDTH + STATE_SPACE)) >= STATE_WIDTH)
		return NULL;
	
	sysarg_t btn = (x - status_start) / (STATE_WIDTH + STATE_SPACE);
	
	if (btn < CONSOLE_COUNT)
		return consoles + btn;
	
	return NULL;
}

/** Handle mouse click
 *
 * @param state Button state (true - pressed, false - depressed)
 *
 */
static console_t *cons_mouse_button(bool state)
{
	if (graphics_state != GRAPHICS_FULL)
		return NULL;
	
	if (state) {
		console_t *cons = cons_find_icon(mouse.x, mouse.y);
		if (cons != NULL) {
			mouse.btn_x = mouse.x;
			mouse.btn_y = mouse.y;
			mouse.pressed = true;
		}
		
		return NULL;
	}
	
	if ((!state) && (!mouse.pressed))
		return NULL;
	
	console_t *cons = cons_find_icon(mouse.x, mouse.y);
	if (cons == cons_find_icon(mouse.btn_x, mouse.btn_y))
		return cons;
	
	mouse.pressed = false;
	return NULL;
}

static void input_events(ipc_callid_t iid, ipc_call_t *icall, void *arg)
{
	/* Ignore parameters, the connection is already opened */
	while (true) {
		ipc_call_t call;
		ipc_callid_t callid = async_get_call(&call);
		
		if (!IPC_GET_IMETHOD(call)) {
			/* TODO: Handle hangup */
			async_hangup(input_sess);
			return;
		}
		
		kbd_event_type_t type;
		keycode_t key;
		keymod_t mods;
		wchar_t c;
		
		switch (IPC_GET_IMETHOD(call)) {
		case INPUT_EVENT_KEY:
			type = IPC_GET_ARG1(call);
			key = IPC_GET_ARG2(call);
			mods = IPC_GET_ARG3(call);
			c = IPC_GET_ARG4(call);
			
			if ((key >= KC_F1) && (key < KC_F1 + CONSOLE_COUNT) &&
			    ((mods & KM_CTRL) == 0))
				cons_switch(&consoles[key - KC_F1]);
			else {
				/* Got key press/release event */
				kbd_event_t *event =
				    (kbd_event_t *) malloc(sizeof(kbd_event_t));
				if (event == NULL) {
					async_answer_0(callid, ENOMEM);
					break;
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
			
			async_answer_0(callid, EOK);
			break;
		case INPUT_EVENT_MOVE:
			cons_mouse_move(IPC_GET_ARG1(call), IPC_GET_ARG2(call));
			async_answer_0(callid, EOK);
			break;
		case INPUT_EVENT_BUTTON:
			/* Got pointer button press/release event */
			if (IPC_GET_ARG1(call) == 1) {
				console_t *cons =
				    cons_mouse_button((bool) IPC_GET_ARG2(call));
				if (cons != NULL)
					cons_switch(cons);
			}
			async_answer_0(callid, EOK);
			break;
		default:
			async_answer_0(callid, EINVAL);
		}
	}
}

/** Process a character from the client (TTY emulation). */
static void cons_write_char(console_t *cons, wchar_t ch)
{
	sysarg_t updated = 0;
	
	fibril_mutex_lock(&cons->mtx);
	
	switch (ch) {
	case '\n':
		updated = screenbuffer_newline(cons->frontbuf);
		break;
	case '\r':
		break;
	case '\t':
		updated = screenbuffer_tabstop(cons->frontbuf, 8);
		break;
	case '\b':
		updated = screenbuffer_backspace(cons->frontbuf);
		break;
	default:
		updated = screenbuffer_putchar(cons->frontbuf, ch, true);
	}
	
	fibril_mutex_unlock(&cons->mtx);
	
	if (updated > 1)
		cons_update(cons);
}

static void cons_set_cursor(console_t *cons, sysarg_t col, sysarg_t row)
{
	fibril_mutex_lock(&cons->mtx);
	screenbuffer_set_cursor(cons->frontbuf, col, row);
	fibril_mutex_unlock(&cons->mtx);
	
	cons_update_cursor(cons);
}

static void cons_set_cursor_visibility(console_t *cons, bool visible)
{
	fibril_mutex_lock(&cons->mtx);
	screenbuffer_set_cursor_visibility(cons->frontbuf, visible);
	fibril_mutex_unlock(&cons->mtx);
	
	cons_update_cursor(cons);
}

static void cons_get_cursor(console_t *cons, ipc_callid_t iid, ipc_call_t *icall)
{
	sysarg_t col;
	sysarg_t row;
	
	fibril_mutex_lock(&cons->mtx);
	screenbuffer_get_cursor(cons->frontbuf, &col, &row);
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
	
	cons_notify_data(cons);
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
	screenbuffer_set_style(cons->frontbuf, style);
	fibril_mutex_unlock(&cons->mtx);
}

static void cons_set_color(console_t *cons, console_color_t bgcolor,
    console_color_t fgcolor, console_color_attr_t attr)
{
	fibril_mutex_lock(&cons->mtx);
	screenbuffer_set_color(cons->frontbuf, bgcolor, fgcolor, attr);
	fibril_mutex_unlock(&cons->mtx);
}

static void cons_set_rgb_color(console_t *cons, pixel_t bgcolor,
    pixel_t fgcolor)
{
	fibril_mutex_lock(&cons->mtx);
	screenbuffer_set_rgb_color(cons->frontbuf, bgcolor, fgcolor);
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
	
	if (atomic_postinc(&cons->refcnt) == 0) {
		cons_set_cursor_visibility(cons, true);
		cons_notify_connect(cons);
	}
	
	/* Accept the connection */
	async_answer_0(iid, EOK);
	
	while (true) {
		ipc_call_t call;
		ipc_callid_t callid = async_get_call(&call);
		
		if (!IPC_GET_IMETHOD(call)) {
			if (atomic_postdec(&cons->refcnt) == 1)
				cons_notify_disconnect(cons);
			
			return;
		}
		
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

static async_sess_t *input_connect(const char *svc)
{
	async_sess_t *sess;
	service_id_t dsid;
	
	int rc = loc_service_get_id(svc, &dsid, 0);
	if (rc == EOK) {
		sess = loc_service_connect(EXCHANGE_ATOMIC, dsid, 0);
		if (sess == NULL) {
			printf("%s: Unable to connect to input service %s\n", NAME,
			    svc);
			return NULL;
		}
	} else
		return NULL;
	
	async_exch_t *exch = async_exchange_begin(sess);
	rc = async_connect_to_me(exch, 0, 0, 0, input_events, NULL);
	async_exchange_end(exch);
	
	if (rc != EOK) {
		async_hangup(sess);
		printf("%s: Unable to create callback connection to service %s (%s)\n",
		    NAME, svc, str_error(rc));
		return NULL;
	}
	
	return sess;
}

static void interrupt_received(ipc_callid_t callid, ipc_call_t *call)
{
	cons_switch(prev_console);
}

static async_sess_t *fb_connect(const char *svc)
{
	async_sess_t *sess;
	service_id_t dsid;
	
	int rc = loc_service_get_id(svc, &dsid, 0);
	if (rc == EOK) {
		sess = loc_service_connect(EXCHANGE_SERIALIZE, dsid, 0);
		if (sess == NULL) {
			printf("%s: Unable to connect to framebuffer service %s\n",
			    NAME, svc);
			return NULL;
		}
	} else
		return NULL;
	
	return sess;
}

static bool console_srv_init(char *input_svc, char *fb_svc)
{
	/* Avoid double initialization */
	if (graphics_state != GRAPHICS_NONE)
		return false;
	
	/* Connect to input service */
	input_sess = input_connect(input_svc);
	if (input_sess == NULL)
		return false;
	
	/* Connect to framebuffer service */
	fb_sess = fb_connect(fb_svc);
	if (fb_sess == NULL)
		return false;
	
	/* Register server */
	async_set_client_connection(client_connection);
	int rc = loc_server_register(NAME);
	if (rc != EOK) {
		printf("%s: Unable to register server (%s)\n", NAME,
		    str_error(rc));
		return false;
	}
	
	fb_get_resolution(fb_sess, &xres, &yres);
	
	/* Initialize the screen */
	screen_vp = fb_vp_create(fb_sess, 0, 0, xres, yres);
	
	if ((xres >= 800) && (yres >= 600)) {
		logo_vp = fb_vp_create(fb_sess, xres - 66, 2, 64, 60);
		logo_img = imgmap_decode_tga((void *) helenos_tga,
		    helenos_tga_size, IMGMAP_FLAG_SHARED);
		logo_handle = fb_imagemap_create(fb_sess, logo_img);
		
		nameic_vp = fb_vp_create(fb_sess, 5, 17, 100, 26);
		nameic_img = imgmap_decode_tga((void *) nameic_tga,
		    nameic_tga_size, IMGMAP_FLAG_SHARED);
		nameic_handle = fb_imagemap_create(fb_sess, nameic_img);
		
		cons_data_img = imgmap_decode_tga((void *) cons_data_tga,
		    cons_data_tga_size, IMGMAP_FLAG_SHARED);
		cons_dis_img = imgmap_decode_tga((void *) cons_dis_tga,
		    cons_dis_tga_size, IMGMAP_FLAG_SHARED);
		cons_dis_sel_img = imgmap_decode_tga((void *) cons_dis_sel_tga,
		    cons_dis_sel_tga_size, IMGMAP_FLAG_SHARED);
		cons_idle_img = imgmap_decode_tga((void *) cons_idle_tga,
		    cons_idle_tga_size, IMGMAP_FLAG_SHARED);
		cons_kernel_img = imgmap_decode_tga((void *) cons_kernel_tga,
		    cons_kernel_tga_size, IMGMAP_FLAG_SHARED);
		cons_sel_img = imgmap_decode_tga((void *) cons_sel_tga,
		    cons_sel_tga_size, IMGMAP_FLAG_SHARED);
		
		state_icons[CONS_DISCONNECTED] =
		    fb_imagemap_create(fb_sess, cons_dis_img);
		state_icons[CONS_DISCONNECTED_SELECTED] =
		    fb_imagemap_create(fb_sess, cons_dis_sel_img);
		state_icons[CONS_SELECTED] =
		    fb_imagemap_create(fb_sess, cons_sel_img);
		state_icons[CONS_IDLE] =
		    fb_imagemap_create(fb_sess, cons_idle_img);
		state_icons[CONS_DATA] =
		    fb_imagemap_create(fb_sess, cons_data_img);
		state_icons[CONS_KERNEL] =
		    fb_imagemap_create(fb_sess, cons_kernel_img);
		
		anim_1_img = imgmap_decode_tga((void *) anim_1_tga,
		    anim_1_tga_size, IMGMAP_FLAG_SHARED);
		anim_2_img = imgmap_decode_tga((void *) anim_2_tga,
		    anim_2_tga_size, IMGMAP_FLAG_SHARED);
		anim_3_img = imgmap_decode_tga((void *) anim_3_tga,
		    anim_3_tga_size, IMGMAP_FLAG_SHARED);
		anim_4_img = imgmap_decode_tga((void *) anim_4_tga,
		    anim_4_tga_size, IMGMAP_FLAG_SHARED);
		
		anim_1 = fb_imagemap_create(fb_sess, anim_1_img);
		anim_2 = fb_imagemap_create(fb_sess, anim_2_img);
		anim_3 = fb_imagemap_create(fb_sess, anim_3_img);
		anim_4 = fb_imagemap_create(fb_sess, anim_4_img);
		
		anim_seq = fb_sequence_create(fb_sess);
		fb_sequence_add_imagemap(fb_sess, anim_seq, anim_1);
		fb_sequence_add_imagemap(fb_sess, anim_seq, anim_2);
		fb_sequence_add_imagemap(fb_sess, anim_seq, anim_3);
		fb_sequence_add_imagemap(fb_sess, anim_seq, anim_4);
		
		console_vp = fb_vp_create(fb_sess, CONSOLE_MARGIN, CONSOLE_TOP,
		    xres - 2 * CONSOLE_MARGIN, yres - (CONSOLE_TOP + CONSOLE_MARGIN));
		
		fb_vp_clear(fb_sess, screen_vp);
		fb_vp_imagemap_damage(fb_sess, logo_vp, logo_handle,
		    0, 0, 64, 60);
		fb_vp_imagemap_damage(fb_sess, nameic_vp, nameic_handle,
		    0, 0, 100, 26);
		
		graphics_state = GRAPHICS_FULL;
	} else {
		console_vp = screen_vp;
		graphics_state = GRAPHICS_BASIC;
	}
	
	fb_vp_set_style(fb_sess, console_vp, STYLE_NORMAL);
	fb_vp_clear(fb_sess, console_vp);
	
	sysarg_t cols;
	sysarg_t rows;
	fb_vp_get_dimensions(fb_sess, console_vp, &cols, &rows);
	
	console_caps_t ccaps;
	fb_vp_get_caps(fb_sess, console_vp, &ccaps);
	
	mouse.x = xres / 2;
	mouse.y = yres / 2;
	mouse.pressed = false;
	
	/* Inititalize consoles */
	for (size_t i = 0; i < CONSOLE_COUNT; i++) {
		consoles[i].index = i;
		atomic_set(&consoles[i].refcnt, 0);
		fibril_mutex_initialize(&consoles[i].mtx);
		prodcons_initialize(&consoles[i].input_pc);
		consoles[i].char_remains_len = 0;
		
		if (graphics_state == GRAPHICS_FULL) {
			/* Create state buttons */
			consoles[i].state_vp =
			    fb_vp_create(fb_sess, STATE_START + (xres - 800) / 2 +
			    CONSOLE_MARGIN + i * (STATE_WIDTH + STATE_SPACE),
			    STATE_TOP, STATE_WIDTH, STATE_HEIGHT);
		}
		
		if (i == KERNEL_CONSOLE) {
			consoles[i].state = CONS_KERNEL;
			cons_redraw_state(&consoles[i]);
			cons_kernel_sequence_start(&consoles[i]);
			continue;
		}
		
		if (i == 0)
			consoles[i].state = CONS_DISCONNECTED_SELECTED;
		else
			consoles[i].state = CONS_DISCONNECTED;
		
		consoles[i].cols = cols;
		consoles[i].rows = rows;
		consoles[i].ccaps = ccaps;
		consoles[i].frontbuf =
		    screenbuffer_create(cols, rows, SCREENBUFFER_FLAG_SHARED);
		
		if (consoles[i].frontbuf == NULL) {
			printf("%s: Unable to allocate frontbuffer %zu\n", NAME, i);
			return false;
		}
		
		consoles[i].fbid = fb_frontbuf_create(fb_sess, consoles[i].frontbuf);
		if (consoles[i].fbid == 0) {
			printf("%s: Unable to create frontbuffer %zu\n", NAME, i);
			return false;
		}
		
		cons_redraw_state(&consoles[i]);
		
		char vc[LOC_NAME_MAXLEN + 1];
		snprintf(vc, LOC_NAME_MAXLEN, "%s/vc%zu", NAMESPACE, i);
		
		if (loc_service_register(vc, &consoles[i].dsid) != EOK) {
			printf("%s: Unable to register device %s\n", NAME, vc);
			return false;
		}
	}
	
	/* Receive kernel notifications */
	async_set_interrupt_received(interrupt_received);
	rc = event_subscribe(EVENT_KCONSOLE, 0);
	if (rc != EOK)
		printf("%s: Failed to register kconsole notifications (%s)\n",
		    NAME, str_error(rc));
	
	return true;
}

static void usage(void)
{
	printf("Usage: console <input_dev> <framebuffer_dev>\n");
}

int main(int argc, char *argv[])
{
	if (argc < 3) {
		usage();
		return -1;
	}
	
	printf("%s: HelenOS Console service\n", NAME);
	
	if (!console_srv_init(argv[1], argv[2]))
		return -1;
	
	printf("%s: Accepting connections\n", NAME);
	task_retval(0);
	async_manager();
	
	return 0;
}

/** @}
 */
