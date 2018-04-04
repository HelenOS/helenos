/*
 * Copyright (c) 2012 Jan Vesely
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

/** @addtogroup libc
 * @{
 */
/** @file
 */

#include <assert.h>
#include <errno.h>
#include <stdlib.h>
#include <adt/list.h>
#include <fibril_synch.h>
#include <ddi.h>
#include <str.h>


typedef struct {
	link_t link;
	volatile void *base;
	size_t size;
	void *data;
	trace_fnc log;
} region_t;

static inline region_t *region_instance(link_t *l)
{
	return list_get_instance(l, region_t, link);
}

static inline region_t *region_create(volatile void *base, size_t size,
    trace_fnc log, void *data)
{
	region_t *new_reg = malloc(sizeof(region_t));
	if (new_reg) {
		link_initialize(&new_reg->link);
		new_reg->base = base;
		new_reg->data = data;
		new_reg->size = size;
		new_reg->log = log;
	}
	return new_reg;
}

static inline void region_destroy(region_t *r)
{
	free(r);
}

typedef struct {
	list_t list;
	fibril_rwlock_t guard;
} pio_regions_t;

static pio_regions_t *get_regions(void)
{
	static pio_regions_t regions = {
		.list = {
			.head = { &regions.list.head, &regions.list.head },
		},
		.guard = FIBRIL_RWLOCK_INITIALIZER(regions.guard),
	};
	return &regions;
}


void pio_trace_log(const volatile void *r, uint64_t val, bool write)
{
	pio_regions_t *regions = get_regions();
	fibril_rwlock_read_lock(&regions->guard);
	list_foreach(regions->list, link, region_t, reg) {
		if ((r >= reg->base) && (r < reg->base + reg->size)) {
			if (reg->log)
				reg->log(r, val, reg->base,
				    reg->size, reg->data, write);
			break;
		}
	}
	fibril_rwlock_read_unlock(&regions->guard);
}

errno_t pio_trace_enable(void *base, size_t size, trace_fnc log, void *data)
{
	pio_regions_t *regions = get_regions();
	assert(regions);

	region_t *region = region_create(base, size, log, data);
	if (!region)
		return ENOMEM;

	fibril_rwlock_write_lock(&regions->guard);
	list_append(&region->link, &regions->list);
	fibril_rwlock_write_unlock(&regions->guard);
	return EOK;
}

void pio_trace_disable(void *r)
{
	pio_regions_t *regions = get_regions();
	assert(regions);

	fibril_rwlock_write_lock(&regions->guard);
	list_foreach_safe(regions->list, it, next) {
		region_t *reg = region_instance(it);
		if (r >= reg->base && (r < reg->base + reg->size)) {
			list_remove(&reg->link);
			region_destroy(reg);
		}
	}
	fibril_rwlock_write_unlock(&regions->guard);
}

/** @}
 */
