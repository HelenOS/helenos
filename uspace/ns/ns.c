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
 * @file	ns.c
 * @brief	Naming service for HelenOS IPC.
 */


#include <ipc/ipc.h>
#include <ipc/ns.h>
#include <ipc/services.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <errno.h>
#include <assert.h>
#include <libadt/list.h>
#include <libadt/hash_table.h>
#include <sysinfo.h>
#include <ddi.h>
#include <as.h>

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

static void *clockaddr = NULL;
static void *klogaddr = NULL;

static void get_as_area(ipc_callid_t callid, ipc_call_t *call, char *name, void **addr)
{
	void *ph_addr;

	if (!*addr) {
		ph_addr = (void *) sysinfo_value(name);
		if (!ph_addr) {
			ipc_answer_fast(callid, ENOENT, 0, 0);
			return;
		}
		*addr = as_get_mappable_page(PAGE_SIZE);
		physmem_map(ph_addr, *addr, 1, AS_AREA_READ | AS_AREA_CACHEABLE);
	}
	ipc_answer_fast(callid, 0, (ipcarg_t) *addr, AS_AREA_READ);
}

int main(int argc, char **argv)
{
	ipc_call_t call;
	ipc_callid_t callid;
	
	ipcarg_t retval;

	if (!hash_table_create(&ns_hash_table, NS_HASH_TABLE_CHAINS, 3, &ns_hash_table_ops)) {
		return ENOMEM;
	}
		
	while (1) {
		callid = ipc_wait_for_call(&call);
		switch (IPC_GET_METHOD(call)) {
		case IPC_M_AS_AREA_RECV:
			switch (IPC_GET_ARG3(call)) {
			case SERVICE_MEM_REALTIME:
				get_as_area(callid, &call, "clock.faddr",
				    &clockaddr);
				break;
			case SERVICE_MEM_KLOG:
				get_as_area(callid, &call, "klog.faddr",
				    &klogaddr);
				break;
			default:
				ipc_answer_fast(callid, ENOENT, 0, 0);
			}
			continue;
		case IPC_M_PHONE_HUNGUP:
			retval = 0;
			break;
		case IPC_M_CONNECT_TO_ME:
			/*
			 * Server requests service registration.
			 */
			retval = register_service(IPC_GET_ARG1(call), IPC_GET_ARG3(call), &call);
			break;
		case IPC_M_CONNECT_ME_TO:
			/*
			 * Client requests to be connected to a service.
			 */
			retval = connect_to_service(IPC_GET_ARG1(call), &call, callid);
			break;
		default:
			retval = ENOENT;
			break;
		}
		if (! (callid & IPC_CALLID_NOTIFICATION)) {
			ipc_answer_fast(callid, retval, 0, 0);
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
			
	if (hash_table_find(&ns_hash_table, keys)) {
		return EEXISTS;
	}
			
	hs = (hashed_service_t *) malloc(sizeof(hashed_service_t));
	if (!hs) {
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
		return ENOENT;
	}
	hs = hash_table_get_instance(hlp, hashed_service_t, link);
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

/** 
 * @}
 */
