/*
 * Copyright (c) 2015 Jiri Svoboda
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

/** @addtogroup libnettl
 * @{
 */

/**
 * @file Association map
 *
 * Manages allocations of endpoints / endpoint pairs (corresponding to
 * UDP associations, TCP listeners and TCP connections)
 */

#include <errno.h>
#include <inet/inet.h>
#include <io/log.h>
#include <nettl/amap.h>
#include <stdint.h>
#include <stdlib.h>

int amap_create(amap_t **rmap)
{
	amap_t *map;
	log_msg(LOG_DEFAULT, LVL_DEBUG, "amap_create()");

	map = calloc(1, sizeof(amap_t));
	if (map == NULL)
		return ENOMEM;

	return EOK;
}

void amap_destroy(amap_t *map)
{
	log_msg(LOG_DEFAULT, LVL_DEBUG, "amap_destroy()");
	free(map);
}

/** Insert endpoint pair into map.
 *
 * If local endpoint is not fully specified, it is filled in (determine
 * source address, allocate port number). Checks for conflicting endpoint pair.
 *
 * @param map Association map
 * @param epp Endpoint pair, possibly with local port inet_port_any
 * @param arg arg User value
 * @param flags Flags
 * @param aepp Place to store actual endpoint pair, possibly with allocated port
 *
 * @return EOK on success, EEXISTS if conflicting epp exists,
 *         ENOMEM if out of memory
 */
int amap_insert(amap_t *map, inet_ep2_t *epp, void *arg, amap_flags_t flags,
    inet_ep2_t *aepp)
{
	int rc;

	log_msg(LOG_DEFAULT, LVL_DEBUG, "amap_insert()");

	*aepp = *epp;

	/* Fill in local address? */
	if (!inet_addr_is_any(&epp->remote.addr) &&
	    inet_addr_is_any(&epp->local.addr)) {
		log_msg(LOG_DEFAULT, LVL_DEBUG, "amap_insert: "
		    "determine local address");
		rc = inet_get_srcaddr(&epp->remote.addr, 0, &aepp->local.addr);
		if (rc != EOK) {
			log_msg(LOG_DEFAULT, LVL_DEBUG, "amap_insert: "
			    "cannot determine local address");
			return rc;
		}
	} else {
		log_msg(LOG_DEFAULT, LVL_DEBUG, "amap_insert: "
		    "local address specified or remote address not specified");
	}

	/** Allocate local port? */
	if (aepp->local.port == inet_port_any) {
		aepp->local.port = inet_port_dyn_lo; /* XXX */
		log_msg(LOG_DEFAULT, LVL_DEBUG, "amap_insert: allocated local "
		    "port %" PRIu16, aepp->local.port);
	} else {
		log_msg(LOG_DEFAULT, LVL_DEBUG, "amap_insert: local "
		    "port %" PRIu16 " specified", aepp->local.port);
	}

	return EOK;
}

void amap_remove(amap_t *map, inet_ep2_t *epp)
{
	log_msg(LOG_DEFAULT, LVL_DEBUG, "amap_remove()");
}

int amap_find(amap_t *map, inet_ep2_t *epp, void **rarg)
{
	log_msg(LOG_DEFAULT, LVL_DEBUG, "amap_find()");
	return EOK;
}

/**
 * @}
 */
