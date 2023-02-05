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
#include <stdbool.h>
#include <errno.h>
#include <mem.h>
#include <member.h>
#include <adt/hash_table.h>
#include <macros.h>
#include <stdint.h>

/**
 * Helper structure for keeping the address in the hash set.
 */
typedef struct nic_addr_entry {
	ht_link_t link;
	uint8_t len;
	uint8_t addr[1];
} nic_addr_entry_t;

/*
 * Hash table helper functions
 */
typedef struct {
	size_t len;
	const uint8_t *addr;
} addr_key_t;

static bool nic_addr_key_equal(const void *key_arg, const ht_link_t *item)
{
	const addr_key_t *key = key_arg;
	nic_addr_entry_t *entry = member_to_inst(item, nic_addr_entry_t, link);

	return memcmp(entry->addr, key->addr, entry->len) == 0;
}

static size_t addr_hash(size_t len, const uint8_t *addr)
{
	size_t hash = 0;

	for (size_t i = 0; i < len; ++i) {
		hash = (hash << 5) ^ addr[i];
	}

	return hash;
}

static size_t nic_addr_key_hash(const void *k)
{
	const addr_key_t *key = k;
	return addr_hash(key->len, key->addr);
}

static size_t nic_addr_hash(const ht_link_t *item)
{
	nic_addr_entry_t *entry = member_to_inst(item, nic_addr_entry_t, link);
	return addr_hash(entry->len, entry->addr);
}

static void nic_addr_removed(ht_link_t *item)
{
	nic_addr_entry_t *entry = member_to_inst(item, nic_addr_entry_t, link);

	free(entry);
}

static const hash_table_ops_t set_ops = {
	.hash = nic_addr_hash,
	.key_hash = nic_addr_key_hash,
	.key_equal = nic_addr_key_equal,
	.equal = NULL,
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
errno_t nic_addr_db_init(nic_addr_db_t *db, size_t addr_len)
{
	assert(db);

	if (addr_len > UCHAR_MAX)
		return EINVAL;

	if (!hash_table_create(&db->set, 0, 0, &set_ops))
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
 * @return EEXIST	If this address already is in the db
 */
errno_t nic_addr_db_insert(nic_addr_db_t *db, const uint8_t *addr)
{
	assert(db && addr);

	addr_key_t key = {
		.len = db->addr_len,
		.addr = addr
	};

	if (hash_table_find(&db->set, &key))
		return EEXIST;

	nic_addr_entry_t *entry = malloc(sizeof(nic_addr_entry_t) + db->addr_len - 1);
	if (entry == NULL)
		return ENOMEM;

	entry->len = (uint8_t) db->addr_len;
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
errno_t nic_addr_db_remove(nic_addr_db_t *db, const uint8_t *addr)
{
	assert(db && addr);

	addr_key_t key = {
		.len = db->addr_len,
		.addr = addr
	};

	if (hash_table_remove(&db->set, &key))
		return EOK;
	else
		return ENOENT;
}

/**
 * Test if the address is contained in the db
 *
 * @param	db
 * @param	addr	Tested addresss
 *
 * @return true if the address is in the db, false otherwise
 */
bool nic_addr_db_contains(const nic_addr_db_t *db, const uint8_t *addr)
{
	assert(db && addr);

	addr_key_t key = {
		.len = db->addr_len,
		.addr = addr
	};

	return 0 != hash_table_find(&db->set, &key);
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
static bool nic_addr_db_fe_helper(ht_link_t *item, void *arg)
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
	hash_table_apply((hash_table_t *)&db->set, nic_addr_db_fe_helper, &hs);
}

/** @}
 */
