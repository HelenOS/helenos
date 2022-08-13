/*
 * SPDX-FileCopyrightText: 2015 Jiri Svoboda
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup libnettl
 * @{
 */
/**
 * @file Port range allocator.
 */

#ifndef LIBNETTL_PORTRNG_H_
#define LIBNETTL_PORTRNG_H_

#include <stdbool.h>
#include <stdint.h>

/** Allocated port */
typedef struct {
	/** Link to portrng_t.used */
	link_t lprng;
	/** Port number */
	uint16_t pn;
	/** User argument */
	void *arg;
} portrng_port_t;

typedef struct {
	list_t used; /* of portrng_port_t */
} portrng_t;

typedef enum {
	pf_allow_system = 0x1
} portrng_flags_t;

extern errno_t portrng_create(portrng_t **);
extern void portrng_destroy(portrng_t *);
extern errno_t portrng_alloc(portrng_t *, uint16_t, void *,
    portrng_flags_t, uint16_t *);
extern errno_t portrng_find_port(portrng_t *, uint16_t, void **);
extern void portrng_free_port(portrng_t *, uint16_t);
extern bool portrng_empty(portrng_t *);

#endif

/** @}
 */
