/*
 * SPDX-FileCopyrightText: 2015 Jiri Svoboda
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup libnettl
 * @{
 */
/**
 * @file Association map.
 */

#ifndef LIBNETTL_AMAP_H_
#define LIBNETTL_AMAP_H_

#include <adt/list.h>
#include <inet/endpoint.h>
#include <nettl/portrng.h>
#include <loc.h>

/** Port range for (remote endpoint, local address) */
typedef struct {
	/** Link to amap_t.repla */
	link_t lamap;
	/** Remote endpoint */
	inet_ep_t rep;
	/* Local address */
	inet_addr_t laddr;
	/** Port range */
	portrng_t *portrng;
} amap_repla_t;

/** Port range for local address */
typedef struct {
	/** Link to amap_t.laddr */
	link_t lamap;
	/** Local address */
	inet_addr_t laddr;
	/** Port range */
	portrng_t *portrng;
} amap_laddr_t;

/** Port range for local link */
typedef struct {
	/** Link to amap_t.llink */
	link_t lamap;
	/** Local link ID */
	service_id_t llink;
	/** Port range */
	portrng_t *portrng;
} amap_llink_t;

/** Association map */
typedef struct {
	/** Remote endpoint, local address */
	list_t repla; /* of amap_repla_t */
	/** Local addresses */
	list_t laddr; /* of amap_laddr_t */
	/** Local links */
	list_t llink; /* of amap_llink_t */
	/** Nothing specified (listen on all local addresses) */
	portrng_t *unspec;
} amap_t;

typedef enum {
	/** Allow specifying port number from system range */
	af_allow_system = 0x1
} amap_flags_t;

extern errno_t amap_create(amap_t **);
extern void amap_destroy(amap_t *);
extern errno_t amap_insert(amap_t *, inet_ep2_t *, void *, amap_flags_t,
    inet_ep2_t *);
extern void amap_remove(amap_t *, inet_ep2_t *);
extern errno_t amap_find_match(amap_t *, inet_ep2_t *, void **);

#endif

/** @}
 */
