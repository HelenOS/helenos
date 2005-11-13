/*
 * Copyright (C) 2005 Jakub Jermar
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

#ifndef __BUDDY_H__
#define __BUDDY_H__

#include <arch/types.h>
#include <typedefs.h>

#define BUDDY_SYSTEM_INNER_BLOCK	0xff

struct buddy_system_operations {
	link_t *(* find_buddy)(link_t *);				/**< Return pointer to left-side or right-side buddy for block passed as argument. */
	link_t *(* bisect)(link_t *);					/**< Bisect the block passed as argument and return pointer to the new right-side buddy. */
	link_t *(* coalesce)(link_t *, link_t *);			/**< Coalesce two buddies into a bigger block. */
	void (*set_order)(link_t *, __u8);				/**< Set order of block passed as argument. */
	__u8 (*get_order)(link_t *);					/**< Return order of block passed as argument. */
};

struct buddy_system {
	__u8 max_order;
	link_t *order;
	buddy_system_operations_t *op;
};

extern buddy_system_t *buddy_system_create(__u8 max_order, buddy_system_operations_t *op);
extern link_t *buddy_system_alloc(buddy_system_t *b, __u8 i);
extern void buddy_system_free(buddy_system_t *b, link_t *block);

#endif
