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
 * @brief Wake-on-LAN support
 */

#ifndef __NIC_WOL_VIRTUES_H__
#define __NIC_WOL_VIRTUES_H__

#ifndef LIBNIC_INTERNAL
#error "This is internal libnic's header, please do not include it"
#endif

#include <nic/nic.h>
#include <adt/hash_table.h>
#include "nic.h"

typedef struct nic_wol_virtues {
	/**
	 * Operations for table
	 */
	hash_table_ops_t table_operations;
	/**
	 * WOL virtues hashed by their ID's.
	 */
	hash_table_t table;
	/**
	 * WOL virtues in lists by their type
	 */
	nic_wol_virtue_t *lists[NIC_WV_MAX];
	/**
	 * Number of virtues in the wv_types list
	 */
	size_t lists_sizes[NIC_WV_MAX];
	/**
	 * Counter for the ID's
	 */
	nic_wv_id_t next_id;
	/**
	 * Maximum capabilities
	 */
	int caps_max[NIC_WV_MAX];
} nic_wol_virtues_t;

extern errno_t nic_wol_virtues_init(nic_wol_virtues_t *);
extern void nic_wol_virtues_clear(nic_wol_virtues_t *);
extern errno_t nic_wol_virtues_verify(nic_wv_type_t, const void *, size_t);
extern errno_t nic_wol_virtues_list(const nic_wol_virtues_t *, nic_wv_type_t type,
    size_t max_count, nic_wv_id_t *id_list, size_t *id_count);
extern errno_t nic_wol_virtues_add(nic_wol_virtues_t *, nic_wol_virtue_t *);
extern nic_wol_virtue_t *nic_wol_virtues_remove(nic_wol_virtues_t *,
    nic_wv_id_t);
extern const nic_wol_virtue_t *nic_wol_virtues_find(const nic_wol_virtues_t *,
    nic_wv_id_t);

#endif

/** @}
 */
