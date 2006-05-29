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
#include <ipc/ipc.h>
#include <ipc/services.h>
#include <stdio.h>
#include <errno.h>

#define NAME "CONSOLE"

int main(int argc, char *argv[])
{
	ipcarg_t phonead;
	ipc_call_t call;
	ipc_callid_t callid;
	int phone_kbd, phone_fb;
	ipcarg_t retval, arg1 = 0xdead, arg2 = 0xbeef;
	
	printf("Uspace console service started.\n");
	
	
	/* Connect to keyboard driver */

	while ((phone_kbd = ipc_connect_me_to(PHONE_NS, SERVICE_KEYBOARD, 0)) < 0) {
	};
	
	if (ipc_connect_to_me(phone_kbd, SERVICE_CONSOLE, 0, &phonead) != 0) {
		printf("%s: Error: Registering at naming service failed.\n", NAME);
		return -1;
	};

	/* Connect to framebuffer driver */
	
	while ((phone_fb = ipc_connect_me_to(PHONE_NS, SERVICE_VIDEO, 0)) < 0) {
	};
	
	
	/* Register service at nameserver */
	printf("%s: Registering at naming service.\n", NAME);

	if (ipc_connect_to_me(PHONE_NS, SERVICE_CONSOLE, 0, &phonead) != 0) {
		printf("%s: Error: Registering at naming service failed.\n", NAME);
		return -1;
	};
	
	while (1) {
		callid = ipc_wait_for_call(&call);
		switch (IPC_GET_METHOD(call)) {
			case IPC_M_PHONE_HUNGUP:
				printf("%s: Phone hung up.\n", NAME);
				retval = 0;
				break;
			case IPC_M_CONNECT_ME_TO:
				printf("%s: Connect me (%P) to: %zd\n",NAME, IPC_GET_ARG3(call), IPC_GET_ARG1(call));
				retval = 0;
				break;
			case KBD_PUSHCHAR:
				printf("%s: Push char '%c'.\n", NAME, IPC_GET_ARG1(call));
				retval = 0;
				
				break;
			default:
				printf("%s: Unknown method: %zd\n", NAME, IPC_GET_METHOD(call));
				retval = ENOENT;
				break;
		}

		if (! (callid & IPC_CALLID_NOTIFICATION)) {
			ipc_answer_fast(callid, retval, arg1, arg2);
		}
	}

	return 0;	
}
