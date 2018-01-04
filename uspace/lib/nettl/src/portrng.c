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
 * @file Port range allocator
 *
 * Allocates port numbers from IETF port number ranges.
 */

#include <adt/list.h>
#include <errno.h>
#include <inet/endpoint.h>
#include <nettl/portrng.h>
#include <stdint.h>
#include <stdlib.h>

#include <io/log.h>

/** Create port range.
 *
 * @param rpr Place to store pointer to new port range
 * @return EOK on success, ENOMEM if out of memory
 */
errno_t portrng_create(portrng_t **rpr)
{
	portrng_t *pr;

	log_msg(LOG_DEFAULT, LVL_DEBUG2, "portrng_create() - begin");

	pr = calloc(1, sizeof(portrng_t));
	if (pr == NULL)
		return ENOMEM;

	list_initialize(&pr->used);
	*rpr = pr;
	log_msg(LOG_DEFAULT, LVL_DEBUG2, "portrng_create() - end");
	return EOK;
}

/** Destroy port range.
 *
 * @param pr Port range
 */
void portrng_destroy(portrng_t *pr)
{
	log_msg(LOG_DEFAULT, LVL_DEBUG2, "portrng_destroy()");
	assert(list_empty(&pr->used));
	free(pr);
}

/** Allocate port number from port range.
 *
 * @param pr    Port range
 * @param pnum  Port number to allocate specific port, or zero to allocate
 *              any valid port from range
 * @param arg   User argument to set for port
 * @param flags Flags, @c pf_allow_system to allow ports from system range
 *              to be specified by @a pnum.
 * @param apnum Place to store allocated port number
 *
 * @return EOK on success, ENOENT if no free port number found, EEXIST
 *         if @a pnum is specified but it is already allocated,
 *         EINVAL if @a pnum is specified from the system range, but
 *         @c pf_allow_system was not set.
 */
errno_t portrng_alloc(portrng_t *pr, uint16_t pnum, void *arg,
    portrng_flags_t flags, uint16_t *apnum)
{
	portrng_port_t *p;
	uint32_t i;
	bool found;

	log_msg(LOG_DEFAULT, LVL_DEBUG2, "portrng_alloc() - begin");

	if (pnum == inet_port_any) {

		for (i = inet_port_dyn_lo; i <= inet_port_dyn_hi; i++) {
			log_msg(LOG_DEFAULT, LVL_DEBUG2, "trying %" PRIu32, i);
			found = false;
			list_foreach(pr->used, lprng, portrng_port_t, port) {
				if (port->pn == pnum) {
					found = true;
					break;
				}
			}

			if (!found) {
				pnum = i;
				break;
			}
		}

		if (pnum == inet_port_any) {
			/* No free port found */
			return ENOENT;
		}
		log_msg(LOG_DEFAULT, LVL_DEBUG2, "selected %" PRIu16, pnum);
	} else {
		log_msg(LOG_DEFAULT, LVL_DEBUG2, "user asked for %" PRIu16, pnum);

		if ((flags & pf_allow_system) == 0 &&
		    pnum < inet_port_user_lo) {
			log_msg(LOG_DEFAULT, LVL_DEBUG2, "system port not allowed");
			return EINVAL;
		}

		list_foreach(pr->used, lprng, portrng_port_t, port) {
			if (port->pn == pnum) {
				log_msg(LOG_DEFAULT, LVL_DEBUG2, "port already used");
				return EEXIST;
			}
		}
	}

	p = calloc(1, sizeof(portrng_port_t));
	if (p == NULL)
		return ENOMEM;

	p->pn = pnum;
	p->arg = arg;
	list_append(&p->lprng, &pr->used);
	*apnum = pnum;
	log_msg(LOG_DEFAULT, LVL_DEBUG2, "portrng_alloc() - end OK pn=%" PRIu16,
	    pnum);
	return EOK;
}

/** Find allocated port number and return its argument.
 *
 * @param pr   Port range
 * @param pnum Port number
 * @param rarg Place to store user argument
 *
 * @return EOK on success, ENOENT if specified port number is not allocated
 */
errno_t portrng_find_port(portrng_t *pr, uint16_t pnum, void **rarg)
{
	list_foreach(pr->used, lprng, portrng_port_t, port) {
		if (port->pn == pnum) {
			*rarg = port->arg;
			return EOK;
		}
	}

	return ENOENT;
}

/** Free port in port range.
 *
 * @param pr   Port range
 * @param pnum Port number
 */
void portrng_free_port(portrng_t *pr, uint16_t pnum)
{
	log_msg(LOG_DEFAULT, LVL_DEBUG2, "portrng_free_port(%u)", pnum);
	log_msg(LOG_DEFAULT, LVL_DEBUG2, "portrng_free_port() - begin");
	list_foreach(pr->used, lprng, portrng_port_t, port) {
		log_msg(LOG_DEFAULT, LVL_DEBUG2, "portrng_free_port - check port %u", port->pn);
		if (port->pn == pnum) {
			log_msg(LOG_DEFAULT, LVL_DEBUG2, "portrng_free_port - OK");
			list_remove(&port->lprng);
			free(port);
			log_msg(LOG_DEFAULT, LVL_DEBUG2, "portrng_free_port() - end");
			return;
		}
	}

	log_msg(LOG_DEFAULT, LVL_DEBUG2, "portrng_free_port - FAIL");
	assert(false);
}

/** Determine if port range is empty.
 *
 * @param pr Port range
 * @return @c true if no ports are allocated from @a pr, @c false otherwise
 */
bool portrng_empty(portrng_t *pr)
{
	log_msg(LOG_DEFAULT, LVL_DEBUG2, "portrng_empty()");
	return list_empty(&pr->used);
}

/**
 * @}
 */
