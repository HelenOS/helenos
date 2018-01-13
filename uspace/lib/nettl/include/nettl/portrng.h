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
