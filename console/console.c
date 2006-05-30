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

//#define CONSOLE_COUNT VFB_CONNECTIONS
#define CONSOLE_COUNT 6

#define NAME "CONSOLE"

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

int main(int argc, char *argv[])
{
	ipcarg_t phonead;
	ipc_call_t call;
	ipc_callid_t callid;
	int kbd_phone, fb_phone;
	ipcarg_t retval, arg1 = 0xdead, arg2 = 0xbeef;
	int i;
	int active_client = 0;
	
	/* Connect to keyboard driver */

	while ((kbd_phone = ipc_connect_me_to(PHONE_NS, SERVICE_KEYBOARD, 0)) < 0) {
	};
	
	if (ipc_connect_to_me(kbd_phone, SERVICE_CONSOLE, 0, &phonead) != 0) {
		return -1;
	};

	/* Connect to framebuffer driver */
	
	for (i = 0; i < CONSOLE_COUNT; i++) {
		connections[i].used = 0;
		keybuffer_init(&(connections[i].keybuffer));
		/* TODO: init key_buffer */
		while ((connections[i].vfb_phone = ipc_connect_me_to(PHONE_NS, SERVICE_VIDEO, 0)) < 0) {
				
ipc_call_async_2(connections[i].vfb_phone, FB_PUTCHAR, 'a', 'b', NULL, (void *)NULL); 
		}
	}
	

	
	if (ipc_connect_to_me(PHONE_NS, SERVICE_CONSOLE, 0, &phonead) != 0) {
		return -1;
	};
	
	while (1) {
		callid = ipc_wait_for_call(&call);
		switch (IPC_GET_METHOD(call)) {
			case IPC_M_PHONE_HUNGUP:
				/*FIXME: if its fb or kbd then panic! */
				/* free connection */
				if (i = find_connection(IPC_GET_ARG3(call)) < CONSOLE_COUNT) {
					connections[i].used = 0;
					 /*TODO: free connection[i].key_buffer; */
					/* FIXME: active_connection hungup */
					retval = 0;
				} else {
					/*FIXME: No such connection */
				}
				break;
			case IPC_M_CONNECT_ME_TO:
				
				/* find first free connection */
				
				if ((i = find_free_connection()) == CONSOLE_COUNT) {
					retval = ELIMIT;
					break;
				}
				
				connections[i].used = 1;
				connections[i].client_phone = IPC_GET_ARG3(call);
				
				retval = 0;
				break;
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
				
				break;
			case CONSOLE_PUTCHAR:
				/* find sender client */
				/* ???
				 * if its active client, send it to vfb
				 **/
				/*FIXME: check, if its from active client, .... */

				if ((i = find_connection(IPC_GET_ARG3(call))) == CONSOLE_COUNT) {
					break;
				};
				
				/* TODO: send message to fb */
				ipc_call_async_2(connections[i].vfb_phone, FB_PUTCHAR, IPC_GET_ARG1(call), IPC_GET_ARG2(call), NULL, NULL); 
				
				break;
			case CONSOLE_GETCHAR:
				/* FIXME: */
				if (!keybuffer_pop(&(connections[active_client].keybuffer), (char *)&arg1)) {
					/* FIXME: buffer empty -> store request */
					arg1 = 'X'; /* Only temporary */
				};
//ipc_call_async_2(connections[active_client].vfb_phone, FB_PUTCHAR, ' ', arg1, NULL, (void *)NULL); 
				break;
			default:
				retval = ENOENT;
				break;
		}

		if (! (callid & IPC_CALLID_NOTIFICATION)) {
			ipc_answer_fast(callid, retval, arg1, arg2);
		}
	}

	return 0;	
}
