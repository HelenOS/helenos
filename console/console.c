/*
 * Copyright (C) 2006 Josef Cejka
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


#include <kbd.h>
#include <fb.h>
#include <ipc/ipc.h>
#include <ipc/fb.h>
#include <ipc/services.h>
#include <errno.h>
#include <key_buffer.h>
#include <console.h>
#include <unistd.h>
#include <async.h>

static void sysput(char c)
{
	__SYSCALL3(SYS_IO, 1, &c, 1);

}
//#define CONSOLE_COUNT VFB_CONNECTIONS
#define CONSOLE_COUNT 6

#define NAME "CONSOLE"

int active_client = 0;


typedef struct {
	keybuffer_t keybuffer;
	int client_phone;
	int vfb_number;	/* Not used */
	int vfb_phone;
	int used;
} connection_t;

connection_t connections[CONSOLE_COUNT];

static int find_free_connection() 
{
	int i = 0;
	
	while (i < CONSOLE_COUNT) {
		if (connections[i].used == 0)
			return i;
		++i;
	}
	return CONSOLE_COUNT;
}


static int find_connection(int client_phone) 
{
	int i = 0;
	
	while (i < CONSOLE_COUNT) {
		if (connections[i].client_phone == client_phone)
			return i;
		++i;
	}
	return  CONSOLE_COUNT;
}

/* Handler for keyboard */
static void keyboard_events(ipc_callid_t iid, ipc_call_t *icall)
{
	ipc_callid_t callid;
	ipc_call_t call;
	int retval;
	int i;

	/* Ignore parameters, the connection is alread opened */
	while (1) {
		callid = async_get_call(&call);
		switch (IPC_GET_METHOD(call)) {
		case IPC_M_PHONE_HUNGUP:
			ipc_answer_fast(callid,0,0,0);
			/* TODO: Handle hangup */
			return;
		case KBD_PUSHCHAR:
			/* got key from keyboard driver */
			
			/* find active console */
			
			/* if client is awaiting key, send it */
			
			/*FIXME: else store key to its buffer */
			retval = 0;
			i = IPC_GET_ARG1(call) & 0xff;
			/* switch to another virtual console */
			if ((i >= KBD_KEY_F1) && (i < KBD_KEY_F1 + CONSOLE_COUNT)) {
				active_client = i - KBD_KEY_F1;
				break;
			}
			keybuffer_push(&(connections[active_client].keybuffer), i);
			/* Send it to first FB, DEBUG */
//			ipc_call_async_2(connections[0].vfb_phone, FB_PUTCHAR, 0, IPC_GET_ARG1(call),NULL,NULL);
//			ipc_call_sync_2(connections[0].vfb_phone, FB_PUTCHAR, 0, IPC_GET_ARG1(call),NULL,NULL);

			break;
		default:
			retval = ENOENT;
		}		
		ipc_answer_fast(callid, retval, 0, 0);
	}
}

/** Default thread for new connections */
void client_connection(ipc_callid_t iid, ipc_call_t *icall)
{
	ipc_callid_t callid;
	ipc_call_t call;
	int consnum;
	ipcarg_t arg1;

	if ((consnum = find_free_connection()) == CONSOLE_COUNT) {
		ipc_answer_fast(iid,ELIMIT,0,0);
		return;
	}
	connections[consnum].used = 1;
	connections[consnum].client_phone = IPC_GET_ARG3(call);

	/* Accept the connection */
	ipc_answer_fast(iid,0,0,0);
	
	while (1) {
		callid = async_get_call(&call);
		switch (IPC_GET_METHOD(call)) {
		case IPC_M_PHONE_HUNGUP:
			/* TODO */
			ipc_answer_fast(callid, 0,0,0);
			return;
		case CONSOLE_PUTCHAR:
			/* Send message to fb */
			ipc_call_async_2(connections[consnum].vfb_phone, FB_PUTCHAR, IPC_GET_ARG1(call), IPC_GET_ARG2(call), NULL, NULL); 
			break;
		case CONSOLE_GETCHAR:
			/* FIXME: Only temporary solution until request storage will be created  */
			while (!keybuffer_pop(&(connections[active_client].keybuffer), (char *)&arg1)) {
				/* FIXME: buffer empty -> store request */
				async_usleep(100000);
			};
			
			break;
		}
		ipc_answer_fast(callid, 0, arg1, 0);
	}
}

int main(int argc, char *argv[])
{
	ipcarg_t phonehash;
	int kbd_phone, fb_phone;
	ipcarg_t retval, arg1 = 0xdead, arg2 = 0xbeef;
	int i;
	
	/* Connect to keyboard driver */

	while ((kbd_phone = ipc_connect_me_to(PHONE_NS, SERVICE_KEYBOARD, 0)) < 0) {
		usleep(10000);
	};
	
	if (ipc_connect_to_me(kbd_phone, SERVICE_CONSOLE, 0, &phonehash) != 0) {
		return -1;
	};
	async_new_connection(phonehash, 0, NULL, keyboard_events);

	/* Connect to framebuffer driver */
	
	for (i = 0; i < CONSOLE_COUNT; i++) {
		connections[i].used = 0;
		keybuffer_init(&(connections[i].keybuffer));
		/* TODO: init key_buffer */
		while ((connections[i].vfb_phone = ipc_connect_me_to(PHONE_NS, SERVICE_VIDEO, 0)) < 0) {
			usleep(10000);
			//ipc_call_async_2(connections[i].vfb_phone, FB_PUTCHAR, 'a', 'b', NULL, (void *)NULL); 
		}
	}
	
	if (ipc_connect_to_me(PHONE_NS, SERVICE_CONSOLE, 0, &phonehash) != 0) {
		return -1;
	};
	
	async_manager();

	return 0;	
}
