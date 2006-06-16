/*
 * Copyright (C) 2006 Ondrej Palkovsky
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

/** @addtogroup ippc IPC Tester
 * @brief	IPC tester and task faulter.
 * @{
 */ 
/**
 * @file
 */

#include <stdio.h>
#include <async.h>
#include <ipc/ipc.h>
#include <ipc/services.h>
#include <errno.h>

#define TEST_START       10000
#define MAXLIST          4

#define MSG_HANG_ME_UP   2000

static int connections[50];
static ipc_callid_t callids[50];
static int phones[20];
static int myservice = 0;

static void client_connection(ipc_callid_t iid, ipc_call_t *icall)
{
	ipc_callid_t callid;
	ipc_call_t call;
	ipcarg_t phonehash = icall->in_phone_hash;
	int retval;
	int i;

	printf("Connected phone: %P, accepting\n", icall->in_phone_hash);
	ipc_answer_fast(iid, 0, 0, 0);
	for (i=0;i < 1024;i++)
		if (!connections[i]) {
			connections[i] = phonehash;
			break;
		}
	
	while (1) {
		callid = async_get_call(&call);
		switch (IPC_GET_METHOD(call)) {
		case IPC_M_PHONE_HUNGUP:
			printf("Phone (%P) hung up.\n", phonehash);
			retval = 0;
			break;
		default:
			printf("Received message from %P: %X\n", phonehash,callid);
			for (i=0; i < 1024; i++)
				if (!callids[i]) {
					callids[i] = callid;
					break;
				}
			continue;
		}
		ipc_answer_fast(callid, retval, 0, 0);
	}
}

static void printhelp(void)
{
	printf("? - help\n");
	printf("c - connect to other service\n");
	printf("h - hangup connection\n");
	printf("a - send async message to other service\n");
	printf("s - send sync message to other service\n");
	printf("d - answer message that we have received\n");
	printf("j - jump to endless loop\n");
	printf("p - page fault\n");
	printf("u - unaligned read\n");
}

static void callback(void *private, int retval, ipc_call_t *data)
{
	printf("Received response to msg %d - retval: %d.\n", private,
	       retval);
}

static void do_answer_msg(void)
{
	int i,cnt, errn = 0;
	char c;

	cnt = 0;
	for (i=0;i < 50;i++) {
		if (callids[i]) {
			printf("%d: %P\n", cnt, callids[i]);
			cnt++;
		}
		if (cnt >= 10)
			break;
	}
	if (!cnt)
		return;
	printf("Choose message:\n");
	do {
		c = getchar();
	} while (c < '0' || (c-'0') >= cnt);
	cnt = c - '0' + 1;
	
	for (i=0;cnt;i++)
		if (callids[i])
			cnt--;
	i -= 1;

	printf("Normal (n) or hangup (h) or error(e) message?\n");
	do {
		c = getchar();
	} while (c != 'n' && c != 'h' && c != 'e');
	if (c == 'n')
		errn = 0;
	else if (c == 'h')
		errn = EHANGUP;
	else if (c == 'e')
		errn = ENOENT;
	printf("Answering %P\n", callids[i]);
	ipc_answer_fast(callids[i], errn, 0, 0);
	callids[i] = 0;
}

static void do_send_msg(int async)
{
	int phoneid;
	int res;
	static int msgid = 1;
	char c;

	printf("Select phoneid to send msg: 2-9\n");
	do {
		c = getchar();
	} while (c < '2' || c > '9');
	phoneid = c - '0';

	if (async) {
		ipc_call_async(phoneid, 2000, 0, (void *)msgid, callback, 1);
		printf("Async sent - msg %d\n", msgid);
		msgid++;
	} else {
		printf("Sending msg...");
		res = ipc_call_sync_2(phoneid, 2000, 0, 0, NULL, NULL);
		printf("done: %d\n", res);
	}
}

static void do_hangup(void)
{
	char c;
	int res;
	int phoneid;

	printf("Select phoneid to hangup: 2-9\n");
	do {
		c = getchar();
	} while (c < '2' || c > '9');
	phoneid = c - '0';
	
	printf("Hanging up...");
	res = ipc_hangup(phoneid);
	printf("done: %d\n", phoneid);
}

static void do_connect(void)
{
	char c;
	int svc;
	int phid;

	printf("Choose one service: 0:10000....9:10009\n");
	do {
		c = getchar();
	} while (c < '0' || c > '9');
	svc = TEST_START + c - '0';
	if (svc == myservice) {
		printf("Currently cannot connect to myself, update test\n");
		return;
	}
	printf("Connecting to %d..", svc);
	phid = ipc_connect_me_to(PHONE_NS, svc, 0);
	if (phid > 0) {
		printf("phoneid: %d\n", phid);
		phones[phid] = 1;
	} else
		printf("error: %d\n", phid);
}



int main(void)
{
	ipcarg_t phonead;
	int i;
	char c;
	int res;
	volatile long long var;
	volatile int var1;
	
	printf("********************************\n");
	printf("***********IPC Tester***********\n");
	printf("********************************\n");

	
	async_set_client_connection(client_connection);

	for (i=TEST_START;i < TEST_START+10;i++) {
		res = ipc_connect_to_me(PHONE_NS, i, 0, &phonead);
		if (!res)
			break;
		printf("Failed registering as %d..:%d\n", i, res);
	}
	printf("Registered as service: %d\n", i);
	myservice = i;

	printhelp();
	while (1) {
		c = getchar();
		switch (c) {
		case '?':
			printhelp();
			break;
		case 'h':
			do_hangup();
			break;
		case 'c':
			do_connect();
			break;
		case 'a':
			do_send_msg(1);
			break;
		case 's':
			do_send_msg(0);
			break;
		case 'd':
			do_answer_msg();
			break;
		case 'j':
			while (1)
				;
		case 'p':
			printf("Doing page fault\n");
			*((char *)0) = 1;
			printf("done\n");
		case 'u':
			var1=*( (int *) ( ( (char *)(&var) ) + 1 ) );
			break;
		}
	}
}

/** @}
 */

