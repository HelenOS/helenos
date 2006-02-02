/*
 * Copyright (C) 2006 Ondrej Palkovsky
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

#include <synch/spinlock.h>
#include <mm/slab.h>
#include <list.h>
#include <memstr.h>
#include <align.h>
#include <mm/heap.h>
#include <config.h>
#include <print.h>
#include <arch.h>
#include <panic.h>

SPINLOCK_INITIALIZE(slab_cache_lock);
LIST_INITIALIZE(slab_cache_list);

slab_cache_t mag_cache;

/**************************************/
/* SLAB low level functions */


/**
 * Return object to slab and call a destructor
 *
 * @return Number of freed pages
 */
static count_t slab_obj_destroy(slab_cache_t *cache, void *obj)
{
	return 0;
}


/**
 * Take new object from slab or create new if needed
 *
 * @return Object address or null
 */
static void * slab_obj_create(slab_cache_t *cache, int flags)
{
	return NULL;
}

/**************************************/
/* CPU-Cache slab functions */

/**
 * Free all objects in magazine and free memory associated with magazine
 *
 * Assume cpu->lock is locked 
 *
 * @return Number of freed pages
 */
static count_t magazine_destroy(slab_cache_t *cache, 
				slab_magazine_t *mag)
{
	int i;
	count_t frames = 0;

	for (i=0;i < mag->busy; i++)
		frames += slab_obj_destroy(cache, mag->objs[i]);
	
	slab_free(&mag_cache, mag);

	return frames;
}

/**
 * Try to find object in CPU-cache magazines
 *
 * @return Pointer to object or NULL if not available
 */
static void * magazine_obj_get(slab_cache_t *cache)
{
	slab_magazine_t *mag;

	spinlock_lock(&cache->mag_cache[CPU->id].lock);

	mag = cache->mag_cache[CPU->id].current;
	if (!mag)
		goto out;

	if (!mag->busy) {
		/* If current is empty && last exists && not empty, exchange */
		if (cache->mag_cache[CPU->id].last \
		    && cache->mag_cache[CPU->id].last->busy) {
			cache->mag_cache[CPU->id].current = cache->mag_cache[CPU->id].last;
			cache->mag_cache[CPU->id].last = mag;
			mag = cache->mag_cache[CPU->id].current;
			goto gotit;
		}
		/* If still not busy, exchange current with some frome
		 * other full magazines */
		spinlock_lock(&cache->lock);
		if (list_empty(&cache->magazines)) {
			spinlock_unlock(&cache->lock);
			goto out;
		}
		/* Free current magazine and take one from list */
		slab_free(&mag_cache, mag);
		mag = list_get_instance(cache->magazines.next,
					slab_magazine_t,
					link);
		list_remove(&mag->link);
		
		spinlock_unlock(&cache->lock);
	}
gotit:
	spinlock_unlock(&cache->mag_cache[CPU->id].lock);
	return mag->objs[--mag->busy];
out:	
	spinlock_unlock(&cache->mag_cache[CPU->id].lock);
	return NULL;
}

/**
 * Put object into CPU-cache magazine
 *
 * We have 2 magazines bound to processor. 
 * First try the current. 
 *  If full, try the last.
 *   If full, put to magazines list.
 *   allocate new, exchange last & current
 *
 * @return 0 - success, -1 - could not get memory
 */
static int magazine_obj_put(slab_cache_t *cache, void *obj)
{
	slab_magazine_t *mag;

	spinlock_lock(&cache->mag_cache[CPU->id].lock);
	
	mag = cache->mag_cache[CPU->id].current;
	if (!mag) {
		/* We do not want to sleep just because of caching */
		/* Especially we do not want reclaiming to start, as 
		 * this would deadlock */
		mag = slab_alloc(&mag_cache, SLAB_ATOMIC | SLAB_NO_RECLAIM);
		if (!mag) /* Allocation failed, give up on caching */
			goto errout;

		cache->mag_cache[CPU->id].current = mag;
		mag->size = SLAB_MAG_SIZE;
		mag->busy = 0;
	} else if (mag->busy == mag->size) {
		/* If the last is full | empty, allocate new */
		mag = cache->mag_cache[CPU->id].last;
		if (!mag || mag->size == mag->busy) {
			if (mag) 
				list_prepend(&cache->magazines, &mag->link);

			mag = slab_alloc(&mag_cache, SLAB_ATOMIC | SLAB_NO_RECLAIM);
			if (!mag)
				goto errout;
			
			mag->size = SLAB_MAG_SIZE;
			mag->busy = 0;
			cache->mag_cache[CPU->id].last = mag;
		} 
		/* Exchange the 2 */
		cache->mag_cache[CPU->id].last = cache->mag_cache[CPU->id].current;
		cache->mag_cache[CPU->id].current = mag;
	}
	mag->objs[mag->busy++] = obj;

	spinlock_unlock(&cache->mag_cache[CPU->id].lock);
	return 0;
errout:
	spinlock_unlock(&cache->mag_cache[CPU->id].lock);
	return -1;
}


/**************************************/
/* Top level SLAB functions */

/** Initialize allocated memory as a slab cache */
static void
_slab_cache_create(slab_cache_t *cache,
		   char *name,
		   size_t size,
		   size_t align,
		   int (*constructor)(void *obj, int kmflag),
		   void (*destructor)(void *obj),
		   int flags)
{
	int i;

	memsetb((__address)cache, sizeof(*cache), 0);
	cache->name = name;
	cache->align = align;

	cache->size = ALIGN_UP(size, align);

	cache->constructor = constructor;
	cache->destructor = destructor;
	cache->flags = flags;

	list_initialize(&cache->full_slabs);
	list_initialize(&cache->partial_slabs);
	list_initialize(&cache->magazines);
	spinlock_initialize(&cache->lock, "cachelock");
	if (! cache->flags & SLAB_CACHE_NOMAGAZINE) {
		for (i=0; i< config.cpu_count; i++)
			spinlock_initialize(&cache->mag_cache[i].lock, 
					    "cpucachelock");
	}

	/* Compute slab sizes, object counts in slabs etc. */
	if (cache->size < SLAB_INSIDE_SIZE)
		cache->flags |= SLAB_CACHE_SLINSIDE;

	

	spinlock_lock(&slab_cache_lock);

	list_append(&cache->link, &slab_cache_list);

	spinlock_unlock(&slab_cache_lock);
}

/** Create slab cache  */
slab_cache_t * slab_cache_create(char *name,
				 size_t size,
				 size_t align,
				 int (*constructor)(void *obj, int kmflag),
				 void (*destructor)(void *obj),
				 int flags)
{
	slab_cache_t *cache;

	cache = malloc(sizeof(*cache) + config.cpu_count*sizeof(cache->mag_cache[0]));
	_slab_cache_create(cache, name, size, align, constructor, destructor,
			   flags);
	return cache;
}

/** 
 * Reclaim space occupied by objects that are already free
 *
 * @param flags If contains SLAB_RECLAIM_ALL, do aggressive freeing
 * @return Number of freed pages
 */
static count_t _slab_reclaim(slab_cache_t *cache, int flags)
{
	int i;
	slab_magazine_t *mag;
	link_t *cur;
	count_t frames = 0;
	
	if (cache->flags & SLAB_CACHE_NOMAGAZINE)
		return 0; /* Nothing to do */
	
	/* First lock all cpu caches, then the complete cache lock */
	for (i=0; i < config.cpu_count; i++)
		spinlock_lock(&cache->mag_cache[i].lock);
	spinlock_lock(&cache->lock);
	
	if (flags & SLAB_RECLAIM_ALL) {
		/* Destroy CPU magazines */
		for (i=0; i<config.cpu_count; i++) {
			mag = cache->mag_cache[i].current;
			if (mag)
				frames += magazine_destroy(cache, mag);
			cache->mag_cache[i].current = NULL;
			
			mag = cache->mag_cache[i].last;
			if (mag)
				frames += magazine_destroy(cache, mag);
			cache->mag_cache[i].last = NULL;
		}
		/* Destroy full magazines */
		cur=cache->magazines.next;
		while (cur!=&cache->magazines) {
			mag = list_get_instance(cur, slab_magazine_t, link);
			
			cur = cur->next;
			list_remove(cur->prev);
			frames += magazine_destroy(cache,mag);
		}
	}
	
	spinlock_unlock(&cache->lock);
	for (i=0; i < config.cpu_count; i++)
		spinlock_unlock(&cache->mag_cache[i].lock);
	
	return frames;
}

/** Check that there are no slabs and remove cache from system  */
void slab_cache_destroy(slab_cache_t *cache)
{
	/* Do not lock anything, we assume the software is correct and
	 * does not touch the cache when it decides to destroy it */
	
	/* Destroy all magazines */
	_slab_reclaim(cache, SLAB_RECLAIM_ALL);

	/* All slabs must be empty */
	if (!list_empty(&cache->full_slabs) \
	    || !list_empty(&cache->partial_slabs))
		panic("Destroying cache that is not empty.");

	spinlock_lock(&slab_cache_lock);
	list_remove(&cache->link);
	spinlock_unlock(&slab_cache_lock);

	free(cache);
}

/** Allocate new object from cache - if no flags given, always returns 
    memory */
void * slab_alloc(slab_cache_t *cache, int flags)
{
	ipl_t ipl;
	void *result = NULL;

	/* Disable interrupts to avoid deadlocks with interrupt handlers */
	ipl = interrupts_disable();
	
	if (!cache->flags & SLAB_CACHE_NOMAGAZINE)
		result = magazine_obj_get(cache);

	if (!result)
		result = slab_obj_create(cache, flags);

	interrupts_restore(ipl);

	return result;
}

/** Return object to cache  */
void slab_free(slab_cache_t *cache, void *obj)
{
	ipl_t ipl;

	ipl = interrupts_disable();

	if (cache->flags & SLAB_CACHE_NOMAGAZINE)
		slab_obj_destroy(cache, obj);
	else {
		if (magazine_obj_put(cache, obj)) /* If magazine put failed */
			slab_obj_destroy(cache, obj);
	}
	interrupts_restore(ipl);
}

/* Go through all caches and reclaim what is possible */
count_t slab_reclaim(int flags)
{
	slab_cache_t *cache;
	link_t *cur;
	count_t frames = 0;

	spinlock_lock(&slab_cache_lock);

	for (cur = slab_cache_list.next;cur!=&slab_cache_list; cur=cur->next) {
		cache = list_get_instance(cur, slab_cache_t, link);
		frames += _slab_reclaim(cache, flags);
	}

	spinlock_unlock(&slab_cache_lock);

	return frames;
}


/* Print list of slabs */
void slab_print_list(void)
{
	slab_cache_t *cache;
	link_t *cur;

	spinlock_lock(&slab_cache_lock);
	printf("SLAB name\tObj size\n");
	for (cur = slab_cache_list.next;cur!=&slab_cache_list; cur=cur->next) {
		cache = list_get_instance(cur, slab_cache_t, link);
		printf("%s\t%d\n", cache->name, cache->size);
	}
	spinlock_unlock(&slab_cache_lock);
}

void slab_cache_init(void)
{
	/* Initialize magazine cache */
	_slab_cache_create(&mag_cache,
			   "slab_magazine",
			   sizeof(slab_magazine_t)+SLAB_MAG_SIZE*sizeof(void*),
			   sizeof(__address),
			   NULL, NULL,
			   SLAB_CACHE_NOMAGAZINE);

	/* Initialize structures for malloc */
}
