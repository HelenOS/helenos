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

static int register_service(ipcarg_t service, ipcarg_t phone, ipc_call_t *call);
static int connect_to_service(ipcarg_t service, ipc_call_t *call, ipc_callid_t callid);

/* Static functions implementing NS hash table operations. */
static hash_index_t ns_hash(unsigned long *key);
static int ns_compare(unsigned long *key, hash_count_t keys, link_t *item);
static void ns_remove(link_t *item);

/** Operations for NS hash table. */
static hash_table_operations_t ns_hash_table_ops = {
	.hash = ns_hash,
	.compare = ns_compare,
	.remove_callback = ns_remove
};

/** NS hash table structure. */
static hash_table_t ns_hash_table;

/** NS hash table item. */
typedef struct {
	link_t link;
	ipcarg_t service;		/**< Number of the service. */
	ipcarg_t phone;			/**< Phone registered with the service. */
	ipcarg_t in_phone_hash;		/**< Incoming phone hash. */
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

	printf("%s: Naming service started.\n", NAME);
	
	if (!hash_table_create(&ns_hash_table, NS_HASH_TABLE_CHAINS, 3, &ns_hash_table_ops)) {
		printf("%s: cannot create hash table\n", NAME);
		return ENOMEM;
	}
	
	
//	ipc_register_irq(2, &msim_kbd);
//	ipc_register_irq(1, &i8042_kbd);
	while (1) {
		callid = ipc_wait_for_call(&call, 0);
		printf("NS: Call in_phone_hash=%lX...", call.in_phone_hash);
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
		case IPC_M_CONNECT_TO_ME:
			/*
			 * Server requests service registration.
			 */
			retval = register_service(IPC_GET_ARG1(call), IPC_GET_ARG3(call), &call);
			ping_phone = IPC_GET_ARG3(call);
			break;
		case IPC_M_CONNECT_ME_TO:
			/*
			 * Client requests to be connected to a service.
			 */
			retval = connect_to_service(IPC_GET_ARG1(call), &call, callid);
			break;
		case NS_HANGUP:
			printf("Closing connection.\n");
			retval = EHANGUP;
			break;
		case NS_PING:
			printf("Ping...%P %P\n", IPC_GET_ARG1(call),
			       IPC_GET_ARG2(call));
			retval = 0;
			arg1 = 0xdead;
			arg2 = 0xbeef;
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

/** Register service.
 *
 * @param service Service to be registered.
 * @param phone phone Phone to be used for connections to the service.
 * @param call Pointer to call structure.
 *
 * @return Zero on success or a value from @ref errno.h.
 */
int register_service(ipcarg_t service, ipcarg_t phone, ipc_call_t *call)
{
	unsigned long keys[3] = { service, call->in_phone_hash, 0 };
	hashed_service_t *hs;
			
	printf("Registering service %d on phone %d...", service, phone);

	if (hash_table_find(&ns_hash_table, keys)) {
		printf("Service %d already registered.\n", service);
		return EEXISTS;
	}
			
	hs = (hashed_service_t *) malloc(sizeof(hashed_service_t));
	if (!hs) {
		printf("Failed to register service %d.\n", service);
		return ENOMEM;
	}
			
	link_initialize(&hs->link);
	hs->service = service;
	hs->phone = phone;
	hs->in_phone_hash = call->in_phone_hash;
	hash_table_insert(&ns_hash_table, keys, &hs->link);
			
	return 0;
}

/** Connect client to service.
 *
 * @param service Service to be connected to.
 * @param call Pointer to call structure.
 * @param callid Call ID of the request.
 *
 * @return Zero on success or a value from @ref errno.h.
 */
int connect_to_service(ipcarg_t service, ipc_call_t *call, ipc_callid_t callid)
{
	unsigned long keys[3] = { service, 0, 0 };
	link_t *hlp;
	hashed_service_t *hs;
			
	hlp = hash_table_find(&ns_hash_table, keys);
	if (!hlp) {
		printf("Service %d not registered.\n", service);
		return ENOENT;
	}
	hs = hash_table_get_instance(hlp, hashed_service_t, link);
	printf("Connecting in_phone_hash=%lX to service at phone %d...", call->in_phone_hash, hs->phone);
	return ipc_forward_fast(callid, hs->phone, 0, 0);
}

/** Compute hash index into NS hash table.
 *
 * @param key Pointer keys. However, only the first key (i.e. service number)
 * 	      is used to compute the hash index.
 * @return Hash index corresponding to key[0].
 */
hash_index_t ns_hash(unsigned long *key)
{
	assert(key);
	return *key % NS_HASH_TABLE_CHAINS;
}

/** Compare a key with hashed item.
 *
 * This compare function always ignores the third key.
 * It exists only to make it possible to remove records
 * originating from connection with key[1] in_phone_hash
 * value. Note that this is close to being classified
 * as a nasty hack.
 *
 * @param key Array of keys.
 * @param keys Must be lesser or equal to 3.
 * @param item Pointer to a hash table item.
 * @return Non-zero if the key matches the item, zero otherwise.
 */
int ns_compare(unsigned long key[], hash_count_t keys, link_t *item)
{
	hashed_service_t *hs;

	assert(key);
	assert(keys <= 3);
	assert(item);
	
	hs = hash_table_get_instance(item, hashed_service_t, link);
	
	if (keys == 2)
		return key[1] == hs->in_phone_hash;
	else
		return key[0] == hs->service;
}

/** Perform actions after removal of item from the hash table.
 *
 * @param item Item that was removed from the hash table.
 */
void ns_remove(link_t *item)
{
	assert(item);
	free(hash_table_get_instance(item, hashed_service_t, link));
}
