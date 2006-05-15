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

/**
 * @file	ns.c
 * @brief	Naming service for HelenOS IPC.
 */

#include <ipc.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <ns.h>
#include <errno.h>
#include <assert.h>
#include <libadt/list.h>
#include <libadt/hash_table.h>

#define NAME	"NS"

#define NS_HASH_TABLE_CHAINS	20

/* Static functions implementing NS hash table operations. */
static hash_index_t ns_hash(unsigned long *key);
static int ns_compare(unsigned long *key, hash_count_t keys, link_t *item);

/** Operations for NS hash table. */
static hash_table_operations_t ns_hash_table_ops = {
	.hash = ns_hash,
	.compare = ns_compare,
	.remove_callback = NULL
};

/** NS hash table structure. */
static hash_table_t ns_hash_table;

/** NS hash table item. */
typedef struct {
	link_t link;
	ipcarg_t service;		/**< Number of the service. */
	ipcarg_t phone;			/**< Phone registered with the service. */
} hashed_service_t;

/*
irq_cmd_t msim_cmds[1] = {
	{ CMD_MEM_READ_1, (void *)0xB0000000, 0 }
};

irq_code_t msim_kbd = {
	1,
	msim_cmds
};
*/
/*
irq_cmd_t i8042_cmds[1] = {
	{ CMD_PORT_READ_1, (void *)0x60, 0 }
};

irq_code_t i8042_kbd = {
	1,
	i8042_cmds
};
*/


int static ping_phone;

int main(int argc, char **argv)
{
	ipc_call_t call;
	ipc_callid_t callid;
	char *as;
	
	ipcarg_t retval, arg1, arg2;

	printf("%s: Name service started.\n", NAME);
	
	if (!hash_table_create(&ns_hash_table, NS_HASH_TABLE_CHAINS, 1, &ns_hash_table_ops)) {
		printf("%s: cannot create hash table\n", NAME);
		return ENOMEM;
	}
	
	
//	ipc_register_irq(2, &msim_kbd);
//	ipc_register_irq(1, &i8042_kbd);
	while (1) {
		callid = ipc_wait_for_call(&call, 0);
		printf("NS: Call phone=%lX...", call.phoneid);
		switch (IPC_GET_METHOD(call)) {
		case IPC_M_AS_SEND:
			as = (char *)IPC_GET_ARG2(call);
			printf("Received as: %P, size:%d\n", as, IPC_GET_ARG3(call));
			retval = ipc_answer(callid, 0,(sysarg_t)(1024*1024), 0);
			if (!retval) {
				printf("Reading shared memory...");
				printf("Text: %s", as);
			} else
				printf("Failed answer: %d\n", retval);
			continue;
			break;
		case IPC_M_INTERRUPT:
			printf("GOT INTERRUPT: %c\n", IPC_GET_ARG2(call));
			break;
		case IPC_M_PHONE_HUNGUP:
			printf("Phone hung up.\n");
			retval = 0;
			break;
		case IPC_M_CONNECT_TO_ME:;
			/*
			 * Request for registration of a service.
			 */
			ipcarg_t service;
			ipcarg_t phone;
			hashed_service_t *hs;
			
			service = IPC_GET_ARG1(call);
			phone = IPC_GET_ARG3(call);
			printf("Registering service %d on phone %d...", service, phone);
			
			hs = (hashed_service_t *) malloc(sizeof(hashed_service_t));
			if (!hs) {
				printf("Failed to register service %d.\n", service);
			}
			
			link_initialize(&hs->link);
			hs->service = service;
			hs->phone = phone;
			hash_table_insert(&ns_hash_table, (unsigned long *) &service, &hs->link);
			
			ping_phone = phone;
			retval = 0;
			break;
		case IPC_M_CONNECT_ME_TO:
			printf("Connectme(%P)to: %zd\n",
			       IPC_GET_ARG3(call), IPC_GET_ARG1(call));
			retval = 0;
			break;
		case NS_PING:
			printf("Ping...%P %P\n", IPC_GET_ARG1(call),
			       IPC_GET_ARG2(call));
			retval = 0;
			arg1 = 0xdead;
			arg2 = 0xbeef;
			break;
		case NS_HANGUP:
			printf("Closing connection.\n");
			retval = EHANGUP;
			break;
		case NS_PING_SVC:
			printf("NS:Pinging service %d\n", ping_phone);
			ipc_call_sync(ping_phone, NS_PING, 0xbeef, 0);
			printf("NS:Got pong\n");
			break;
		default:
			printf("Unknown method: %zd\n", IPC_GET_METHOD(call));
			retval = ENOENT;
			break;
		}
		if (! (callid & IPC_CALLID_NOTIFICATION)) {
			printf("Answering.\n");
			ipc_answer(callid, retval, arg1, arg2);
		}
	}
}

/** Compute hash index into NS hash table.
 *
 * @param key Pointer to single key (i.e. service number).
 * @return Hash index corresponding to *key.
 */
hash_index_t ns_hash(unsigned long *key)
{
	assert(key);
	return *key % NS_HASH_TABLE_CHAINS;
}

/** Compare a key with hashed item.
 *
 * @param key Single key pointer.
 * @param keys Must be 1.
 * @param item Pointer to a hash table item.
 * @return Non-zero if the key matches the item, zero otherwise.
 */
int ns_compare(unsigned long *key, hash_count_t keys, link_t *item)
{
	hashed_service_t *hs;

	assert(key);
	assert(keys == 1);
	assert(item);
	
	hs = hash_table_get_instance(item, hashed_service_t, link);
	
	return *key == hs->service;
}
