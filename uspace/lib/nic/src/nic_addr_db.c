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
#include <assert.h>
#include <stdlib.h>
#include <bool.h>
#include <errno.h>
#include <mem.h>

#include "nic_addr_db.h"

/**
 * Hash set helper function
 */
static int nic_addr_equals(const link_t *item1, const link_t *item2)
{
	assert(item1 && item2);
	size_t addr_len = ((const nic_addr_entry_t *) item1)->addr_len;

	assert(addr_len	== ((const nic_addr_entry_t *) item2)->addr_len);

	size_t i;
	for (i = 0; i < addr_len; ++i) {
		if (((nic_addr_entry_t *) item1)->addr[i] !=
			((nic_addr_entry_t *) item2)->addr[i])
			return false;
	}
	return true;
}

/**
 * Hash set helper function
 */
static unsigned long nic_addr_hash(const link_t *item)
{
	assert(item);
	const nic_addr_entry_t *entry = (const nic_addr_entry_t *) item;
	unsigned long hash = 0;

	size_t i;
	for (i = 0; i < entry->addr_len; ++i) {
		hash = (hash << 8) ^ (hash >> 24) ^ entry->addr[i];
	}
	return hash;
}

/**
 * Helper wrapper
 */
static void nic_addr_destroy(link_t *item, void *unused)
{
	free(item);
}

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
	if (!hash_set_init(&db->set, nic_addr_hash, nic_addr_equals,
		NIC_ADDR_DB_INIT_SIZE)) {
		return ENOMEM;
	}
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
	hash_set_clear(&db->set, nic_addr_destroy, NULL);
}

/**
 * Free the memory used by db, including all records...
 *
 * @param	db
 */
void nic_addr_db_destroy(nic_addr_db_t *db)
{
	assert(db);
	hash_set_apply(&db->set, nic_addr_destroy, NULL);
	hash_set_destroy(&db->set);
}

/**
 * Get number of addresses in the db
 *
 * @param	db
 *
 * @return Number of adresses
 */
size_t nic_addr_db_count(const nic_addr_db_t *db)
{
	assert(db);
	return hash_set_count(&db->set);
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
	nic_addr_entry_t *entry = malloc(sizeof (nic_addr_entry_t));
	if (entry == NULL) {
		return ENOMEM;
	}
	entry->addr_len = db->addr_len;
	memcpy(entry->addr, addr, db->addr_len);

	return hash_set_insert(&db->set, &entry->item) ? EOK : EEXIST;
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
	nic_addr_entry_t entry;
	entry.addr_len = db->addr_len;
	memcpy(entry.addr, addr, db->addr_len);

	link_t *removed = hash_set_remove(&db->set, &entry.item);
	free(removed);
	return removed != NULL ? EOK : ENOENT;
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
	nic_addr_entry_t entry;
	entry.addr_len = db->addr_len;
	memcpy(entry.addr, addr, db->addr_len);

	return hash_set_contains(&db->set, &entry.item);
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
static void nic_addr_db_fe_helper(link_t *item, void *arg) {
	nic_addr_db_fe_arg_t *hs = (nic_addr_db_fe_arg_t *) arg;
	hs->func(((nic_addr_entry_t *) item)->addr, hs->arg);
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
	hash_set_apply((hash_set_t *) &db->set, nic_addr_db_fe_helper, &hs);
}

/**
 * Helper structure for nic_addr_db_remove_selected
 */
typedef struct {
	int (*func)(const uint8_t *, void *);
	void *arg;
} nic_addr_db_rs_arg_t;

/**
 * Helper function for nic_addr_db_foreach
 */
static int nic_addr_db_rs_helper(link_t *item, void *arg) {
	nic_addr_db_rs_arg_t *hs = (nic_addr_db_rs_arg_t *) arg;
	int retval = hs->func(((nic_addr_entry_t *) item)->addr, hs->arg);
	if (retval) {
		free(item);
	}
	return retval;
}

/**
 * Removes all addresses for which the function returns non-zero.
 *
 * @param	db
 * @param	func	User-defined function
 * @param	arg		Custom argument passed to the function
 */
void nic_addr_db_remove_selected(nic_addr_db_t *db,
	int (*func)(const uint8_t *, void *), void *arg)
{
	nic_addr_db_rs_arg_t hs = { .func = func, .arg = arg };
	hash_set_remove_selected(&db->set, nic_addr_db_rs_helper, &hs);
}

/** @}
 */
