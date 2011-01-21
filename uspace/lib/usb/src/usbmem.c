/*
 * Copyright (c) 2010 Matus Dekanek
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

/** @addtogroup libc
 * @{
 */
/** @file implementation of special memory management, used mostly in usb stack
 */

#include "usb/usbmem.h"
#include <adt/hash_table.h>
#include <adt/list.h>

#define ADDR_BUCKETS 1537

hash_table_t * pa2fa_table = NULL;
hash_table_t * fa2pa_table = NULL;

typedef struct{
	long addr;
	link_t link;

}addr_node_t;

static hash_index_t addr_hash(unsigned long key[])
{
	return (3*key[0]<<4) % ADDR_BUCKETS;
}

static int addr_compare(unsigned long key[], hash_count_t keys,
    link_t *item)
{
	addr_node_t *addr_node = hash_table_get_instance(item, addr_node_t, link);
	return (addr_node->addr == key[0]);
}

static void addr_remove_callback(link_t *item)
{
}


static hash_table_operations_t addr_devices_ops = {
	.hash = addr_hash,
	.compare = addr_compare,
	.remove_callback = addr_remove_callback
};


//


void * mman_malloc(size_t size){
	//check if tables were initialized
	if(!pa2fa_table){
		pa2fa_table = (hash_table_t*)malloc(sizeof(hash_table_t*));
		fa2pa_table = (hash_table_t*)malloc(sizeof(hash_table_t*));
		hash_table_create(pa2fa_table, ADDR_BUCKETS, 1,
		    &addr_devices_ops);
		hash_table_create(fa2pa_table, ADDR_BUCKETS, 1,
		    &addr_devices_ops);
	}
	//allocate

	//get translation

	//store translation


	return NULL;

}

void * mman_getVA(void * addr){
	return NULL;
}

void * mman_getPA(void * addr){
	return NULL;
}

void mman_free(void * addr){
	
}



/** @}
 */
