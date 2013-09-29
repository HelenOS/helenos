/*
 * Copyright (c) 2012 Jiri Svoboda
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

/** @addtogroup inet
 * @{
 */
/**
 * @file
 * @brief
 */

#include <bitops.h>
#include <errno.h>
#include <fibril_synch.h>
#include <io/log.h>
#include <ipc/loc.h>
#include <stdlib.h>
#include <str.h>
#include "sroute.h"
#include "inetsrv.h"
#include "inet_link.h"

static FIBRIL_MUTEX_INITIALIZE(sroute_list_lock);
static LIST_INITIALIZE(sroute_list);
static sysarg_t sroute_id = 0;

inet_sroute_t *inet_sroute_new(void)
{
	inet_sroute_t *sroute = calloc(1, sizeof(inet_sroute_t));

	if (sroute == NULL) {
		log_msg(LOG_DEFAULT, LVL_ERROR, "Failed allocating static route object. "
		    "Out of memory.");
		return NULL;
	}

	link_initialize(&sroute->sroute_list);
	fibril_mutex_lock(&sroute_list_lock);
	sroute->id = ++sroute_id;
	fibril_mutex_unlock(&sroute_list_lock);

	return sroute;
}

void inet_sroute_delete(inet_sroute_t *sroute)
{
	if (sroute->name != NULL)
		free(sroute->name);
	free(sroute);
}

void inet_sroute_add(inet_sroute_t *sroute)
{
	fibril_mutex_lock(&sroute_list_lock);
	list_append(&sroute->sroute_list, &sroute_list);
	fibril_mutex_unlock(&sroute_list_lock);
}

void inet_sroute_remove(inet_sroute_t *sroute)
{
	fibril_mutex_lock(&sroute_list_lock);
	list_remove(&sroute->sroute_list);
	fibril_mutex_unlock(&sroute_list_lock);
}

/** Find static route object matching address @a addr.
 *
 * @param addr	Address
 */
inet_sroute_t *inet_sroute_find(inet_addr_t *addr)
{
	ip_ver_t addr_ver = inet_addr_get(addr, NULL, NULL);
	
	inet_sroute_t *best = NULL;
	uint8_t best_bits = 0;
	
	fibril_mutex_lock(&sroute_list_lock);
	
	list_foreach(sroute_list, sroute_list, inet_sroute_t, sroute) {
		uint8_t dest_bits;
		ip_ver_t dest_ver = inet_naddr_get(&sroute->dest, NULL, NULL,
		    &dest_bits);
		
		/* Skip comparison with different address family */
		if (addr_ver != dest_ver)
			continue;
		
		/* Look for the most specific route */
		if ((best != NULL) && (best_bits >= dest_bits))
			continue;
		
		if (inet_naddr_compare_mask(&sroute->dest, addr)) {
			log_msg(LOG_DEFAULT, LVL_DEBUG, "inet_sroute_find: found candidate %p",
			    sroute);
			
			best = sroute;
			best_bits = dest_bits;
		}
	}
	
	if (best == NULL)
		log_msg(LOG_DEFAULT, LVL_DEBUG, "inet_sroute_find: Not found");
	
	fibril_mutex_unlock(&sroute_list_lock);
	
	return best;
}

/** Find static route with a specific name.
 *
 * @param name	Address object name
 * @return	Address object
 */
inet_sroute_t *inet_sroute_find_by_name(const char *name)
{
	log_msg(LOG_DEFAULT, LVL_DEBUG, "inet_sroute_find_by_name('%s')",
	    name);

	fibril_mutex_lock(&sroute_list_lock);

	list_foreach(sroute_list, sroute_list, inet_sroute_t, sroute) {
		if (str_cmp(sroute->name, name) == 0) {
			fibril_mutex_unlock(&sroute_list_lock);
			log_msg(LOG_DEFAULT, LVL_DEBUG, "inet_sroute_find_by_name: found %p",
			    sroute);
			return sroute;
		}
	}

	log_msg(LOG_DEFAULT, LVL_DEBUG, "inet_sroute_find_by_name: Not found");
	fibril_mutex_unlock(&sroute_list_lock);

	return NULL;
}

/** Find static route with the given ID.
 *
 * @param id	Address object ID
 * @return	Address object
 */
inet_sroute_t *inet_sroute_get_by_id(sysarg_t id)
{
	log_msg(LOG_DEFAULT, LVL_DEBUG, "inet_sroute_get_by_id(%zu)", (size_t)id);

	fibril_mutex_lock(&sroute_list_lock);

	list_foreach(sroute_list, sroute_list, inet_sroute_t, sroute) {
		if (sroute->id == id) {
			fibril_mutex_unlock(&sroute_list_lock);
			return sroute;
		}
	}

	fibril_mutex_unlock(&sroute_list_lock);

	return NULL;
}

/** Get IDs of all static routes. */
int inet_sroute_get_id_list(sysarg_t **rid_list, size_t *rcount)
{
	sysarg_t *id_list;
	size_t count, i;

	fibril_mutex_lock(&sroute_list_lock);
	count = list_count(&sroute_list);

	id_list = calloc(count, sizeof(sysarg_t));
	if (id_list == NULL) {
		fibril_mutex_unlock(&sroute_list_lock);
		return ENOMEM;
	}

	i = 0;
	list_foreach(sroute_list, sroute_list, inet_sroute_t, sroute) {
		id_list[i++] = sroute->id;
	}

	fibril_mutex_unlock(&sroute_list_lock);

	*rid_list = id_list;
	*rcount = count;

	return EOK;
}

/** @}
 */
