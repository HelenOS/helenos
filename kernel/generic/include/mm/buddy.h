/*
 * Copyright (c) 2005 Jakub Jermar
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

/** @addtogroup genericmm
 * @{
 */
/** @file
 */

#ifndef KERN_BUDDY_H_
#define KERN_BUDDY_H_

#include <typedefs.h>
#include <adt/list.h>

#define BUDDY_SYSTEM_INNER_BLOCK	0xff

struct buddy_system;

/** Buddy system operations to be implemented by each implementation. */
typedef struct {
	/**
	 * Return pointer to left-side or right-side buddy for block passed as
	 * argument.
	 */
	link_t *(* find_buddy)(struct buddy_system *, link_t *);
	/**
	 * Bisect the block passed as argument and return pointer to the new
	 * right-side buddy.
	 */
	link_t *(* bisect)(struct buddy_system *, link_t *);
	/** Coalesce two buddies into a bigger block. */
	link_t *(* coalesce)(struct buddy_system *, link_t *, link_t *);
	/** Set order of block passed as argument. */
	void (*set_order)(struct buddy_system *, link_t *, uint8_t);
	/** Return order of block passed as argument. */
	uint8_t (*get_order)(struct buddy_system *, link_t *);
	/** Mark block as busy. */
	void (*mark_busy)(struct buddy_system *, link_t *);
	/** Mark block as available. */
	void (*mark_available)(struct buddy_system *, link_t *);
	/** Find parent of block that has given order  */
	link_t *(* find_block)(struct buddy_system *, link_t *, uint8_t);
} buddy_system_operations_t;

typedef struct buddy_system {
	/** Maximal order of block which can be stored by buddy system. */
	uint8_t max_order;
	list_t *order;
	buddy_system_operations_t *op;
	/** Pointer to be used by the implementation. */
	void *data;
} buddy_system_t;

extern void buddy_system_create(buddy_system_t *, uint8_t,
    buddy_system_operations_t *, void *);
extern link_t *buddy_system_alloc(buddy_system_t *, uint8_t);
extern bool buddy_system_can_alloc(buddy_system_t *, uint8_t);
extern void buddy_system_free(buddy_system_t *, link_t *);
extern size_t buddy_conf_size(size_t);
extern link_t *buddy_system_alloc_block(buddy_system_t *, link_t *);

#endif

/** @}
 */
