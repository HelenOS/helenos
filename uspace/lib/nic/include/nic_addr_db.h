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
