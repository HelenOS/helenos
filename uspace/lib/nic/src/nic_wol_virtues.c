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

#include "nic_wol_virtues.h"
#include "nic.h"
#include <assert.h>
#include <errno.h>


/*
 * Hash table helper functions
 */

static size_t nic_wv_key_hash(void *key)
{
	return *(nic_wv_id_t*) key;
}

static size_t nic_wv_hash(const ht_link_t *item)
{
	nic_wol_virtue_t *virtue = (nic_wol_virtue_t *) item;
	return virtue->id;
}

static bool nic_wv_key_equal(void *key, const ht_link_t *item)
{
	nic_wol_virtue_t *virtue = (nic_wol_virtue_t *) item;
	return (virtue->id == *(nic_wv_id_t*) key);
}

/**
 * Initializes the WOL virtues structure
 *
 * @param wvs
 *
 * @return EOK		On success
 * @return ENOMEM	On not enough memory
 */
errno_t nic_wol_virtues_init(nic_wol_virtues_t *wvs)
{
	memset(wvs, 0, sizeof(nic_wol_virtues_t));
	wvs->table_operations.hash = nic_wv_hash;
	wvs->table_operations.key_hash = nic_wv_key_hash;
	wvs->table_operations.key_equal = nic_wv_key_equal;
	wvs->table_operations.equal = 0;
	wvs->table_operations.remove_callback = 0;
	
	if (!hash_table_create(&wvs->table, 0, 0, &wvs->table_operations)) {
		return ENOMEM;
	}
	size_t i;
	for (i = 0; i < NIC_WV_MAX; ++i) {
		wvs->caps_max[i] = -1;
	}
	wvs->next_id = 0;
	return EOK;
}

/**
 * Reinitializes the structure, destroying all virtues. The next_id is not
 * changed (some apps could still hold the filter IDs).
 *
 * @param wvs
 */
void nic_wol_virtues_clear(nic_wol_virtues_t *wvs)
{
	hash_table_clear(&wvs->table);
	nic_wv_type_t type;
	for (type = NIC_WV_NONE; type < NIC_WV_MAX; ++type) {
		nic_wol_virtue_t *virtue = wvs->lists[type];
		while (virtue != NULL) {
			nic_wol_virtue_t *next = virtue->next;
			free(virtue->data);
			free(virtue);
			virtue = next;
		}
		wvs->lists_sizes[type] = 0;
	}
}

/**
 * Verifies that the arguments for the WOL virtues are correct.
 *
 * @param type		Type of the virtue
 * @param data		Data argument for the virtue
 * @param length	Length of the data
 *
 * @return EOK		The arguments are correct
 * @return EINVAL	The arguments are incorrect
 * @return ENOTSUP	This type is unknown
 */
errno_t nic_wol_virtues_verify(nic_wv_type_t type, const void *data, size_t length)
{
	switch (type) {
	case NIC_WV_ARP_REQUEST:
	case NIC_WV_BROADCAST:
	case NIC_WV_LINK_CHANGE:
		return EOK;
	case NIC_WV_DESTINATION:
		return length == sizeof (nic_address_t) ? EOK : EINVAL;
	case NIC_WV_DIRECTED_IPV4:
		return length == sizeof (nic_wv_ipv4_data_t) ? EOK : EINVAL;
	case NIC_WV_DIRECTED_IPV6:
		return length == sizeof (nic_wv_ipv6_data_t) ? EOK : EINVAL;
	case NIC_WV_FULL_MATCH:
		return length % 2 == 0 ? EOK : EINVAL;
	case NIC_WV_MAGIC_PACKET:
		return data == NULL || length == sizeof (nic_wv_magic_packet_data_t) ?
			EOK : EINVAL;
	default:
		return ENOTSUP;
	}
}

/**
 * Adds the virtue to the list of known virtues, activating it.
 *
 * @param wvs
 * @param virtue	The virtue structure
 *
 * @return EOK		On success
 * @return ENOTSUP	If the virtue type is not supported
 * @return EINVAL	If the virtue type is a single-filter and there's already
 * 					a virtue of this type defined, or there is something wrong
 * 					with the data
 * @return ENOMEM	Not enough memory to activate the virtue
 */
errno_t nic_wol_virtues_add(nic_wol_virtues_t *wvs, nic_wol_virtue_t *virtue)
{
	if (!nic_wv_is_multi(virtue->type) &&
		wvs->lists[virtue->type] != NULL) {
		return EINVAL;
	}
	do {
		virtue->id = wvs->next_id++;
	} while (NULL != hash_table_find(&wvs->table, &virtue->id));
	hash_table_insert(&wvs->table, &virtue->item);
	virtue->next = wvs->lists[virtue->type];
	wvs->lists[virtue->type] = virtue;
	wvs->lists_sizes[virtue->type]++;
	return EOK;
}

/**
 * Removes the virtue from the list of virtues, but NOT deallocating the
 * nic_wol_virtue structure.
 *
 * @param wvs
 * @param id	Identifier of the removed virtue
 *
 * @return Removed virtue structure or NULL if not found.
 */
nic_wol_virtue_t *nic_wol_virtues_remove(nic_wol_virtues_t *wvs, nic_wv_id_t id)
{
	nic_wol_virtue_t *virtue = 
		(nic_wol_virtue_t *) hash_table_find(&wvs->table, &id);
	if (virtue == NULL) {
		return NULL;
	}

	/* Remove from filter_table */
	hash_table_remove_item(&wvs->table, &virtue->item);

	/* Remove from filter_types */
	assert(wvs->lists[virtue->type] != NULL);
	if (wvs->lists[virtue->type] == virtue) {
		wvs->lists[virtue->type] = virtue->next;
	} else {
		nic_wol_virtue_t *wv = wvs->lists[virtue->type];
		while (wv->next != virtue) {
			wv = wv->next;
			assert(wv != NULL);
		}
		wv->next = virtue->next;
	}
	wvs->lists_sizes[virtue->type]--;

	virtue->next = NULL;
	return virtue;
}


/**
 * Searches the filters table for a filter with specified ID
 *
 * @param wvs
 * @param id	Identifier of the searched virtue
 *
 * @return Requested filter or NULL if not found.
 */
const nic_wol_virtue_t *nic_wol_virtues_find(const nic_wol_virtues_t *wvs,
	nic_wv_id_t id)
{
	/*
	 * The hash_table_find cannot be const, because it would require the
	 * returned link to be const as well. But in this case, when we're returning
	 * constant virtue the retyping is correct.
	 */
	ht_link_t *virtue = hash_table_find(&((nic_wol_virtues_t *) wvs)->table, &id);
	return (const nic_wol_virtue_t *) virtue;
}

/**
 * Fill identifiers of current wol virtues of the specified type into the list.
 * If the type is set to NIC_WV_NONE, all virtues are used.
 *
 * @param		wvs
 * @param[in]	type		Type of the virtues or NIC_WV_NONE
 * @param[out]	id_list		The new vector of filter IDs. Can be NULL.
 * @param[out]	count		Number of IDs in the filter_list. Can be NULL.
 *
 * @return EOK		If it completes successfully
 * @return EINVAL	If the filter type is invalid
 */
errno_t nic_wol_virtues_list(const nic_wol_virtues_t *wvs, nic_wv_type_t type,
	size_t max_count, nic_wv_id_t *id_list, size_t *id_count)
{
	size_t count = 0;
	if (type == NIC_WV_NONE) {
		size_t i;
		for (i = NIC_WV_NONE; i < NIC_WV_MAX; ++i) {
			if (id_list != NULL) {
				nic_wol_virtue_t *virtue = wvs->lists[i];
				while (virtue != NULL) {
					if (count < max_count) {
						id_list[count] = virtue->id;
					}
					++count;
					virtue = virtue->next;
				}
			} else {
				count += wvs->lists_sizes[i];
			}
		}
	} else if (type >= NIC_WV_MAX) {
		return EINVAL;
	} else {
		if (id_list != NULL) {
			nic_wol_virtue_t *virtue = wvs->lists[type];
			while (virtue != NULL) {
				if (count < max_count) {
					id_list[count] = virtue->id;
				}
				++count;
				virtue = virtue->next;
			}
		} else {
			count = wvs->lists_sizes[type];
		}
	}
	if (id_count != NULL) {
		*id_count = count;
	}
	return EOK;
}

/** @}
 */
