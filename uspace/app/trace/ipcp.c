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
#include <str_error.h>
#include <inttypes.h>
#include <adt/hash_table.h>
#include <abi/ipc/methods.h>
#include <macros.h>
#include "ipc_desc.h"
#include "proto.h"
#include "trace.h"
#include "ipcp.h"

typedef struct {
	cap_phone_handle_t phone_handle;
	ipc_call_t question;
	oper_t *oper;

	cap_call_handle_t call_handle;

	ht_link_t link;
} pending_call_t;

typedef struct {
	int server;
	proto_t *proto;
} connection_t;

#define MAX_PHONE 64
connection_t connections[MAX_PHONE];
int have_conn[MAX_PHONE];

static hash_table_t pending_calls;

/*
 * Pseudo-protocols
 */
proto_t *proto_system;		/**< Protocol describing system IPC methods. */
proto_t	*proto_unknown;		/**< Protocol with no known methods. */


static size_t pending_call_key_hash(void *key)
{
	cap_call_handle_t *chandle = (cap_call_handle_t *) key;
	return CAP_HANDLE_RAW(*chandle);
}

static size_t pending_call_hash(const ht_link_t *item)
{
	pending_call_t *hs = hash_table_get_inst(item, pending_call_t, link);
	return CAP_HANDLE_RAW(hs->call_handle);
}

static bool pending_call_key_equal(void *key, const ht_link_t *item)
{
	cap_call_handle_t *chandle = (cap_call_handle_t *) key;
	pending_call_t *hs = hash_table_get_inst(item, pending_call_t, link);

	return *chandle == hs->call_handle;
}

static hash_table_ops_t pending_call_ops = {
	.hash = pending_call_hash,
	.key_hash = pending_call_key_hash,
	.key_equal = pending_call_key_equal,
	.equal = NULL,
	.remove_callback = NULL
};


void ipcp_connection_set(cap_phone_handle_t phone, int server, proto_t *proto)
{
	// XXX: there is no longer a limit on the number of phones as phones are
	// now handled using capabilities
	if (CAP_HANDLE_RAW(phone) < 0 || CAP_HANDLE_RAW(phone) >= MAX_PHONE)
		return;
	connections[CAP_HANDLE_RAW(phone)].server = server;
	connections[CAP_HANDLE_RAW(phone)].proto = proto;
	have_conn[CAP_HANDLE_RAW(phone)] = 1;
}

void ipcp_connection_clear(cap_phone_handle_t phone)
{
	have_conn[CAP_HANDLE_RAW(phone)] = 0;
	connections[CAP_HANDLE_RAW(phone)].server = 0;
	connections[CAP_HANDLE_RAW(phone)].proto = NULL;
}

static void ipc_m_print(proto_t *proto, sysarg_t method)
{
	oper_t *oper;

	/* Try system methods first */
	oper = proto_get_oper(proto_system, method);

	if (oper == NULL && proto != NULL) {
		/* Not a system method, try the user protocol. */
		oper = proto_get_oper(proto, method);
	}

	if (oper != NULL) {
		printf("%s (%" PRIun ")", oper->name, method);
		return;
	}

	printf("%" PRIun, method);
}

void ipcp_init(void)
{
	val_type_t arg_def[OPER_MAX_ARGS] = {
		V_INTEGER,
		V_INTEGER,
		V_INTEGER,
		V_INTEGER,
		V_INTEGER
	};

	/*
	 * Create a pseudo-protocol 'unknown' that has no known methods.
	 */
	proto_unknown = proto_new("unknown");

	/*
	 * Create a pseudo-protocol 'system' defining names of system IPC
	 * methods.
	 */
	proto_system = proto_new("system");

	for (size_t i = 0; i < ipc_methods_len; i++) {
		oper_t *oper = oper_new(ipc_methods[i].name, OPER_MAX_ARGS,
		    arg_def, V_INTEGER, OPER_MAX_ARGS, arg_def);
		proto_add_oper(proto_system, ipc_methods[i].number, oper);
	}

	bool ok = hash_table_create(&pending_calls, 0, 0, &pending_call_ops);
	assert(ok);
}

void ipcp_cleanup(void)
{
	proto_delete(proto_system);
	hash_table_destroy(&pending_calls);
}

void ipcp_call_out(cap_phone_handle_t phandle, ipc_call_t *call,
    cap_call_handle_t chandle)
{
	pending_call_t *pcall;
	proto_t *proto;
	oper_t *oper;
	sysarg_t *args;
	int i;

	if (have_conn[CAP_HANDLE_RAW(phandle)])
		proto = connections[CAP_HANDLE_RAW(phandle)].proto;
	else
		proto = NULL;

	args = call->args;

	if ((display_mask & DM_IPC) != 0) {
		printf("Call handle: %p, phone: %p, proto: %s, method: ",
		    chandle, phandle, (proto ? proto->name : "n/a"));
		ipc_m_print(proto, IPC_GET_IMETHOD(*call));
		printf(" args: (%" PRIun ", %" PRIun ", %" PRIun ", "
		    "%" PRIun ", %" PRIun ")\n",
		    args[1], args[2], args[3], args[4], args[5]);
	}


	if ((display_mask & DM_USER) != 0) {

		if (proto != NULL) {
			oper = proto_get_oper(proto, IPC_GET_IMETHOD(*call));
		} else {
			oper = NULL;
		}

		if (oper != NULL) {

			printf("%s(%p).%s", (proto ? proto->name : "n/a"),
			    phandle, (oper ? oper->name : "unknown"));

			putchar('(');
			for (i = 1; i <= oper->argc; ++i) {
				if (i > 1)
					printf(", ");
				val_print(args[i], oper->arg_type[i - 1]);
			}
			putchar(')');

			if (oper->rv_type == V_VOID && oper->respc == 0) {
				/*
				 * No response data (typically the task will
				 * not be interested in the response).
				 * We will not display response.
				 */
				putchar('.');
			}

			putchar('\n');
		}
	} else {
		oper = NULL;
	}

	/* Store call in hash table for response matching */

	pcall = malloc(sizeof(pending_call_t));
	pcall->phone_handle = phandle;
	pcall->question = *call;
	pcall->call_handle = chandle;
	pcall->oper = oper;

	hash_table_insert(&pending_calls, &pcall->link);
}

static void parse_answer(cap_call_handle_t call_handle, pending_call_t *pcall,
    ipc_call_t *answer)
{
	cap_phone_handle_t phone;
	sysarg_t method;
	sysarg_t service;
	errno_t retval;
	proto_t *proto;
	cap_phone_handle_t cphone;

	sysarg_t *resp;
	oper_t *oper;
	int i;

	phone = pcall->phone_handle;
	method = IPC_GET_IMETHOD(pcall->question);
	retval = IPC_GET_RETVAL(*answer);

	resp = answer->args;

	if ((display_mask & DM_IPC) != 0) {
		printf("Response to %p: retval=%s, args = (%" PRIun ", "
		    "%" PRIun ", %" PRIun ", %" PRIun ", %" PRIun ")\n",
		    call_handle, str_error_name(retval), IPC_GET_ARG1(*answer),
		    IPC_GET_ARG2(*answer), IPC_GET_ARG3(*answer),
		    IPC_GET_ARG4(*answer), IPC_GET_ARG5(*answer));
	}

	if ((display_mask & DM_USER) != 0) {
		oper = pcall->oper;

		if ((oper != NULL) &&
		    ((oper->rv_type != V_VOID) || (oper->respc > 0))) {
			printf("->");

			if (oper->rv_type != V_VOID) {
				putchar(' ');
				val_print((sysarg_t) retval, oper->rv_type);
			}

			if (oper->respc > 0) {
				putchar(' ');
				putchar('(');
				for (i = 1; i <= oper->respc; ++i) {
					if (i > 1)
						printf(", ");
					val_print(resp[i], oper->resp_type[i - 1]);
				}
				putchar(')');
			}

			putchar('\n');
		}
	}

	if ((phone == PHONE_NS) && (method == IPC_M_CONNECT_ME_TO) &&
	    (retval == 0)) {
		/* Connected to a service (through NS) */
		service = IPC_GET_ARG2(pcall->question);
		proto = proto_get_by_srv(service);
		if (proto == NULL)
			proto = proto_unknown;

		cphone = (cap_phone_handle_t) IPC_GET_ARG5(*answer);
		if ((display_mask & DM_SYSTEM) != 0) {
			printf("Registering connection (phone %p, protocol: %s)\n", cphone,
			    proto->name);
		}

		ipcp_connection_set(cphone, 0, proto);
	}
}

void ipcp_call_in(ipc_call_t *call, cap_call_handle_t chandle)
{
	ht_link_t *item;
	pending_call_t *pcall;

	if ((call->flags & IPC_CALL_ANSWERED) == 0) {
		/* Not a response */
		if ((display_mask & DM_IPC) != 0) {
			printf("Not a response (handle %p)\n", chandle);
		}
		return;
	}

	item = hash_table_find(&pending_calls, &chandle);
	if (item == NULL)
		return; /* No matching question found */

	/*
	 * Response matched to question.
	 */

	pcall = hash_table_get_inst(item, pending_call_t, link);
	hash_table_remove(&pending_calls, &chandle);

	parse_answer(chandle, pcall, call);
	free(pcall);
}

void ipcp_hangup(cap_phone_handle_t phone, errno_t rc)
{
	if ((display_mask & DM_SYSTEM) != 0) {
		printf("Hang up phone %p -> %s\n", phone, str_error_name(rc));
		ipcp_connection_clear(phone);
	}
}

/** @}
 */
