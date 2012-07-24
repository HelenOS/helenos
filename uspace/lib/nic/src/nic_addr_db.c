/*
 * Copyright (c) 2011 Radim Vansa
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
 * @addtogroup libnic
 * @{
 */
/**
 * @file
 * @brief Generic hash-set based database of addresses
 */
#include "nic_addr_db.h"
#include <assert.h>
#include <stdlib.h>
#include <bool.h>
#include <errno.h>
#include <mem.h>
#include <adt/hash_table.h>
#include <macros.h>

/* The key count hash table field is not used. Use this dummy value. */
#define KEY_CNT 1

/**
 * Maximal length of addresses in the DB (in bytes).
 */
#define NIC_ADDR_MAX_LENGTH		16

/**
 * Helper structure for keeping the address in the hash set.
 */
typedef struct nic_addr_entry {
	link_t link;
	uint8_t addr[NIC_ADDR_MAX_LENGTH];
} nic_addr_entry_t;


/* 
 * Hash table helper functions 
 */

static bool nic_addr_match(unsigned long *key, size_t key_cnt, 
	const link_t *item)
{
	uint8_t *addr = (uint8_t*)key;
	nic_addr_entry_t *entry = member_to_inst(item, nic_addr_entry_t, link);

	return 0 == bcmp(entry->addr, addr, NIC_ADDR_MAX_LENGTH);
}

static size_t nic_addr_key_hash(unsigned long *key)
{
	uint8_t *addr = (uint8_t*)key;
	size_t hash = 0;

	for (int i = NIC_ADDR_MAX_LENGTH - 1; i >= 0; --i) {
		hash = (hash << 8) ^ (hash >> 24) ^ addr[i];
	}
	
	return hash;
}

static size_t nic_addr_hash(const link_t *item)
{
	nic_addr_entry_t *entry = member_to_inst(item, nic_addr_entry_t, link);
	
	unsigned long *key = (unsigned long*)entry->addr;
	return nic_addr_key_hash(key);
}

static void nic_addr_removed(link_t *item)
{
	nic_addr_entry_t *entry = member_to_inst(item, nic_addr_entry_t, link);
	
	free(entry);
}

static hash_table_ops_t set_ops = {
	.hash = nic_addr_hash,
	.key_hash = nic_addr_key_hash,
	.match = nic_addr_match,
	.equal = 0,
	.remove_callback = nic_addr_removed
};

/**
 * Initialize the database
 *
 * @param[in,out]	db			Uninitialized storage
 * @param[in]		addr_len	Size of addresses in the db
 *
 * @return EOK		If successfully initialized
 * @return EINVAL	If the address length is too big
 * @return ENOMEM	If there was not enough memory to initialize the storage
 */
int nic_addr_db_init(nic_addr_db_t *db, size_t addr_len)
{
	assert(db);
	if (addr_len > NIC_ADDR_MAX_LENGTH) {
		return EINVAL;
	}
	
	if (!hash_table_create(&db->set, 0, KEY_CNT, &set_ops))
		return ENOMEM;
	
	db->addr_len = addr_len;
	return EOK;
}

/**
 * Remove all records from the DB
 *
 * @param db
 */
void nic_addr_db_clear(nic_addr_db_t *db)
{
	assert(db);
	hash_table_clear(&db->set);
}

/**
 * Free the memory used by db, including all records...
 *
 * @param	db
 */
void nic_addr_db_destroy(nic_addr_db_t *db)
{
	assert(db);
	nic_addr_db_clear(db);
	hash_table_destroy(&db->set);
}


/**
 * Insert an address to the db
 *
 * @param	db
 * @param	addr 	Inserted address. Length is implicitly concluded from the
 * 					db's address length.
 *
 * @return EOK		If the address was inserted
 * @return ENOMEM	If there was not enough memory
 * @return EEXIST	If this adress already is in the db
 */
int nic_addr_db_insert(nic_addr_db_t *db, const uint8_t *addr)
{
	assert(db && addr);
	/* Ugly type-punning hack. */
	unsigned long *key = (unsigned long*)addr;
	
	if (hash_table_find(&db->set, key))
		return EEXIST;
	
	nic_addr_entry_t *entry = malloc(sizeof(nic_addr_entry_t));
	if (entry == NULL) 
		return ENOMEM;
	
	link_initialize(&entry->link);
	
	bzero(entry->addr, NIC_ADDR_MAX_LENGTH);
	memcpy(entry->addr, addr, db->addr_len);
	
	hash_table_insert(&db->set, &entry->link);
	return EOK;
}

/**
 * Remove the address from the db
 *
 * @param	db
 * @param	addr	Removed address.
 *
 * @return EOK		If the address was removed
 * @return ENOENT	If there is no address
 */
int nic_addr_db_remove(nic_addr_db_t *db, const uint8_t *addr)
{
	assert(db && addr);
	unsigned long *key = (unsigned long*)addr;
	
	link_t *item = hash_table_find(&db->set, key);
	
	if (item) {
		hash_table_remove_item(&db->set, item);
		return EOK;
	} else {
		return ENOENT;
	}
}

/**
 * Test if the address is contained in the db
 *
 * @param	db
 * @param	addr	Tested addresss
 *
 * @return true if the address is in the db, false otherwise
 */
int nic_addr_db_contains(const nic_addr_db_t *db, const uint8_t *addr)
{
	assert(db && addr);
	unsigned long *key = (unsigned long*)addr;
	
	return 0 != hash_table_find(&db->set, key);
}

/**
 * Helper structure for nic_addr_db_foreach
 */
typedef struct {
	void (*func)(const uint8_t *, void *);
	void *arg;
} nic_addr_db_fe_arg_t;

/**
 * Helper function for nic_addr_db_foreach
 */
static bool nic_addr_db_fe_helper(link_t *item, void *arg) 
{
	nic_addr_db_fe_arg_t *hs = (nic_addr_db_fe_arg_t *) arg;
	nic_addr_entry_t *entry = member_to_inst(item, nic_addr_entry_t, link);
	hs->func(entry->addr, hs->arg);
	return true;
}

/**
 * Executes a user-defined function on all addresses in the DB. The function
 * must not change the addresses.
 *
 * @param	db
 * @param	func	User-defined function
 * @param	arg		Custom argument passed to the function
 */
void nic_addr_db_foreach(const nic_addr_db_t *db,
	void (*func)(const uint8_t *, void *), void *arg)
{
	nic_addr_db_fe_arg_t hs = { .func = func, .arg = arg };
	hash_table_apply((hash_table_t*)&db->set, nic_addr_db_fe_helper, &hs);
}

/** @}
 */
