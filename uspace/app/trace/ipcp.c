/*
 * Copyright (c) 2008 Jiri Svoboda
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

/** @addtogroup trace
 * @{
 */
/** @file
 */

#include <stdio.h>
#include <stdlib.h>
#include <libadt/hash_table.h>

#include "ipc_desc.h"
#include "proto.h"
#include "ipcp.h"

#define IPCP_CALLID_SYNC 0

typedef struct {
	int phone_hash;
	ipc_call_t question;

	int call_hash;

	link_t link;
} pending_call_t;

typedef struct {
	int server;
	proto_t *proto;
} connection_t;

#define MAX_PHONE 64
connection_t connections[MAX_PHONE];
int have_conn[MAX_PHONE];

#define PCALL_TABLE_CHAINS 32
hash_table_t pending_calls;

static hash_index_t pending_call_hash(unsigned long key[]);
static int pending_call_compare(unsigned long key[], hash_count_t keys,
    link_t *item);
static void pending_call_remove_callback(link_t *item);

hash_table_operations_t pending_call_ops = {
	.hash = pending_call_hash,
	.compare = pending_call_compare,
	.remove_callback = pending_call_remove_callback
};


static hash_index_t pending_call_hash(unsigned long key[])
{
//	printf("pending_call_hash\n");
	return key[0] % PCALL_TABLE_CHAINS;
}

static int pending_call_compare(unsigned long key[], hash_count_t keys,
    link_t *item)
{
	pending_call_t *hs;

//	printf("pending_call_compare\n");
	hs = hash_table_get_instance(item, pending_call_t, link);

	return key[0] == hs->call_hash;
}

static void pending_call_remove_callback(link_t *item)
{
//	printf("pending_call_remove_callback\n");
}


void ipcp_connection_set(int phone, int server, proto_t *proto)
{
	if (phone <0 || phone >= MAX_PHONE) return;
	connections[phone].server = server;
	connections[phone].proto = proto;
	have_conn[phone] = 1;
}

void ipcp_connection_clear(int phone)
{
	have_conn[phone] = 0;
	connections[phone].server = 0;
	connections[phone].proto = NULL;
}

static void ipc_m_print(proto_t *proto, ipcarg_t method)
{
	ipc_m_desc_t *desc;
	oper_t *oper;

	/* FIXME: too slow */
	desc = ipc_methods;
	while (desc->number != 0) {
		if (desc->number == method) {
			printf("%s (%d)", desc->name, method);
			return;
		}

		++desc;
	}

	if (proto != NULL) {
		oper = proto_get_oper(proto, method);
		if (oper != NULL) {
			printf("%s (%d)", oper->name, method);
			return;
		}
	}

	printf("%d", method);
}

void ipcp_init(void)
{
	hash_table_create(&pending_calls, PCALL_TABLE_CHAINS, 1, &pending_call_ops);
}

void ipcp_cleanup(void)
{
	hash_table_destroy(&pending_calls);
}

void ipcp_call_out(int phone, ipc_call_t *call, ipc_callid_t hash)
{
	pending_call_t *pcall;
	proto_t *proto;
	unsigned long key[1];

	if (have_conn[phone]) proto = connections[phone].proto;
	else proto = NULL;

//	printf("ipcp_call_out()\n");
	printf("call id: 0x%x, phone: %d, proto: %s, method: ", hash, phone,
		(proto ? proto->name : "n/a"));
	ipc_m_print(proto, IPC_GET_METHOD(*call));
	printf(" args: (%u, %u, %u, %u, %u)\n",
	    IPC_GET_ARG1(*call),
	    IPC_GET_ARG2(*call),
	    IPC_GET_ARG3(*call),
	    IPC_GET_ARG4(*call),
	    IPC_GET_ARG5(*call)
	);

	/* Store call in hash table for response matching */

	pcall = malloc(sizeof(pending_call_t));
	pcall->phone_hash = phone;
	pcall->question = *call;
	pcall->call_hash = hash;

	key[0] = hash;

	hash_table_insert(&pending_calls, key, &pcall->link);
}

static void parse_answer(pending_call_t *pcall, ipc_call_t *answer)
{
	int phone;
	ipcarg_t method;
	ipcarg_t service;
	int retval;
	static proto_t proto_unknown = { .name = "unknown" };
	proto_t *proto;
	int cphone;

//	printf("parse_answer\n");

	phone = pcall->phone_hash;
	method = IPC_GET_METHOD(pcall->question);
	retval = IPC_GET_RETVAL(*answer);
	printf("phone=%d, method=%d, retval=%d\n",
		phone, method, retval);

	if (phone == 0 && method == IPC_M_CONNECT_ME_TO && retval == 0) {
		/* Connected to a service (through NS) */
		service = IPC_GET_ARG1(pcall->question);
		proto = proto_get_by_srv(service);
		if (proto == NULL) proto = &proto_unknown;

		cphone = IPC_GET_ARG5(*answer);
		printf("registering connection (phone %d, protocol: %s)\n", cphone,
			proto->name);
		ipcp_connection_set(cphone, 0, proto);
	}
}

void ipcp_call_in(ipc_call_t *call, ipc_callid_t hash)
{
	link_t *item;
	pending_call_t *pcall;
	unsigned long key[1];

//	printf("ipcp_call_in()\n");
/*	printf("phone: %d, method: ", call->in_phone_hash);
	ipc_m_print(IPC_GET_METHOD(*call));
	printf(" args: (%u, %u, %u, %u, %u)\n",
	    IPC_GET_ARG1(*call),
	    IPC_GET_ARG2(*call),
	    IPC_GET_ARG3(*call),
	    IPC_GET_ARG4(*call),
	    IPC_GET_ARG5(*call)
	);*/

	if ((hash & IPC_CALLID_ANSWERED) == 0 && hash != IPCP_CALLID_SYNC) {
		/* Not a response */
		printf("Not a response (hash %d)\n", hash);
		return;
	}

	hash = hash & ~IPC_CALLID_ANSWERED;
	key[0] = hash;

	item = hash_table_find(&pending_calls, key);
	if (item == NULL) return; // No matching question found
	
	pcall = hash_table_get_instance(item, pending_call_t, link);

	printf("response matched to question\n");
	hash_table_remove(&pending_calls, key, 1);

	parse_answer(pcall, call);
	free(pcall);
}

void ipcp_call_sync(int phone, ipc_call_t *call, ipc_call_t *answer)
{
	ipcp_call_out(phone, call, IPCP_CALLID_SYNC);
	ipcp_call_in(answer, IPCP_CALLID_SYNC);
}

void ipcp_hangup(int phone, int rc)
{
	printf("hangup phone %d -> %d\n", phone, rc);
	ipcp_connection_clear(phone);
}

/** @}
 */
