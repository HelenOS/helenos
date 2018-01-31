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
 *
 * An association map contains different types of entries, based on which
 * set of attributes (key) they specify. In order from most specific to the
 * least specific one:
 *
 *  - repla (remote endpoint, local address)
 *  - laddr (local address)
 *  - llink (local link)
 *  - unspec (unspecified)
 *
 * In the unspecified case only the local port is known and the entry matches
 * all remote and local addresses.
 */

#include <adt/list.h>
#include <errno.h>
#include <inet/addr.h>
#include <inet/inet.h>
#include <io/log.h>
#include <nettl/amap.h>
#include <stdint.h>
#include <stdlib.h>

/** Convert association map flags to port range flags.
 *
 * @param flags Association map flags
 * @return Port range flags
 */
static portrng_flags_t aflags_to_pflags(amap_flags_t flags)
{
	portrng_flags_t pflags;

	pflags = 0;
	if ((flags & af_allow_system) != 0)
		pflags |= pf_allow_system;

	return pflags;
}

/** Create association map.
 *
 * @param rmap Place to store pointer to new association map
 * @return EOk on success, ENOMEM if out of memory
 */
errno_t amap_create(amap_t **rmap)
{
	amap_t *map;
	errno_t rc;

	log_msg(LOG_DEFAULT, LVL_DEBUG2, "amap_create()");

	map = calloc(1, sizeof(amap_t));
	if (map == NULL)
		return ENOMEM;

	rc = portrng_create(&map->unspec);
	if (rc != EOK) {
		assert(rc == ENOMEM);
		free(map);
		return ENOMEM;
	}

	list_initialize(&map->repla);
	list_initialize(&map->laddr);
	list_initialize(&map->llink);

	*rmap = map;
	return EOK;
}

/** Destroy association map.
 *
 * @param map Association map
 */
void amap_destroy(amap_t *map)
{
	log_msg(LOG_DEFAULT, LVL_DEBUG2, "amap_destroy()");

	assert(list_empty(&map->repla));
	assert(list_empty(&map->laddr));
	assert(list_empty(&map->llink));
	free(map);
}

/** Find exact repla.
 *
 * Find repla (remote endpoint, local address) entry by exact match.
 *
 * @param map Association map
 * @param rep Remote endpoint
 * @param la  Local address
 * @param rrepla Place to store pointer to repla
 *
 * @return EOK on success, ENOENT if not found
 */
static errno_t amap_repla_find(amap_t *map, inet_ep_t *rep, inet_addr_t *la,
    amap_repla_t **rrepla)
{
	char *sraddr, *sladdr;

	(void) inet_addr_format(&rep->addr, &sraddr);
	(void) inet_addr_format(la, &sladdr);

	log_msg(LOG_DEFAULT, LVL_DEBUG2, "amap_repla_find(): rep=(%s,%" PRIu16
	    ") la=%s", sraddr, rep->port, sladdr);
	free(sraddr);
	free(sladdr);

	list_foreach(map->repla, lamap, amap_repla_t, repla) {
		(void) inet_addr_format(&repla->rep.addr, &sraddr);
		(void) inet_addr_format(&repla->laddr, &sladdr);
		log_msg(LOG_DEFAULT, LVL_DEBUG2, "amap_repla_find(): "
		    "compare to rep=(%s, %" PRIu16 ") la=%s",
		    sraddr, repla->rep.port, sladdr);
		free(sraddr);
		free(sladdr);
		if (inet_addr_compare(&repla->rep.addr, &rep->addr) &&
		    repla->rep.port == rep->port &&
		    inet_addr_compare(&repla->laddr, la)) {
			*rrepla = repla;
			return EOK;
		}
	}

	*rrepla = NULL;
	return ENOENT;
}

/** Insert repla.
 *
 * Insert new repla (remote endpoint, local address) entry to association map.
 *
 * @param amap   Association map
 * @param rep    Remote endpoint
 * @param la     Local address
 * @param rrepla Place to store pointer to new repla
 *
 * @return EOK on success, ENOMEM if out of memory
 */
static errno_t amap_repla_insert(amap_t *map, inet_ep_t *rep, inet_addr_t *la,
    amap_repla_t **rrepla)
{
	amap_repla_t *repla;
	errno_t rc;

	repla = calloc(1, sizeof(amap_repla_t));
	if (repla == NULL) {
		*rrepla = NULL;
		return ENOMEM;
	}

	rc = portrng_create(&repla->portrng);
	if (rc != EOK) {
		free(repla);
		return ENOMEM;
	}

	repla->rep = *rep;
	repla->laddr = *la;
	list_append(&repla->lamap, &map->repla);

	*rrepla = repla;
	return EOK;
}

/** Remove repla from association map.
 *
 * Remove repla (remote endpoint, local address) from association map.
 *
 * @param map   Association map
 * @param repla Repla
 */
static void amap_repla_remove(amap_t *map, amap_repla_t *repla)
{
	list_remove(&repla->lamap);
	portrng_destroy(repla->portrng);
	free(repla);
}

/** Find exact laddr.
 *
 * Find laddr (local address) entry by exact match.
 *
 * @param map    Association map
 * @param addr   Address
 * @param rladdr Place to store pointer to laddr entry
 *
 * @return EOK on success, ENOENT if not found.
 */
static errno_t amap_laddr_find(amap_t *map, inet_addr_t *addr,
    amap_laddr_t **rladdr)
{
	list_foreach(map->laddr, lamap, amap_laddr_t, laddr) {
		if (inet_addr_compare(&laddr->laddr, addr)) {
			*rladdr = laddr;
			return EOK;
		}
	}

	*rladdr = NULL;
	return ENOENT;
}

/** Insert laddr.
 *
 * Insert new laddr (local address) entry to association map.
 *
 * @param addr   Local address
 * @param rladdr Place to store pointer to new laddr
 *
 * @return EOK on success, ENOMEM if out of memory
 */
static errno_t amap_laddr_insert(amap_t *map, inet_addr_t *addr,
    amap_laddr_t **rladdr)
{
	amap_laddr_t *laddr;
	errno_t rc;

	laddr = calloc(1, sizeof(amap_laddr_t));
	if (laddr == NULL) {
		*rladdr = NULL;
		return ENOMEM;
	}

	rc = portrng_create(&laddr->portrng);
	if (rc != EOK) {
		free(laddr);
		return ENOMEM;
	}

	laddr->laddr = *addr;
	list_append(&laddr->lamap, &map->laddr);

	*rladdr = laddr;
	return EOK;
}

/** Remove laddr from association map.
 *
 * Remove laddr (local address) entry from association map.
 *
 * @param map   Association map
 * @param laddr Laddr entry
 */
static void amap_laddr_remove(amap_t *map, amap_laddr_t *laddr)
{
	list_remove(&laddr->lamap);
	portrng_destroy(laddr->portrng);
	free(laddr);
}

/** Find exact llink.
 *
 * Find llink (local link) entry by exact match.
 *
 * @param map     Association map
 * @param link_id Local link ID
 * @param rllink  Place to store pointer to llink entry
 *
 * @return EOK on success, ENOENT if not found.
 */
static errno_t amap_llink_find(amap_t *map, sysarg_t link_id,
    amap_llink_t **rllink)
{
	list_foreach(map->llink, lamap, amap_llink_t, llink) {
		if (llink->llink == link_id) {
			*rllink = llink;
			return EOK;
		}
	}

	*rllink = NULL;
	return ENOENT;
}

/** Insert llink.
 *
 * Insert new llink (local link) entry to association map.
 *
 * @param link_id Local link
 * @param rllink  Place to store pointer to new llink
 *
 * @return EOK on success, ENOMEM if out of memory
 */
static errno_t amap_llink_insert(amap_t *map, sysarg_t link_id,
    amap_llink_t **rllink)
{
	amap_llink_t *llink;
	errno_t rc;

	llink = calloc(1, sizeof(amap_llink_t));
	if (llink == NULL) {
		*rllink = NULL;
		return ENOMEM;
	}

	rc = portrng_create(&llink->portrng);
	if (rc != EOK) {
		free(llink);
		return ENOMEM;
	}

	llink->llink = link_id;
	list_append(&llink->lamap, &map->llink);

	*rllink = llink;
	return EOK;
}

/** Remove llink from association map.
 *
 * Remove llink (local link) entry from association map.
 *
 * @param map   Association map
 * @param llink Llink entry
 */
static void amap_llink_remove(amap_t *map, amap_llink_t *llink)
{
	list_remove(&llink->lamap);
	portrng_destroy(llink->portrng);
	free(llink);
}

/** Insert endpoint pair into map with repla as key.
 *
 * If local port number is not specified, it is allocated.
 *
 * @param map Association map
 * @param epp Endpoint pair, possibly with local port inet_port_any
 * @param arg arg User value
 * @param flags Flags
 * @param aepp Place to store actual endpoint pair, possibly with allocated port
 *
 * @return EOK on success, EEXIST if conflicting epp exists,
 *         ENOMEM if out of memory
 */
static errno_t amap_insert_repla(amap_t *map, inet_ep2_t *epp, void *arg,
    amap_flags_t flags, inet_ep2_t *aepp)
{
	amap_repla_t *repla;
	inet_ep2_t mepp;
	errno_t rc;

	log_msg(LOG_DEFAULT, LVL_DEBUG2, "amap_insert_repla()");

	rc = amap_repla_find(map, &epp->remote, &epp->local.addr, &repla);
	if (rc != EOK) {
		/* New repla */
		rc = amap_repla_insert(map, &epp->remote, &epp->local.addr,
		    &repla);
		if (rc != EOK) {
			assert(rc == ENOMEM);
			return rc;
		}
	}

	mepp = *epp;

	rc = portrng_alloc(repla->portrng, epp->local.port, arg, aflags_to_pflags(flags),
	    &mepp.local.port);
	if (rc != EOK) {
		return rc;
	}

	*aepp = mepp;
	return EOK;
}

/** Insert endpoint pair into map with laddr as key.
 *
 * If local port number is not specified, it is allocated.
 *
 * @param map Association map
 * @param epp Endpoint pair, possibly with local port inet_port_any
 * @param arg arg User value
 * @param flags Flags
 * @param aepp Place to store actual endpoint pair, possibly with allocated port
 *
 * @return EOK on success, EEXIST if conflicting epp exists,
 *         ENOMEM if out of memory
 */
static errno_t amap_insert_laddr(amap_t *map, inet_ep2_t *epp, void *arg,
    amap_flags_t flags, inet_ep2_t *aepp)
{
	amap_laddr_t *laddr;
	inet_ep2_t mepp;
	errno_t rc;

	log_msg(LOG_DEFAULT, LVL_DEBUG2, "amap_insert_laddr()");

	rc = amap_laddr_find(map, &epp->local.addr, &laddr);
	if (rc != EOK) {
		/* New laddr */
		rc = amap_laddr_insert(map, &epp->local.addr, &laddr);
		if (rc != EOK) {
			assert(rc == ENOMEM);
			return rc;
		}
	}

	mepp = *epp;

	rc = portrng_alloc(laddr->portrng, epp->local.port, arg, aflags_to_pflags(flags),
	    &mepp.local.port);
	if (rc != EOK) {
		return rc;
	}

	*aepp = mepp;
	return EOK;
}

/** Insert endpoint pair into map with llink as key.
 *
 * If local port number is not specified, it is allocated.
 *
 * @param map Association map
 * @param epp Endpoint pair, possibly with local port inet_port_any
 * @param arg arg User value
 * @param flags Flags
 * @param aepp Place to store actual endpoint pair, possibly with allocated port
 *
 * @return EOK on success, EEXIST if conflicting epp exists,
 *         ENOMEM if out of memory
 */
static errno_t amap_insert_llink(amap_t *map, inet_ep2_t *epp, void *arg,
    amap_flags_t flags, inet_ep2_t *aepp)
{
	amap_llink_t *llink;
	inet_ep2_t mepp;
	errno_t rc;

	log_msg(LOG_DEFAULT, LVL_DEBUG2, "amap_insert_llink()");

	rc = amap_llink_find(map, epp->local_link, &llink);
	if (rc != EOK) {
		/* New llink */
		rc = amap_llink_insert(map, epp->local_link, &llink);
		if (rc != EOK) {
			assert(rc == ENOMEM);
			return rc;
		}
	}

	mepp = *epp;

	rc = portrng_alloc(llink->portrng, epp->local.port, arg, aflags_to_pflags(flags),
	    &mepp.local.port);
	if (rc != EOK) {
		return rc;
	}

	*aepp = mepp;
	return EOK;
}

/** Insert endpoint pair into map with unspec as key.
 *
 * If local port number is not specified, it is allocated.
 *
 * @param map Association map
 * @param epp Endpoint pair, possibly with local port inet_port_any
 * @param arg arg User value
 * @param flags Flags
 * @param aepp Place to store actual endpoint pair, possibly with allocated port
 *
 * @return EOK on success, EEXIST if conflicting epp exists,
 *         ENOMEM if out of memory
 */
static errno_t amap_insert_unspec(amap_t *map, inet_ep2_t *epp, void *arg,
    amap_flags_t flags, inet_ep2_t *aepp)
{
	inet_ep2_t mepp;
	errno_t rc;

	log_msg(LOG_DEFAULT, LVL_DEBUG2, "amap_insert_unspec()");
	mepp = *epp;

	rc = portrng_alloc(map->unspec, epp->local.port, arg, aflags_to_pflags(flags),
	    &mepp.local.port);
	if (rc != EOK) {
		return rc;
	}

	*aepp = mepp;
	return EOK;
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
 * @return EOK on success, EEXIST if conflicting epp exists,
 *         ENOMEM if out of memory
 */
errno_t amap_insert(amap_t *map, inet_ep2_t *epp, void *arg, amap_flags_t flags,
    inet_ep2_t *aepp)
{
	bool raddr, rport, laddr, llink;
	inet_ep2_t mepp;
	errno_t rc;

	log_msg(LOG_DEFAULT, LVL_DEBUG2, "amap_insert()");

	mepp = *epp;

	/* Fill in local address? */
	if (!inet_addr_is_any(&epp->remote.addr) &&
	    inet_addr_is_any(&epp->local.addr)) {
		log_msg(LOG_DEFAULT, LVL_DEBUG2, "amap_insert: "
		    "determine local address");
		rc = inet_get_srcaddr(&epp->remote.addr, 0, &mepp.local.addr);
		if (rc != EOK) {
			log_msg(LOG_DEFAULT, LVL_DEBUG2, "amap_insert: "
			    "cannot determine local address");
			return rc;
		}
	} else {
		log_msg(LOG_DEFAULT, LVL_DEBUG2, "amap_insert: "
		    "local address specified or remote address not specified");
	}

	raddr = !inet_addr_is_any(&mepp.remote.addr);
	rport = mepp.remote.port != inet_port_any;
	laddr = !inet_addr_is_any(&mepp.local.addr);
	llink = mepp.local_link != 0;

	if (raddr && rport && laddr && !llink) {
		return amap_insert_repla(map, &mepp, arg, flags, aepp);
	} else if (!raddr && !rport && laddr && !llink) {
		return amap_insert_laddr(map, &mepp, arg, flags, aepp);
	} else if (!raddr && !rport && !laddr && llink) {
		return amap_insert_llink(map, &mepp, arg, flags, aepp);
	} else if (!raddr && !rport && !laddr && !llink) {
		return amap_insert_unspec(map, &mepp, arg, flags, aepp);
	} else {
		log_msg(LOG_DEFAULT, LVL_DEBUG2, "amap_insert: invalid "
		    "combination of raddr=%d rport=%d laddr=%d llink=%d",
		    raddr, rport, laddr, llink);
		return EINVAL;
	}

	return EOK;
}

/** Remove endpoint pair using repla as key from map.
 *
 * The endpoint pair must be present in the map, otherwise behavior
 * is unspecified.
 *
 * @param map Association map
 * @param epp Endpoint pair
 */
static void amap_remove_repla(amap_t *map, inet_ep2_t *epp)
{
	amap_repla_t *repla;
	errno_t rc;

	rc = amap_repla_find(map, &epp->remote, &epp->local.addr, &repla);
	if (rc != EOK) {
		log_msg(LOG_DEFAULT, LVL_DEBUG2, "amap_remove_repla: not found");
		return;
	}

	portrng_free_port(repla->portrng, epp->local.port);

	if (portrng_empty(repla->portrng))
		amap_repla_remove(map, repla);
}

/** Remove endpoint pair using laddr as key from map.
 *
 * The endpoint pair must be present in the map, otherwise behavior
 * is unspecified.
 *
 * @param map Association map
 * @param epp Endpoint pair
 */
static void amap_remove_laddr(amap_t *map, inet_ep2_t *epp)
{
	amap_laddr_t *laddr;
	errno_t rc;

	rc = amap_laddr_find(map, &epp->local.addr, &laddr);
	if (rc != EOK) {
		log_msg(LOG_DEFAULT, LVL_DEBUG2, "amap_remove_laddr: not found");
		return;
	}

	portrng_free_port(laddr->portrng, epp->local.port);

	if (portrng_empty(laddr->portrng))
		amap_laddr_remove(map, laddr);
}

/** Remove endpoint pair using llink as key from map.
 *
 * The endpoint pair must be present in the map, otherwise behavior
 * is unspecified.
 *
 * @param map Association map
 * @param epp Endpoint pair
 */
static void amap_remove_llink(amap_t *map, inet_ep2_t *epp)
{
	amap_llink_t *llink;
	errno_t rc;

	rc = amap_llink_find(map, epp->local_link, &llink);
	if (rc != EOK) {
		log_msg(LOG_DEFAULT, LVL_DEBUG2, "amap_remove_llink: not found");
		return;
	}

	portrng_free_port(llink->portrng, epp->local.port);

	if (portrng_empty(llink->portrng))
		amap_llink_remove(map, llink);
}

/** Remove endpoint pair using unspec as key from map.
 *
 * The endpoint pair must be present in the map, otherwise behavior
 * is unspecified.
 *
 * @param map Association map
 * @param epp Endpoint pair
 */
static void amap_remove_unspec(amap_t *map, inet_ep2_t *epp)
{
	portrng_free_port(map->unspec, epp->local.port);
}

/** Remove endpoint pair from map.
 *
 * The endpoint pair must be present in the map, otherwise behavior
 * is unspecified.
 *
 * @param map Association map
 * @param epp Endpoint pair
 */
void amap_remove(amap_t *map, inet_ep2_t *epp)
{
	bool raddr, rport, laddr, llink;

	log_msg(LOG_DEFAULT, LVL_DEBUG2, "amap_remove()");

	raddr = !inet_addr_is_any(&epp->remote.addr);
	rport = epp->remote.port != inet_port_any;
	laddr = !inet_addr_is_any(&epp->local.addr);
	llink = epp->local_link != 0;

	if (raddr && rport && laddr && !llink) {
		amap_remove_repla(map, epp);
	} else if (!raddr && !rport && laddr && !llink) {
		amap_remove_laddr(map, epp);
	} else if (!raddr && !rport && !laddr && llink) {
		amap_remove_llink(map, epp);
	} else if (!raddr && !rport && !laddr && !llink) {
		amap_remove_unspec(map, epp);
	} else {
		log_msg(LOG_DEFAULT, LVL_DEBUG2, "amap_remove: invalid "
		    "combination of raddr=%d rport=%d laddr=%d llink=%d",
		    raddr, rport, laddr, llink);
		return;
	}
}

/** Find association matching an endpoint pair.
 *
 * Used to find which association to deliver a datagram to.
 *
 * @param map	Association map
 * @param epp	Endpoint pair
 * @param rarg	Place to store user argument for the matching association.
 *
 * @return	EOK on success, ENOENT if not found.
 */
errno_t amap_find_match(amap_t *map, inet_ep2_t *epp, void **rarg)
{
	errno_t rc;
	amap_repla_t *repla;
	amap_laddr_t *laddr;
	amap_llink_t *llink;

	log_msg(LOG_DEFAULT, LVL_DEBUG2, "amap_find_match(llink=%zu)",
	    epp->local_link);

	/* Remode endpoint, local address */
	rc = amap_repla_find(map, &epp->remote, &epp->local.addr, &repla);
	if (rc == EOK) {
		rc = portrng_find_port(repla->portrng, epp->local.port,
		    rarg);
		if (rc == EOK) {
			log_msg(LOG_DEFAULT, LVL_DEBUG2, "Matched repla / "
			    "port %" PRIu16, epp->local.port);
			return EOK;
		}
	}

	/* Local address */
	rc = amap_laddr_find(map, &epp->local.addr, &laddr);
	if (rc == EOK) {
		rc = portrng_find_port(laddr->portrng, epp->local.port,
		    rarg);
		if (rc == EOK) {
			log_msg(LOG_DEFAULT, LVL_DEBUG2, "Matched laddr / "
			    "port %" PRIu16, epp->local.port);
			return EOK;
		}
	}

	/* Local link */
	rc = amap_llink_find(map, epp->local_link, &llink);
	if (epp->local_link != 0 && rc == EOK) {
		rc = portrng_find_port(llink->portrng, epp->local.port,
		    rarg);
		if (rc == EOK) {
			log_msg(LOG_DEFAULT, LVL_DEBUG2, "Matched llink / "
			    "port %" PRIu16, epp->local.port);
			return EOK;
		}
	}

	/* Unspecified */
	rc = portrng_find_port(map->unspec, epp->local.port, rarg);
	if (rc == EOK) {
		log_msg(LOG_DEFAULT, LVL_DEBUG2, "Matched unspec / port %" PRIu16,
		    epp->local.port);
		return EOK;
	}

	log_msg(LOG_DEFAULT, LVL_DEBUG2, "No match.");
	return ENOENT;
}

/**
 * @}
 */
