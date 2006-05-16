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

#include <ipc.h>
#include <services.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <ns.h>
#include <errno.h>
#include <arch/kbd.h>
#include <kbd.h>
#include <key_buffer.h>
#include <fifo.h>

#define NAME "KBD"

#define KBD_REQUEST_MAX 32 /**< Maximum requests buffered until keypress */

int main(int argc, char **argv)
{
	ipc_call_t call;
	ipc_callid_t callid;
	char connected = 0;
	int res;
	int c;
	ipcarg_t phonead;
	
	ipcarg_t retval, arg1, arg2;
	
	fifo_count_t callers_counter = 0;
	
	FIFO_INITIALIZE_STATIC(callers_buffer, ipc_callid_t, KBD_REQUEST_MAX);

	printf("Uspace kbd service started.\n");

	/* Initialize arch dependent parts */
	if (!(res = kbd_arch_init())) {
			printf("Kbd registration failed with retval %d.\n", res);
			return -1;
			};
	
	/* Initialize key buffer */
	key_buffer_init();
	
	/* Register service at nameserver */
	printf("%s: Registering at naming service.\n", NAME);

	if ((res = ipc_connect_to_me(PHONE_NS, SERVICE_KEYBOARD, 0, &phonead)) != 0) {
		printf("%s: Error: Registering at naming service failed.\n", NAME);
		return -1;
	};
	
	while (1) {
		callid = ipc_wait_for_call(&call, 0);
	//	printf("%s:Call phone=%lX..", NAME, call.in_phone_hash);
		switch (IPC_GET_METHOD(call)) {
		case IPC_M_PHONE_HUNGUP:
			if (connected) {
				if (--connected == 0) {
					callers_counter = 0;
					callers_buffer.head = callers_buffer.tail = 0;
					key_buffer_free();	
				}
			}
			
			printf("%s: Phone hung up.\n", NAME);
			retval = 0;
			break;
		case IPC_M_CONNECT_TO_ME:
			printf("%s: Somebody connecting phid=%zd.\n", NAME, IPC_GET_ARG3(call));
			retval = 0;
			break;
		case IPC_M_CONNECT_ME_TO:
		//	printf("%s: Connect me (%P) to: %zd\n",NAME, IPC_GET_ARG3(call), IPC_GET_ARG1(call));
			if (connected) {
				retval = ELIMIT;
			} else {
				retval = 0;
				connected = 1;
			}
			break;
		case IPC_M_INTERRUPT:
			if (connected) {
				kbd_arch_process(IPC_GET_ARG2(call));
			}
			//printf("%s: GOT INTERRUPT: %c\n", NAME, IPC_GET_ARG2(call));
			if (!callers_counter)
				break;
			/* Small trick - interrupt does not need answer so we can change callid to caller awaiting key */
			callers_counter--;
			callid = fifo_pop(callers_buffer);
		case KBD_GETCHAR:
//			printf("%s: Getchar: ", NAME);
			retval = 0;
			arg1 = 0;	
			if (!key_buffer_pop((char *)&arg1)) {
				if (callers_counter < KBD_REQUEST_MAX) {
					callers_counter++;
					fifo_push(callers_buffer, callid);
				} else {
					retval = ELIMIT;
				}
				continue;
			};
			arg2 = 0xbeef;
		//	printf("GetChar return %c\n", arg1);
			
			break;
		default:
			printf("%s: Unknown method: %zd\n", NAME, IPC_GET_METHOD(call));
			retval = ENOENT;
			break;
		}
		if (! (callid & IPC_CALLID_NOTIFICATION)) {
		//	printf("%s: Answering\n", NAME);
			ipc_answer_fast(callid, retval, arg1, arg2);
		}
	}
}
