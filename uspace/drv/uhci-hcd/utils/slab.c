/*
 * Copyright (c) 2011 Jan Vesely
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
/** @addtogroup usb
 * @{
 */
/** @file
 * @brief UHCI driver
 */
#include <as.h>
#include <assert.h>
#include <fibril_synch.h>
#include <usb/debug.h>

#include "slab.h"

#define SLAB_SIZE (PAGE_SIZE * 16)
#define SLAB_ELEMENT_COUNT (SLAB_SIZE / SLAB_ELEMENT_SIZE)

typedef struct slab {
	void *page;
	bool slabs[SLAB_ELEMENT_COUNT];
	fibril_mutex_t guard;
} slab_t;

static slab_t global_slab;

static void * slab_malloc(slab_t *intance);
static bool slab_in_range(slab_t *intance, void *addr);
static void slab_free(slab_t *intance, void *addr);
/*----------------------------------------------------------------------------*/
void * slab_malloc_g(void)
{
	return slab_malloc(&global_slab);
}
/*----------------------------------------------------------------------------*/
void slab_free_g(void *addr)
{
	return slab_free(&global_slab, addr);
}
/*----------------------------------------------------------------------------*/
bool slab_in_range_g(void *addr)
{
	return slab_in_range(&global_slab, addr);
}
/*----------------------------------------------------------------------------*/
static void slab_init(slab_t *instance)
{
	static FIBRIL_MUTEX_INITIALIZE(init_mutex);
	assert(instance);
	fibril_mutex_lock(&init_mutex);
	if (instance->page != NULL) {
		/* already initialized */
		fibril_mutex_unlock(&init_mutex);
		return;
	}
	fibril_mutex_initialize(&instance->guard);
	size_t i = 0;
	for (;i < SLAB_ELEMENT_COUNT; ++i) {
		instance->slabs[i] = true;
	}
	instance->page = as_get_mappable_page(SLAB_SIZE);
	if (instance->page != NULL) {
		void* ret =
		    as_area_create(instance->page, SLAB_SIZE, AS_AREA_READ | AS_AREA_WRITE);
		if (ret != instance->page) {
			instance->page = NULL;
		}
	}
	memset(instance->page, 0xa, SLAB_SIZE);
	fibril_mutex_unlock(&init_mutex);
	usb_log_fatal("SLAB initialized at %p.\n", instance->page);
}
/*----------------------------------------------------------------------------*/
static void * slab_malloc(slab_t *instance) {
	assert(instance);
	if (instance->page == NULL)
		slab_init(instance);

	fibril_mutex_lock(&instance->guard);
	void *addr = NULL;
	size_t i = 0;
	for (; i < SLAB_ELEMENT_COUNT; ++i) {
		if (instance->slabs[i]) {
			instance->slabs[i] = false;
			addr = (instance->page + (i * SLAB_ELEMENT_SIZE));
			break;
		}
	}
	fibril_mutex_unlock(&instance->guard);

	usb_log_fatal("SLAB allocated address element %zu(%p).\n", i, addr);
	return addr;
}
/*----------------------------------------------------------------------------*/
static bool slab_in_range(slab_t *instance, void *addr) {
	assert(instance);
	bool in_range = (instance->page != NULL) &&
		(addr >= instance->page) && (addr < instance->page + SLAB_SIZE);
//	usb_log_fatal("SLAB address %sin range %p(%p-%p).\n",
//	    in_range ? "" : "NOT ", addr, instance->page, instance->page + SLAB_SIZE);
	return in_range;
}
/*----------------------------------------------------------------------------*/
static void slab_free(slab_t *instance, void *addr)
{
	assert(instance);
	assert(slab_in_range(instance, addr));
	memset(addr, 0xa, SLAB_ELEMENT_SIZE);

	const size_t pos = (addr - instance->page) /  SLAB_ELEMENT_SIZE;

	fibril_mutex_lock(&instance->guard);
	assert(instance->slabs[pos] == false);
	instance->slabs[pos] = true;
	fibril_mutex_unlock(&instance->guard);

	usb_log_fatal("SLAB freed element %zu(%p).\n", pos, addr);
}
/**
 * @}
 */
