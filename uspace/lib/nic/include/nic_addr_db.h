/*
 * SPDX-FileCopyrightText: 2011 Radim Vansa
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/**
 * @addtogroup libnic
 * @{
 */
/**
 * @file
 * @brief Generic hash-set based database of addresses
 */

#ifndef __NIC_ADDR_DB_H__
#define __NIC_ADDR_DB_H__

#ifndef LIBNIC_INTERNAL
#error "This is internal libnic header, please don't include it"
#endif

#include <adt/hash_table.h>
#include <types/common.h>

/**
 * Fibril-safe database of addresses implemented using hash set.
 */
typedef struct nic_addr_db {
	hash_table_t set;
	size_t addr_len;
} nic_addr_db_t;

extern errno_t nic_addr_db_init(nic_addr_db_t *db, size_t addr_len);
extern void nic_addr_db_clear(nic_addr_db_t *db);
extern void nic_addr_db_destroy(nic_addr_db_t *db);
extern errno_t nic_addr_db_insert(nic_addr_db_t *db, const uint8_t *addr);
extern errno_t nic_addr_db_remove(nic_addr_db_t *db, const uint8_t *addr);
extern bool nic_addr_db_contains(const nic_addr_db_t *db, const uint8_t *addr);
extern void nic_addr_db_foreach(const nic_addr_db_t *db,
    void (*func)(const uint8_t *, void *), void *arg);

#endif

/** @}
 */
