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

#include <adt/hash_table.h>
#include <adt/list.h>
#include <as.h>
#include <errno.h>
#include <malloc.h>

#include "usb/usbmem.h"

#define ADDR_BUCKETS 1537

//address translation tables
static hash_table_t * pa2va_table = NULL;
static hash_table_t * va2pa_table = NULL;

/**
 * address translation hashtable item
 */
typedef struct{
	unsigned long addr;
	unsigned long translation;
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
	//delete item
	addr_node_t *addr_node = hash_table_get_instance(item, addr_node_t, link);
	free(addr_node);
}


static hash_table_operations_t addr_devices_ops = {
	.hash = addr_hash,
	.compare = addr_compare,
	.remove_callback = addr_remove_callback
};

/**
 * create node for address translation hashtable
 * @param addr
 * @param translation
 * @return
 */
static addr_node_t * create_addr_node(void * addr, void * translation){
	addr_node_t * node = (addr_node_t*)malloc(sizeof(addr_node_t));
	node->addr = (unsigned long)addr;
	node->translation = (unsigned long)translation;
	return node;
}

/**
 * allocate size on heap and register it`s pa<->va translation
 *
 * If physical address + size is 2GB or higher, nothing is allocated and NULL
 * is returned.
 * @param size
 * @param alignment
 * @return
 */
void * mman_malloc(
	size_t size,
	size_t alignment,
	unsigned long max_physical_address)
{
	if (size == 0)
		return NULL;
	if (alignment == 0)
		return NULL;
	//check if tables were initialized
	if(!pa2va_table){
		pa2va_table = (hash_table_t*)malloc(sizeof(hash_table_t*));
		va2pa_table = (hash_table_t*)malloc(sizeof(hash_table_t*));
		hash_table_create(pa2va_table, ADDR_BUCKETS, 1,
		    &addr_devices_ops);
		hash_table_create(va2pa_table, ADDR_BUCKETS, 1,
		    &addr_devices_ops);
	}
	//allocate
	void * vaddr = memalign(alignment, size);
	
	//get translation
	void * paddr = NULL;
	int opResult = as_get_physical_mapping(vaddr,(uintptr_t*)&paddr);
	if(opResult != EOK){
		//something went wrong
		free(vaddr);
		return NULL;
	}
	if((unsigned long)paddr + size > max_physical_address){
		//unusable address for usb
		free(vaddr);
		return NULL;
	}
	//store translation
	addr_node_t * pa2vaNode = create_addr_node(paddr,vaddr);
	addr_node_t * va2paNode = create_addr_node(vaddr,paddr);

	unsigned long keypaddr = (unsigned long)paddr;
	unsigned long keyvaddr = (unsigned long)vaddr;
	hash_table_insert(pa2va_table, (&keypaddr), &pa2vaNode->link);
	hash_table_insert(va2pa_table, (&keyvaddr), &va2paNode->link);
	//return
	return vaddr;

}

/**
 * get virtual address from physical
 * @param addr
 * @return translated virtual address or null
 */
void * mman_getVA(void * addr){
	unsigned long keypaddr = (unsigned long)addr;
	link_t * link = hash_table_find(pa2va_table, &keypaddr);
	if(!link) return NULL;
	addr_node_t * node = hash_table_get_instance(link, addr_node_t, link);
	return (void*)node->translation;
}

/**
 * get physical address from virtual
 * @param addr
 * @return physical address or null
 */
void * mman_getPA(void * addr){
	unsigned long keyvaddr = (unsigned long)addr;
	link_t * link = hash_table_find(va2pa_table, &keyvaddr);
	if(!link) return NULL;
	addr_node_t * node = hash_table_get_instance(link, addr_node_t, link);
	return (void*)node->translation;
}

/**
 * free the address and deregister it from pa<->va translation
 * @param vaddr if NULL, nothing happens
 */
void mman_free(void * vaddr){
	if(!vaddr)
		return;
	//get paddress
	void * paddr = mman_getPA(vaddr);
	unsigned long keypaddr = (unsigned long)paddr;
	unsigned long keyvaddr = (unsigned long)vaddr;
	//remove mapping
	hash_table_remove(pa2va_table,&keypaddr, 1);
	hash_table_remove(va2pa_table,&keyvaddr, 1);
	//free address
	free(vaddr);
}



/** @}
 */
