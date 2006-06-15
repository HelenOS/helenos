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

/** @defgroup kbd Keyboard handler
 * @brief	HelenOS uspace keyboard handler.
 * @{ 
 * @}
 */
/** @addtogroup kbdgen generic
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
#include <errno.h>
#include <arch/kbd.h>
#include <kbd.h>
#include <libadt/fifo.h>
#include <key_buffer.h>
#include <async.h>
#include <keys.h>

#define NAME "KBD"

int cons_connected = 0;
int phone2cons = -1;
keybuffer_t keybuffer;	

static void irq_handler(ipc_callid_t iid, ipc_call_t *call)
{
	int chr;

#ifdef MOUSE_ENABLED
	if (mouse_arch_process(phone2cons, call))
		return;
#endif
	
	kbd_arch_process(&keybuffer, call);

	if (cons_connected && phone2cons != -1) {
		/* recode to ASCII - one interrupt can produce more than one code so result is stored in fifo */
		while (!keybuffer_empty(&keybuffer)) {
			if (!keybuffer_pop(&keybuffer, (int *)&chr))
				break;

			async_msg(phone2cons, KBD_PUSHCHAR, chr);
		}
	}
}

static void console_connection(ipc_callid_t iid, ipc_call_t *icall)
{
	ipc_callid_t callid;
	ipc_call_t call;
	int retval;

	if (cons_connected) {
		ipc_answer_fast(iid, ELIMIT, 0, 0);
		return;
	}
	cons_connected = 1;
	ipc_answer_fast(iid, 0, 0, 0);

	while (1) {
		callid = async_get_call(&call);
		switch (IPC_GET_METHOD(call)) {
		case IPC_M_PHONE_HUNGUP:
			cons_connected = 0;
			ipc_hangup(phone2cons);
			phone2cons = -1;
			ipc_answer_fast(callid, 0,0,0);
			return;
		case IPC_M_CONNECT_TO_ME:
			if (phone2cons != -1) {
				retval = ELIMIT;
				break;
			}
			phone2cons = IPC_GET_ARG3(call);
			retval = 0;
			break;
		}
		ipc_answer_fast(callid, retval, 0, 0);
	}	
}


int main(int argc, char **argv)
{
	ipc_call_t call;
	ipc_callid_t callid;
	int res;
	ipcarg_t phonead;
	ipcarg_t phoneid;
	char connected = 0;
	ipcarg_t retval, arg1, arg2;
	
	/* Initialize arch dependent parts */
	if (kbd_arch_init())
		return -1;
	
	/* Initialize key buffer */
	keybuffer_init(&keybuffer);
	
	async_set_client_connection(console_connection);
	async_set_interrupt_received(irq_handler);
	/* Register service at nameserver */
	if ((res = ipc_connect_to_me(PHONE_NS, SERVICE_KEYBOARD, 0, &phonead)) != 0) {
		return -1;
	}

	async_manager();

}

/**
 * @}
 */ 

