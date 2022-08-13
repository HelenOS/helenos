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
