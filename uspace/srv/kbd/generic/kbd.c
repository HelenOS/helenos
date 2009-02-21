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

/**
 * @addtogroup kbdgen generic
 * @brief	HelenOS generic uspace keyboard handler.
 * @ingroup  kbd
 * @{
 */ 
/** @file
 */

#include <ipc/ipc.h>
#include <ipc/services.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <ipc/ns.h>
#include <async.h>
#include <errno.h>
#include <libadt/fifo.h>
#include <kbd/kbd.h>
#include <kbd/keycode.h>

#include <kbd.h>
#include <key_buffer.h>
#include <kbd_port.h>
#include <kbd_ctl.h>
#include <layout.h>

#define NAME "kbd"

int cons_connected = 0;
int phone2cons = -1;
keybuffer_t keybuffer;

/** Currently active modifiers. */
static unsigned mods = KM_NUM_LOCK;

/** Currently pressed lock keys. We track these to tackle autorepeat. */
static unsigned lock_keys;

void kbd_push_scancode(int scancode)
{
/*	printf("scancode: 0x%x\n", scancode);*/
	kbd_ctl_parse_scancode(scancode);
}

void kbd_push_ev(int type, unsigned int key)
{
	kbd_event_t ev;
	unsigned mod_mask;

	switch (key) {
	case KC_LCTRL: mod_mask = KM_LCTRL; break;
	case KC_RCTRL: mod_mask = KM_RCTRL; break;
	case KC_LSHIFT: mod_mask = KM_LSHIFT; break;
	case KC_RSHIFT: mod_mask = KM_RSHIFT; break;
	case KC_LALT: mod_mask = KM_LALT; break;
	case KC_RALT: mod_mask = KM_RALT; break;
	default: mod_mask = 0; break;
	}

	if (mod_mask != 0) {
		if (type == KE_PRESS)
			mods = mods | mod_mask;
		else
			mods = mods & ~mod_mask;
	}

	switch (key) {
	case KC_CAPS_LOCK: mod_mask = KM_CAPS_LOCK; break;
	case KC_NUM_LOCK: mod_mask = KM_NUM_LOCK; break;
	case KC_SCROLL_LOCK: mod_mask = KM_SCROLL_LOCK; break;
	default: mod_mask = 0; break;
	}

	if (mod_mask != 0) {
		if (type == KE_PRESS) {
			/*
			 * Only change lock state on transition from released
			 * to pressed. This prevents autorepeat from messing
			 * up the lock state.
			 */
			mods = mods ^ (mod_mask & ~lock_keys);
			lock_keys = lock_keys | mod_mask;
		} else {
			lock_keys = lock_keys & ~mod_mask;
		}
	}
/*
	printf("type: %d\n", type);
	printf("mods: 0x%x\n", mods);
	printf("keycode: %u\n", key);
*/
	ev.type = type;
	ev.key = key;
	ev.mods = mods;

	ev.c = layout_parse_ev(&ev);

	async_msg_4(phone2cons, KBD_EVENT, ev.type, ev.key, ev.mods, ev.c);
}

static void console_connection(ipc_callid_t iid, ipc_call_t *icall)
{
	ipc_callid_t callid;
	ipc_call_t call;
	int retval;

	if (cons_connected) {
		ipc_answer_0(iid, ELIMIT);
		return;
	}
	cons_connected = 1;
	ipc_answer_0(iid, EOK);

	while (1) {
		callid = async_get_call(&call);
		switch (IPC_GET_METHOD(call)) {
		case IPC_M_PHONE_HUNGUP:
			cons_connected = 0;
			ipc_hangup(phone2cons);
			phone2cons = -1;
			ipc_answer_0(callid, EOK);
			return;
		case IPC_M_CONNECT_TO_ME:
			if (phone2cons != -1) {
				retval = ELIMIT;
				break;
			}
			phone2cons = IPC_GET_ARG5(call);
			retval = 0;
			break;
		default:
			retval = EINVAL;
		}
		ipc_answer_0(callid, retval);
	}	
}


int main(int argc, char **argv)
{
	printf(NAME ": HelenOS Keyboard service\n");
	
	ipcarg_t phonead;
	
	/* Initialize port driver. */
	if (kbd_port_init())
		return -1;
	
	/* Initialize key buffer */
	keybuffer_init(&keybuffer);
	
	async_set_client_connection(console_connection);

	/* Register service at nameserver. */
	if (ipc_connect_to_me(PHONE_NS, SERVICE_KEYBOARD, 0, 0, &phonead) != 0)
		return -1;
	
	printf(NAME ": Accepting connections\n");
	async_manager();

	/* Not reached. */
	return 0;
}

/**
 * @}
 */ 
