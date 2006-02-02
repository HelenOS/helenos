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
#include <mm/frame.h>
#include <config.h>
#include <print.h>
#include <arch.h>
#include <panic.h>
#include <debug.h>

SPINLOCK_INITIALIZE(slab_cache_lock);
LIST_INITIALIZE(slab_cache_list);

slab_cache_t mag_cache;


typedef struct {
	slab_cache_t *cache; /**< Pointer to parent cache */
	link_t link;       /* List of full/partial slabs */
	void *start;       /**< Start address of first available item */
	count_t available; /**< Count of available items in this slab */
	index_t nextavail; /**< The index of next available item */
}slab_t;

/**************************************/
/* SLAB allocation functions          */

/**
 * Allocate frames for slab space and initialize
 *
 * TODO: Change slab_t allocation to slab_alloc(????), malloc with flags!!
 */
static slab_t * slab_space_alloc(slab_cache_t *cache, int flags)
{
	void *data;
	slab_t *slab;
	size_t fsize;
	int i;
	zone_t *zone = NULL;
	int status;

	data = (void *)frame_alloc(FRAME_KA | flags, cache->order, &status, &zone);
	if (status != FRAME_OK)
		return NULL;

	if (! cache->flags & SLAB_CACHE_SLINSIDE) {
		slab = malloc(sizeof(*slab)); // , flags);
		if (!slab) {
			frame_free((__address)data);
			return NULL;
		}
	} else {
		fsize = (PAGE_SIZE << cache->order);
		slab = data + fsize - sizeof(*slab);
	}

	/* Fill in slab structures */
	/* TODO: some better way of accessing the frame */
	for (i=0; i< (1<<cache->order); i++) {
		ADDR2FRAME(zone, (__address)(data+i*PAGE_SIZE))->parent = slab;
	}

	slab->start = data;
	slab->available = cache->objects;
	slab->nextavail = 0;

	for (i=0; i<cache->objects;i++)
		*((int *) (slab->start + i*cache->size)) = i+1;
	return slab;
}

/**
 * Free space associated with SLAB
 *
 * @return number of freed frames
 */
static count_t slab_space_free(slab_cache_t *cache, slab_t *slab)
{
	frame_free((__address)slab->start);
	if (! cache->flags & SLAB_CACHE_SLINSIDE)
		free(slab);
	return 1 << cache->order;
}

/** Map object to slab structure */
static slab_t * obj2slab(void *obj)
{
	frame_t *frame; 

	frame = frame_addr2frame((__address)obj);
	return (slab_t *)frame->parent;
}

/**************************************/
/* SLAB functions */


/**
 * Return object to slab and call a destructor
 *
 * Assume the cache->lock is held;
 *
 * @param slab If the caller knows directly slab of the object, otherwise NULL
 *
 * @return Number of freed pages
 */
static count_t slab_obj_destroy(slab_cache_t *cache, void *obj,
				slab_t *slab)
{
	count_t frames = 0;

	if (!slab)
		slab = obj2slab(obj);

	spinlock_lock(&cache->lock);

	*((int *)obj) = slab->nextavail;
	slab->nextavail = (obj - slab->start)/cache->size;
	slab->available++;

	/* Move it to correct list */
	if (slab->available == 1) {
		/* It was in full, move to partial */
		list_remove(&slab->link);
		list_prepend(&cache->partial_slabs, &slab->link);
	}
	if (slab->available == cache->objects) {
		/* Free associated memory */
		list_remove(&slab->link);
		/* Avoid deadlock */
		spinlock_unlock(&cache->lock);
		frames = slab_space_free(cache, slab);
		spinlock_lock(&cache->lock);
	}

	spinlock_unlock(&cache->lock);

	return frames;
}

/**
 * Take new object from slab or create new if needed
 *
 * Assume cache->lock is held. 
 *
 * @return Object address or null
 */
static void * slab_obj_create(slab_cache_t *cache, int flags)
{
	slab_t *slab;
	void *obj;

	if (list_empty(&cache->partial_slabs)) {
		/* Allow recursion and reclaiming
		 * - this should work, as the SLAB control structures
		 *   are small and do not need to allocte with anything
		 *   other ten frame_alloc when they are allocating,
		 *   that's why we should get recursion at most 1-level deep
		 */
		spinlock_unlock(&cache->lock);
		slab = slab_space_alloc(cache, flags);
		spinlock_lock(&cache->lock);
		if (!slab)
			return NULL;
	} else {
		slab = list_get_instance(cache->partial_slabs.next,
					 slab_t,
					 link);
		list_remove(&slab->link);
	}
	obj = slab->start + slab->nextavail * cache->size;
	slab->nextavail = *((int *)obj);
	slab->available--;
	if (! slab->available)
		list_prepend(&cache->full_slabs, &slab->link);
	else
		list_prepend(&cache->partial_slabs, &slab->link);
	return obj;
}

/**************************************/
/* CPU-Cache slab functions */

/**
 * Free all objects in magazine and free memory associated with magazine
 *
 * Assume mag_cache[cpu].lock is locked 
 *
 * @return Number of freed pages
 */
static count_t magazine_destroy(slab_cache_t *cache, 
				slab_magazine_t *mag)
{
	int i;
	count_t frames = 0;

	for (i=0;i < mag->busy; i++)
		frames += slab_obj_destroy(cache, mag->objs[i], NULL);
	
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
		/* If still not busy, exchange current with some from
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
		mag = slab_alloc(&mag_cache, FRAME_ATOMIC | FRAME_NO_RECLAIM);
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

			mag = slab_alloc(&mag_cache, FRAME_ATOMIC | FRAME_NO_RECLAIM);
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
/* SLAB CACHE functions */

/** Return number of objects that fit in certain cache size */
static int comp_objects(slab_cache_t *cache)
{
	if (cache->flags & SLAB_CACHE_SLINSIDE)
		return ((PAGE_SIZE << cache->order) - sizeof(slab_t)) / cache->size;
	else 
		return (PAGE_SIZE << cache->order) / cache->size;
}

/** Return wasted space in slab */
static int badness(slab_cache_t *cache)
{
	int objects;
	int ssize;

	objects = comp_objects(cache);
	ssize = PAGE_SIZE << cache->order;
	if (cache->flags & SLAB_CACHE_SLINSIDE)
		ssize -= sizeof(slab_t);
	return ssize - objects*cache->size;
}

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

	if (align)
		size = ALIGN_UP(size, align);
	cache->size = size;

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

	/* Minimum slab order */
	cache->order = (cache->size / PAGE_SIZE) + 1;
		
	while (badness(cache) > SLAB_MAX_BADNESS(cache)) {
		cache->order += 1;
	}

	cache->objects = comp_objects(cache);

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
 *
 * TODO: Add light reclaim
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
		/* Aggressive memfree */

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
	}
	/* Destroy full magazines */
	cur=cache->magazines.prev;
	while (cur!=&cache->magazines) {
		mag = list_get_instance(cur, slab_magazine_t, link);
		
		cur = cur->prev;
		list_remove(cur->next);
		frames += magazine_destroy(cache,mag);
		/* If we do not do full reclaim, break
		 * as soon as something is freed */
		if (!(flags & SLAB_RECLAIM_ALL) && frames)
			break;
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

	if (!result) {
		spinlock_lock(&cache->lock);
		result = slab_obj_create(cache, flags);
		spinlock_unlock(&cache->lock);
	}

	interrupts_restore(ipl);

	return result;
}

/** Return object to cache  */
void slab_free(slab_cache_t *cache, void *obj)
{
	ipl_t ipl;

	ipl = interrupts_disable();

	if ((cache->flags & SLAB_CACHE_NOMAGAZINE) \
	    || magazine_obj_put(cache, obj)) {
		
		spinlock_lock(&cache->lock);
		slab_obj_destroy(cache, obj, NULL);
		spinlock_unlock(&cache->lock);
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
	printf("SLAB name\tOsize\tOrder\n");
	for (cur = slab_cache_list.next;cur!=&slab_cache_list; cur=cur->next) {
		cache = list_get_instance(cur, slab_cache_t, link);
		printf("%s\t%d\t%d\n", cache->name, cache->size, cache->order);
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
