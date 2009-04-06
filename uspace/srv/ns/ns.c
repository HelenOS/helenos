/*
 * Copyright (c) 2006 Ondrej Palkovsky
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

/** @addtogroup ns
 * @{
 */

/**
 * @file  ns.c
 * @brief Naming service for HelenOS IPC.
 */


#include <ipc/ipc.h>
#include <ipc/ns.h>
#include <ipc/services.h>
#include <stdio.h>
#include <bool.h>
#include <unistd.h>
#include <stdlib.h>
#include <errno.h>
#include <assert.h>
#include <libadt/list.h>
#include <libadt/hash_table.h>
#include <sysinfo.h>
#include <loader/loader.h>
#include <ddi.h>
#include <as.h>

#define NAME  "ns"

#define NS_HASH_TABLE_CHAINS  20

static int register_service(ipcarg_t service, ipcarg_t phone, ipc_call_t *call);
static void connect_to_service(ipcarg_t service, ipc_call_t *call,
    ipc_callid_t callid);

void register_clonable(ipcarg_t service, ipcarg_t phone, ipc_call_t *call,
    ipc_callid_t callid);
void connect_to_clonable(ipcarg_t service, ipc_call_t *call,
    ipc_callid_t callid);


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
	ipcarg_t service;        /**< Number of the service. */
	ipcarg_t phone;          /**< Phone registered with the service. */
	ipcarg_t in_phone_hash;  /**< Incoming phone hash. */
} hashed_service_t;

/** Pending connection structure. */
typedef struct {
	link_t link;
	ipcarg_t service;        /**< Number of the service. */
	ipc_callid_t callid;     /**< Call ID waiting for the connection */
	ipcarg_t arg2;           /**< Second argument */
	ipcarg_t arg3;           /**< Third argument */
} pending_req_t;

static link_t pending_req;

/** Request for connection to a clonable service. */
typedef struct {
	link_t link;
	ipcarg_t service;
	ipc_call_t call;
	ipc_callid_t callid;
} cs_req_t;

/** List of clonable-service connection requests. */
static link_t cs_req;

static void *clockaddr = NULL;
static void *klogaddr = NULL;

/** Return true if @a service is clonable. */
static bool service_clonable(int service)
{
	return (service == SERVICE_LOAD);
}

static void get_as_area(ipc_callid_t callid, ipc_call_t *call, void *ph_addr, count_t pages, void **addr)
{
	if (ph_addr == NULL) {
		ipc_answer_0(callid, ENOENT);
		return;
	}
	
	if (*addr == NULL) {
		*addr = as_get_mappable_page(pages * PAGE_SIZE);
		
		if (*addr == NULL) {
			ipc_answer_0(callid, ENOENT);
			return;
		}
		
		if (physmem_map(ph_addr, *addr, pages,
		    AS_AREA_READ | AS_AREA_CACHEABLE) != 0) {
			ipc_answer_0(callid, ENOENT);
			return;
		}
	}
	
	ipc_answer_2(callid, EOK, (ipcarg_t) *addr, AS_AREA_READ);
}

/** Process pending connection requests */
static void process_pending_req()
{
	link_t *cur;
	
loop:
	for (cur = pending_req.next; cur != &pending_req; cur = cur->next) {
		pending_req_t *pr = list_get_instance(cur, pending_req_t, link);
		
		unsigned long keys[3] = {
			pr->service,
			0,
			0
		};
		
		link_t *link = hash_table_find(&ns_hash_table, keys);
		if (!link)
			continue;
		
		hashed_service_t *hs = hash_table_get_instance(link, hashed_service_t, link);
		ipcarg_t retval = ipc_forward_fast(pr->callid, hs->phone,
		    pr->arg2, pr->arg3, 0, IPC_FF_NONE);
		
		if (!(pr->callid & IPC_CALLID_NOTIFICATION))
			ipc_answer_0(pr->callid, retval);
		
		list_remove(cur);
		free(pr);
		goto loop;
	}
}

int main(int argc, char **argv)
{
	printf(NAME ": HelenOS IPC Naming Service\n");
	
	if (!hash_table_create(&ns_hash_table, NS_HASH_TABLE_CHAINS, 3,
	    &ns_hash_table_ops)) {
		printf(NAME ": No memory available for services\n");
		return ENOMEM;
	}
	
	list_initialize(&pending_req);
	list_initialize(&cs_req);
	
	printf(NAME ": Accepting connections\n");
	while (true) {
		process_pending_req();
		
		ipc_call_t call;
		ipc_callid_t callid = ipc_wait_for_call(&call);
		ipcarg_t retval;
		
		switch (IPC_GET_METHOD(call)) {
		case IPC_M_SHARE_IN:
			switch (IPC_GET_ARG3(call)) {
			case SERVICE_MEM_REALTIME:
				get_as_area(callid, &call, sysinfo_value("clock.faddr"), 1, &clockaddr);
				break;
			case SERVICE_MEM_KLOG:
				get_as_area(callid, &call, sysinfo_value("klog.faddr"), sysinfo_value("klog.pages"), &klogaddr);
				break;
			default:
				ipc_answer_0(callid, ENOENT);
			}
			continue;
		case IPC_M_PHONE_HUNGUP:
			retval = EOK;
			break;
		case IPC_M_CONNECT_TO_ME:
			/*
			 * Server requests service registration.
			 */
			if (service_clonable(IPC_GET_ARG1(call))) {
				register_clonable(IPC_GET_ARG1(call),
				    IPC_GET_ARG5(call), &call, callid);
				continue;
			} else {
				retval = register_service(IPC_GET_ARG1(call),
				    IPC_GET_ARG5(call), &call);
			}
			break;
		case IPC_M_CONNECT_ME_TO:
			/*
			 * Client requests to be connected to a service.
			 */
			if (service_clonable(IPC_GET_ARG1(call))) {
				connect_to_clonable(IPC_GET_ARG1(call),
				    &call, callid);
				continue;
			} else {
				connect_to_service(IPC_GET_ARG1(call), &call,
				    callid);
				continue;
			}
			break;
		default:
			retval = ENOENT;
			break;
		}
		
		if (!(callid & IPC_CALLID_NOTIFICATION))
			ipc_answer_0(callid, retval);
	}
	
	/* Not reached */
	return 0;
}

/** Register service.
 *
 * @param service Service to be registered.
 * @param phone   Phone to be used for connections to the service.
 * @param call    Pointer to call structure.
 *
 * @return Zero on success or a value from @ref errno.h.
 *
 */
int register_service(ipcarg_t service, ipcarg_t phone, ipc_call_t *call)
{
	unsigned long keys[3] = {
		service,
		call->in_phone_hash,
		0
	};
	
	if (hash_table_find(&ns_hash_table, keys))
		return EEXISTS;
	
	hashed_service_t *hs = (hashed_service_t *) malloc(sizeof(hashed_service_t));
	if (!hs)
		return ENOMEM;
	
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
 * @param call    Pointer to call structure.
 * @param callid  Call ID of the request.
 *
 * @return Zero on success or a value from @ref errno.h.
 *
 */
void connect_to_service(ipcarg_t service, ipc_call_t *call, ipc_callid_t callid)
{
	ipcarg_t retval;
	unsigned long keys[3] = {
		service,
		0,
		0
	};
	
	link_t *link = hash_table_find(&ns_hash_table, keys);
	if (!link) {
		if (IPC_GET_ARG4(*call) & IPC_FLAG_BLOCKING) {
			/* Blocking connection, add to pending list */
			pending_req_t *pr = (pending_req_t *) malloc(sizeof(pending_req_t));
			if (!pr) {
				retval = ENOMEM;
				goto out;
			}
			
			pr->service = service;
			pr->callid = callid;
			pr->arg2 = IPC_GET_ARG2(*call);
			pr->arg3 = IPC_GET_ARG3(*call);
			list_append(&pr->link, &pending_req);
			return;
		}
		retval = ENOENT;
		goto out;
	}
	
	hashed_service_t *hs = hash_table_get_instance(link, hashed_service_t, link);
	retval = ipc_forward_fast(callid, hs->phone, IPC_GET_ARG2(*call),
	    IPC_GET_ARG3(*call), 0, IPC_FF_NONE);
out:
	if (!(callid & IPC_CALLID_NOTIFICATION))
		ipc_answer_0(callid, retval);
}

/** Register clonable service.
 *
 * @param service Service to be registered.
 * @param phone   Phone to be used for connections to the service.
 * @param call    Pointer to call structure.
 *
 */
void register_clonable(ipcarg_t service, ipcarg_t phone, ipc_call_t *call,
    ipc_callid_t callid)
{
	if (list_empty(&cs_req)) {
		/* There was no pending connection request. */
		printf(NAME ": Unexpected clonable server.\n");
		ipc_answer_0(callid, EBUSY);
		return;
	}
	
	cs_req_t *csr = list_get_instance(cs_req.next, cs_req_t, link);
	list_remove(&csr->link);
	
	/* Currently we can only handle a single type of clonable service. */
	assert(csr->service == SERVICE_LOAD);
	
	ipc_answer_0(callid, EOK);
	
	int rc = ipc_forward_fast(csr->callid, phone, IPC_GET_ARG2(csr->call),
		IPC_GET_ARG3(csr->call), 0, IPC_FF_NONE);

	free(csr);
	ipc_hangup(phone);
}

/** Connect client to clonable service.
 *
 * @param service Service to be connected to.
 * @param call    Pointer to call structure.
 * @param callid  Call ID of the request.
 *
 * @return Zero on success or a value from @ref errno.h.
 *
 */
void connect_to_clonable(ipcarg_t service, ipc_call_t *call,
    ipc_callid_t callid)
{
	assert(service == SERVICE_LOAD);
	
	cs_req_t *csr = malloc(sizeof(cs_req_t));
	if (csr == NULL) {
		ipc_answer_0(callid, ENOMEM);
		return;
	}
	
	/* Spawn a loader. */
	int rc = loader_spawn("loader");
	
	if (rc < 0) {
		free(csr);
		ipc_answer_0(callid, rc);
		return;
	}
	
	csr->service = service;
	csr->call = *call;
	csr->callid = callid;
	
	/*
	 * We can forward the call only after the server we spawned connects
	 * to us. Meanwhile we might need to service more connection requests.
	 * Thus we store the call in a queue.
	 */
	list_append(&csr->link, &cs_req);
}

/** Compute hash index into NS hash table.
 *
 * @param key Pointer keys. However, only the first key (i.e. service number)
 *            is used to compute the hash index.
 *
 * @return Hash index corresponding to key[0].
 *
 */
hash_index_t ns_hash(unsigned long *key)
{
	assert(key);
	return (*key % NS_HASH_TABLE_CHAINS);
}

/** Compare a key with hashed item.
 *
 * This compare function always ignores the third key.
 * It exists only to make it possible to remove records
 * originating from connection with key[1] in_phone_hash
 * value. Note that this is close to being classified
 * as a nasty hack.
 *
 * @param key  Array of keys.
 * @param keys Must be lesser or equal to 3.
 * @param item Pointer to a hash table item.
 *
 * @return Non-zero if the key matches the item, zero otherwise.
 *
 */
int ns_compare(unsigned long key[], hash_count_t keys, link_t *item)
{
	assert(key);
	assert(keys <= 3);
	assert(item);
	
	hashed_service_t *hs = hash_table_get_instance(item, hashed_service_t, link);
	
	if (keys == 2)
		return key[1] == hs->in_phone_hash;
	else
		return key[0] == hs->service;
}

/** Perform actions after removal of item from the hash table.
 *
 * @param item Item that was removed from the hash table.
 *
 */
void ns_remove(link_t *item)
{
	assert(item);
	free(hash_table_get_instance(item, hashed_service_t, link));
}

/**
 * @}
 */
