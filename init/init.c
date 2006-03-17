/*
 * Copyright (C) 2005 Martin Decky
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

#include "version.h"
#include <ipc.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <ns.h>
#include <thread.h>

extern void utest(void *arg);
void utest(void *arg)
{
//	printf("Uspace thread created.\n");
	for (;;)
		;
}

static void test_printf(void)
{
	printf("Simple text.\n");
	printf("Now insert '%s' string.\n","this");
	printf("We are brave enought to print numbers like %%d = '%d'\n", 0x123456);
	printf("And now... '%b' byte! '%w' word! '%W' Word! \n", 0x12, 0x1234, 0x1234);
	printf("'%Q' Q! Another '%q' q! \n", 0x1234567887654321ll, 0x1234567887654321ll);
	printf("'%Q' with 64bit value and '%p' with 32 bit value. \n", 0x1234567887654321ll, 0x12345678 );
	printf("'%Q' 64bit, '%p' 32bit, '%b' 8bit, '%w' 16bit, '%Q' 64bit and '%s' string.\n", 0x1234567887654321ll, 0x12345678, 0x12, 0x1234, 0x1234567887654321ull, "Lovely string" );
	
	printf("Thats all, folks!\n");
}


extern char _heap;
static void test_mremap(void)
{
	printf("Writing to good memory\n");
	mremap(&_heap, 120000, 0);
	printf("%P\n", ((char *)&_heap));
	printf("%P\n", ((char *)&_heap) + 80000);
	*(((char *)&_heap) + 80000) = 10;
	printf("Making small\n");
	mremap(&_heap, 16000, 0);
	printf("Failing..\n");
	*((&_heap) + 80000) = 10;

	printf("memory done\n");
}
/*
static void test_sbrk(void)
{
	printf("Writing to good memory\n");
	printf("Got: %P\n", sbrk(120000));
	printf("%P\n", ((char *)&_heap));
	printf("%P\n", ((char *)&_heap) + 80000);
	*(((char *)&_heap) + 80000) = 10;
	printf("Making small, got: %P\n",sbrk(-120000));
	printf("Testing access to heap\n");
	_heap = 10;
	printf("Failing..\n");
	*((&_heap) + 80000) = 10;

	printf("memory done\n");
}
*/
/*
static void test_malloc(void)
{
	char *data;

	data = malloc(10);
	printf("Heap: %P, data: %P\n", &_heap, data);
	data[0] = 'a';
	free(data);
}
*/


static void test_ping(void)
{
	ipcarg_t result;
	int retval;

	retval = ipc_call_sync(PHONE_NS, NS_PING, 0xbeef,&result);
	printf("Retval: %d - received: %P\n", retval, result);
}

static void got_answer(void *private, int retval, ipc_data_t *data)
{
	printf("Retval: %d...%s...%X, %X\n", retval, private,
	       IPC_GET_ARG1(*data), IPC_GET_ARG2(*data));
}
static void test_async_ipc(void)
{
	ipc_call_t data;
	int i;

	printf("Sending ping\n");
	ipc_call_async_2(PHONE_NS, NS_PING, 1, 0xbeefbee2,
			 "Pong1", got_answer);
	ipc_call_async_2(PHONE_NS, NS_PING, 2, 0xbeefbee4, 
			 "Pong2", got_answer);
	ipc_call_async_2(PHONE_NS, NS_PING, 3, 0xbeefbee4, 
			 "Pong3", got_answer);
	ipc_call_async_2(PHONE_NS, NS_PING, 4, 0xbeefbee4, 
			 "Pong4", got_answer);
	ipc_call_async_2(PHONE_NS, NS_PING, 5, 0xbeefbee4, 
			 "Pong5", got_answer);
	ipc_call_async_2(PHONE_NS, NS_PING, 6, 0xbeefbee4, 
			 "Pong6", got_answer);
	printf("Waiting forever...\n");
	for (i=0; i<100;i++)
		printf(".");
	printf("\n");
	ipc_wait_for_call(&data, NULL);
	printf("Received call???\n");
}


static void got_answer_2(void *private, int retval, ipc_data_t *data)
{
	printf("Pong\n");
}
static void test_advanced_ipc(void)
{
	int res;
	unsigned long long taskid;
	ipc_callid_t callid;
	ipc_call_t data;
	int i;

	printf("Asking 0 to connect to me...\n");
	res = ipc_connect_to_me(0, 1, 2, &taskid);
	printf("Result: %d - taskid: %Q\n", res, taskid);
	for (i=0; i < 100; i++) {
		printf("----------------\n");
		ipc_call_async(PHONE_NS, NS_PING_SVC, 0, "prov",
			       got_answer_2);
		callid = ipc_wait_for_call(&data, NULL);
		printf("Received ping\n");
		ipc_answer(callid, 0, 0, 0);
	}
	callid = ipc_wait_for_call(&data, NULL);
}

static void test_connection_ipc(void)
{
	int res;
	ipcarg_t result;

	printf("Starting connect...\n");
	res = ipc_connect_me_to(PHONE_NS, 10, 20);
	printf("Connected: %d\n", res);
	printf("pinging.\n");
	res = ipc_call_sync(res, NS_PING, 0xbeef,&result);
	printf("Retval: %d - received: %P\n", res, result);
	
}

int main(int argc, char *argv[])
{
	int tid;
	char *stack;
	version_print();

/*	test_printf(); */
//	test_ping();
//	test_async_ipc();
//	test_advanced_ipc();
	test_connection_ipc();
	
	stack = (char *) malloc(getpagesize());
	if (!stack) {
		printf("Malloc failed.\n");
	} else {
		if ((tid = thread_create(utest, NULL, stack, "utest") != -1)) {
			printf("Created thread tid=%d\n", tid);
		}
	}
	
	return 0;
}
