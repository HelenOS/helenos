/*
 * Copyright (c) 2006 Ondrej Palkovsky
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

/**
 * @file
 * @brief	Slab allocator.
 *
 * The slab allocator is closely modelled after OpenSolaris slab allocator.
 * @see http://www.usenix.org/events/usenix01/full_papers/bonwick/bonwick_html/
 *
 * with the following exceptions:
 * @li empty slabs are deallocated immediately 
 *     (in Linux they are kept in linked list, in Solaris ???)
 * @li empty magazines are deallocated when not needed
 *     (in Solaris they are held in linked list in slab cache)
 *
 * Following features are not currently supported but would be easy to do:
 * @li cache coloring
 * @li dynamic magazine growing (different magazine sizes are already
 *     supported, but we would need to adjust allocation strategy)
 *
 * The slab allocator supports per-CPU caches ('magazines') to facilitate
 * good SMP scaling. 
 *
 * When a new object is being allocated, it is first checked, if it is 
 * available in a CPU-bound magazine. If it is not found there, it is
 * allocated from a CPU-shared slab - if a partially full one is found,
 * it is used, otherwise a new one is allocated. 
 *
 * When an object is being deallocated, it is put to a CPU-bound magazine.
 * If there is no such magazine, a new one is allocated (if this fails, 
 * the object is deallocated into slab). If the magazine is full, it is
 * put into cpu-shared list of magazines and a new one is allocated.
 *
 * The CPU-bound magazine is actually a pair of magazines in order to avoid
 * thrashing when somebody is allocating/deallocating 1 item at the magazine
 * size boundary. LIFO order is enforced, which should avoid fragmentation
 * as much as possible. 
 *  
 * Every cache contains list of full slabs and list of partially full slabs.
 * Empty slabs are immediately freed (thrashing will be avoided because
 * of magazines). 
 *
 * The slab information structure is kept inside the data area, if possible.
 * The cache can be marked that it should not use magazines. This is used
 * only for slab related caches to avoid deadlocks and infinite recursion
 * (the slab allocator uses itself for allocating all it's control structures).
 *
 * The slab allocator allocates a lot of space and does not free it. When
 * the frame allocator fails to allocate a frame, it calls slab_reclaim().
 * It tries 'light reclaim' first, then brutal reclaim. The light reclaim
 * releases slabs from cpu-shared magazine-list, until at least 1 slab 
 * is deallocated in each cache (this algorithm should probably change).
 * The brutal reclaim removes all cached objects, even from CPU-bound
 * magazines.
 *
 * @todo
 * For better CPU-scaling the magazine allocation strategy should
 * be extended. Currently, if the cache does not have magazine, it asks
 * for non-cpu cached magazine cache to provide one. It might be feasible
 * to add cpu-cached magazine cache (which would allocate it's magazines
 * from non-cpu-cached mag. cache). This would provide a nice per-cpu
 * buffer. The other possibility is to use the per-cache 
 * 'empty-magazine-list', which decreases competing for 1 per-system
 * magazine cache.
 *
 * @todo
 * it might be good to add granularity of locks even to slab level,
 * we could then try_spinlock over all partial slabs and thus improve
 * scalability even on slab level
 */

#include <synch/spinlock.h>
#include <mm/slab.h>
#include <adt/list.h>
#include <memstr.h>
#include <align.h>
#include <mm/frame.h>
#include <config.h>
#include <print.h>
#include <arch.h>
#include <panic.h>
#include <debug.h>
#include <bitops.h>
#include <macros.h>

SPINLOCK_INITIALIZE(slab_cache_lock);
static LIST_INITIALIZE(slab_cache_list);

/** Magazine cache */
static slab_cache_t mag_cache;
/** Cache for cache descriptors */
static slab_cache_t slab_cache_cache;
/** Cache for external slab descriptors
 * This time we want per-cpu cache, so do not make it static
 * - using slab for internal slab structures will not deadlock,
 *   as all slab structures are 'small' - control structures of
 *   their caches do not require further allocation
 */
static slab_cache_t *slab_extern_cache;
/** Caches for malloc */
static slab_cache_t *malloc_caches[SLAB_MAX_MALLOC_W - SLAB_MIN_MALLOC_W + 1];
char *malloc_names[] =  {
	"malloc-16",
	"malloc-32",
	"malloc-64",
	"malloc-128",
	"malloc-256",
	"malloc-512",
	"malloc-1K",
	"malloc-2K",
	"malloc-4K",
	"malloc-8K",
	"malloc-16K",
	"malloc-32K",
	"malloc-64K",
	"malloc-128K",
	"malloc-256K"
};

/** Slab descriptor */
typedef struct {
	slab_cache_t *cache; 	/**< Pointer to parent cache. */
	link_t link;       	/**< List of full/partial slabs. */
	void *start;       	/**< Start address of first available item. */
	count_t available; 	/**< Count of available items in this slab. */
	index_t nextavail; 	/**< The index of next available item. */
} slab_t;

#ifdef CONFIG_DEBUG
static int _slab_initialized = 0;
#endif

/**************************************/
/* Slab allocation functions          */

/**
 * Allocate frames for slab space and initialize
 *
 */
static slab_t *slab_space_alloc(slab_cache_t *cache, int flags)
{
	void *data;
	slab_t *slab;
	size_t fsize;
	unsigned int i;
	count_t zone = 0;
	
	data = frame_alloc_generic(cache->order, FRAME_KA | flags, &zone);
	if (!data) {
		return NULL;
	}
	if (!(cache->flags & SLAB_CACHE_SLINSIDE)) {
		slab = slab_alloc(slab_extern_cache, flags);
		if (!slab) {
			frame_free(KA2PA(data));
			return NULL;
		}
	} else {
		fsize = (PAGE_SIZE << cache->order);
		slab = data + fsize - sizeof(*slab);
	}
	
	/* Fill in slab structures */
	for (i = 0; i < ((unsigned int) 1 << cache->order); i++)
		frame_set_parent(ADDR2PFN(KA2PA(data)) + i, slab, zone);

	slab->start = data;
	slab->available = cache->objects;
	slab->nextavail = 0;
	slab->cache = cache;

	for (i = 0; i < cache->objects; i++)
		*((int *) (slab->start + i*cache->size)) = i + 1;

	atomic_inc(&cache->allocated_slabs);
	return slab;
}

/**
 * Deallocate space associated with slab
 *
 * @return number of freed frames
 */
static count_t slab_space_free(slab_cache_t *cache, slab_t *slab)
{
	frame_free(KA2PA(slab->start));
	if (! (cache->flags & SLAB_CACHE_SLINSIDE))
		slab_free(slab_extern_cache, slab);

	atomic_dec(&cache->allocated_slabs);
	
	return 1 << cache->order;
}

/** Map object to slab structure */
static slab_t * obj2slab(void *obj)
{
	return (slab_t *) frame_get_parent(ADDR2PFN(KA2PA(obj)), 0);
}

/**************************************/
/* Slab functions */


/**
 * Return object to slab and call a destructor
 *
 * @param slab If the caller knows directly slab of the object, otherwise NULL
 *
 * @return Number of freed pages
 */
static count_t slab_obj_destroy(slab_cache_t *cache, void *obj, slab_t *slab)
{
	int freed = 0;

	if (!slab)
		slab = obj2slab(obj);

	ASSERT(slab->cache == cache);

	if (cache->destructor)
		freed = cache->destructor(obj);
	
	spinlock_lock(&cache->slablock);
	ASSERT(slab->available < cache->objects);

	*((int *)obj) = slab->nextavail;
	slab->nextavail = (obj - slab->start) / cache->size;
	slab->available++;

	/* Move it to correct list */
	if (slab->available == cache->objects) {
		/* Free associated memory */
		list_remove(&slab->link);
		spinlock_unlock(&cache->slablock);

		return freed + slab_space_free(cache, slab);

	} else if (slab->available == 1) {
		/* It was in full, move to partial */
		list_remove(&slab->link);
		list_prepend(&slab->link, &cache->partial_slabs);
	}
	spinlock_unlock(&cache->slablock);
	return freed;
}

/**
 * Take new object from slab or create new if needed
 *
 * @return Object address or null
 */
static void *slab_obj_create(slab_cache_t *cache, int flags)
{
	slab_t *slab;
	void *obj;

	spinlock_lock(&cache->slablock);

	if (list_empty(&cache->partial_slabs)) {
		/* Allow recursion and reclaiming
		 * - this should work, as the slab control structures
		 *   are small and do not need to allocate with anything
		 *   other than frame_alloc when they are allocating,
		 *   that's why we should get recursion at most 1-level deep
		 */
		spinlock_unlock(&cache->slablock);
		slab = slab_space_alloc(cache, flags);
		if (!slab)
			return NULL;
		spinlock_lock(&cache->slablock);
	} else {
		slab = list_get_instance(cache->partial_slabs.next, slab_t,
		    link);
		list_remove(&slab->link);
	}
	obj = slab->start + slab->nextavail * cache->size;
	slab->nextavail = *((int *)obj);
	slab->available--;

	if (!slab->available)
		list_prepend(&slab->link, &cache->full_slabs);
	else
		list_prepend(&slab->link, &cache->partial_slabs);

	spinlock_unlock(&cache->slablock);

	if (cache->constructor && cache->constructor(obj, flags)) {
		/* Bad, bad, construction failed */
		slab_obj_destroy(cache, obj, slab);
		return NULL;
	}
	return obj;
}

/**************************************/
/* CPU-Cache slab functions */

/**
 * Finds a full magazine in cache, takes it from list
 * and returns it 
 *
 * @param first If true, return first, else last mag
 */
static slab_magazine_t *get_mag_from_cache(slab_cache_t *cache, int first)
{
	slab_magazine_t *mag = NULL;
	link_t *cur;

	spinlock_lock(&cache->maglock);
	if (!list_empty(&cache->magazines)) {
		if (first)
			cur = cache->magazines.next;
		else
			cur = cache->magazines.prev;
		mag = list_get_instance(cur, slab_magazine_t, link);
		list_remove(&mag->link);
		atomic_dec(&cache->magazine_counter);
	}
	spinlock_unlock(&cache->maglock);
	return mag;
}

/** Prepend magazine to magazine list in cache */
static void put_mag_to_cache(slab_cache_t *cache, slab_magazine_t *mag)
{
	spinlock_lock(&cache->maglock);

	list_prepend(&mag->link, &cache->magazines);
	atomic_inc(&cache->magazine_counter);
	
	spinlock_unlock(&cache->maglock);
}

/**
 * Free all objects in magazine and free memory associated with magazine
 *
 * @return Number of freed pages
 */
static count_t magazine_destroy(slab_cache_t *cache, slab_magazine_t *mag)
{
	unsigned int i;
	count_t frames = 0;

	for (i = 0; i < mag->busy; i++) {
		frames += slab_obj_destroy(cache, mag->objs[i], NULL);
		atomic_dec(&cache->cached_objs);
	}
	
	slab_free(&mag_cache, mag);

	return frames;
}

/**
 * Find full magazine, set it as current and return it
 *
 * Assume cpu_magazine lock is held
 */
static slab_magazine_t *get_full_current_mag(slab_cache_t *cache)
{
	slab_magazine_t *cmag, *lastmag, *newmag;

	cmag = cache->mag_cache[CPU->id].current;
	lastmag = cache->mag_cache[CPU->id].last;
	if (cmag) { /* First try local CPU magazines */
		if (cmag->busy)
			return cmag;

		if (lastmag && lastmag->busy) {
			cache->mag_cache[CPU->id].current = lastmag;
			cache->mag_cache[CPU->id].last = cmag;
			return lastmag;
		}
	}
	/* Local magazines are empty, import one from magazine list */
	newmag = get_mag_from_cache(cache, 1);
	if (!newmag)
		return NULL;

	if (lastmag)
		magazine_destroy(cache, lastmag);

	cache->mag_cache[CPU->id].last = cmag;
	cache->mag_cache[CPU->id].current = newmag;
	return newmag;
}

/**
 * Try to find object in CPU-cache magazines
 *
 * @return Pointer to object or NULL if not available
 */
static void *magazine_obj_get(slab_cache_t *cache)
{
	slab_magazine_t *mag;
	void *obj;

	if (!CPU)
		return NULL;

	spinlock_lock(&cache->mag_cache[CPU->id].lock);

	mag = get_full_current_mag(cache);
	if (!mag) {
		spinlock_unlock(&cache->mag_cache[CPU->id].lock);
		return NULL;
	}
	obj = mag->objs[--mag->busy];
	spinlock_unlock(&cache->mag_cache[CPU->id].lock);
	atomic_dec(&cache->cached_objs);
	
	return obj;
}

/**
 * Assure that the current magazine is empty, return pointer to it, or NULL if 
 * no empty magazine is available and cannot be allocated
 *
 * Assume mag_cache[CPU->id].lock is held
 *
 * We have 2 magazines bound to processor. 
 * First try the current. 
 *  If full, try the last.
 *   If full, put to magazines list.
 *   allocate new, exchange last & current
 *
 */
static slab_magazine_t *make_empty_current_mag(slab_cache_t *cache)
{
	slab_magazine_t *cmag,*lastmag,*newmag;

	cmag = cache->mag_cache[CPU->id].current;
	lastmag = cache->mag_cache[CPU->id].last;

	if (cmag) {
		if (cmag->busy < cmag->size)
			return cmag;
		if (lastmag && lastmag->busy < lastmag->size) {
			cache->mag_cache[CPU->id].last = cmag;
			cache->mag_cache[CPU->id].current = lastmag;
			return lastmag;
		}
	}
	/* current | last are full | nonexistent, allocate new */
	/* We do not want to sleep just because of caching */
	/* Especially we do not want reclaiming to start, as 
	 * this would deadlock */
	newmag = slab_alloc(&mag_cache, FRAME_ATOMIC | FRAME_NO_RECLAIM);
	if (!newmag)
		return NULL;
	newmag->size = SLAB_MAG_SIZE;
	newmag->busy = 0;

	/* Flush last to magazine list */
	if (lastmag)
		put_mag_to_cache(cache, lastmag);

	/* Move current as last, save new as current */
	cache->mag_cache[CPU->id].last = cmag;	
	cache->mag_cache[CPU->id].current = newmag;	

	return newmag;
}

/**
 * Put object into CPU-cache magazine
 *
 * @return 0 - success, -1 - could not get memory
 */
static int magazine_obj_put(slab_cache_t *cache, void *obj)
{
	slab_magazine_t *mag;

	if (!CPU)
		return -1;

	spinlock_lock(&cache->mag_cache[CPU->id].lock);

	mag = make_empty_current_mag(cache);
	if (!mag) {
		spinlock_unlock(&cache->mag_cache[CPU->id].lock);
		return -1;
	}
	
	mag->objs[mag->busy++] = obj;

	spinlock_unlock(&cache->mag_cache[CPU->id].lock);
	atomic_inc(&cache->cached_objs);
	return 0;
}


/**************************************/
/* Slab cache functions */

/** Return number of objects that fit in certain cache size */
static unsigned int comp_objects(slab_cache_t *cache)
{
	if (cache->flags & SLAB_CACHE_SLINSIDE)
		return ((PAGE_SIZE << cache->order) - sizeof(slab_t)) /
		    cache->size;
	else 
		return (PAGE_SIZE << cache->order) / cache->size;
}

/** Return wasted space in slab */
static unsigned int badness(slab_cache_t *cache)
{
	unsigned int objects;
	unsigned int ssize;

	objects = comp_objects(cache);
	ssize = PAGE_SIZE << cache->order;
	if (cache->flags & SLAB_CACHE_SLINSIDE)
		ssize -= sizeof(slab_t);
	return ssize - objects * cache->size;
}

/**
 * Initialize mag_cache structure in slab cache
 */
static void make_magcache(slab_cache_t *cache)
{
	unsigned int i;
	
	ASSERT(_slab_initialized >= 2);

	cache->mag_cache = malloc(sizeof(slab_mag_cache_t) * config.cpu_count,
	    0);
	for (i = 0; i < config.cpu_count; i++) {
		memsetb(&cache->mag_cache[i], sizeof(cache->mag_cache[i]), 0);
		spinlock_initialize(&cache->mag_cache[i].lock,
		    "slab_maglock_cpu");
	}
}

/** Initialize allocated memory as a slab cache */
static void
_slab_cache_create(slab_cache_t *cache, char *name, size_t size, size_t align,
    int (*constructor)(void *obj, int kmflag), int (*destructor)(void *obj),
    int flags)
{
	int pages;
	ipl_t ipl;

	memsetb(cache, sizeof(*cache), 0);
	cache->name = name;

	if (align < sizeof(unative_t))
		align = sizeof(unative_t);
	size = ALIGN_UP(size, align);
		
	cache->size = size;

	cache->constructor = constructor;
	cache->destructor = destructor;
	cache->flags = flags;

	list_initialize(&cache->full_slabs);
	list_initialize(&cache->partial_slabs);
	list_initialize(&cache->magazines);
	spinlock_initialize(&cache->slablock, "slab_lock");
	spinlock_initialize(&cache->maglock, "slab_maglock");
	if (!(cache->flags & SLAB_CACHE_NOMAGAZINE))
		make_magcache(cache);

	/* Compute slab sizes, object counts in slabs etc. */
	if (cache->size < SLAB_INSIDE_SIZE)
		cache->flags |= SLAB_CACHE_SLINSIDE;

	/* Minimum slab order */
	pages = SIZE2FRAMES(cache->size);
	/* We need the 2^order >= pages */
	if (pages == 1)
		cache->order = 0;
	else
		cache->order = fnzb(pages - 1) + 1;

	while (badness(cache) > SLAB_MAX_BADNESS(cache)) {
		cache->order += 1;
	}
	cache->objects = comp_objects(cache);
	/* If info fits in, put it inside */
	if (badness(cache) > sizeof(slab_t))
		cache->flags |= SLAB_CACHE_SLINSIDE;

	/* Add cache to cache list */
	ipl = interrupts_disable();
	spinlock_lock(&slab_cache_lock);

	list_append(&cache->link, &slab_cache_list);

	spinlock_unlock(&slab_cache_lock);
	interrupts_restore(ipl);
}

/** Create slab cache  */
slab_cache_t *
slab_cache_create(char *name, size_t size, size_t align,
    int (*constructor)(void *obj, int kmflag), int (*destructor)(void *obj),
    int flags)
{
	slab_cache_t *cache;

	cache = slab_alloc(&slab_cache_cache, 0);
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
	unsigned int i;
	slab_magazine_t *mag;
	count_t frames = 0;
	int magcount;
	
	if (cache->flags & SLAB_CACHE_NOMAGAZINE)
		return 0; /* Nothing to do */

	/* We count up to original magazine count to avoid
	 * endless loop 
	 */
	magcount = atomic_get(&cache->magazine_counter);
	while (magcount-- && (mag=get_mag_from_cache(cache, 0))) {
		frames += magazine_destroy(cache,mag);
		if (!(flags & SLAB_RECLAIM_ALL) && frames)
			break;
	}
	
	if (flags & SLAB_RECLAIM_ALL) {
		/* Free cpu-bound magazines */
		/* Destroy CPU magazines */
		for (i = 0; i < config.cpu_count; i++) {
			spinlock_lock(&cache->mag_cache[i].lock);

			mag = cache->mag_cache[i].current;
			if (mag)
				frames += magazine_destroy(cache, mag);
			cache->mag_cache[i].current = NULL;
			
			mag = cache->mag_cache[i].last;
			if (mag)
				frames += magazine_destroy(cache, mag);
			cache->mag_cache[i].last = NULL;

			spinlock_unlock(&cache->mag_cache[i].lock);
		}
	}

	return frames;
}

/** Check that there are no slabs and remove cache from system  */
void slab_cache_destroy(slab_cache_t *cache)
{
	ipl_t ipl;

	/* First remove cache from link, so that we don't need
	 * to disable interrupts later
	 */

	ipl = interrupts_disable();
	spinlock_lock(&slab_cache_lock);

	list_remove(&cache->link);

	spinlock_unlock(&slab_cache_lock);
	interrupts_restore(ipl);

	/* Do not lock anything, we assume the software is correct and
	 * does not touch the cache when it decides to destroy it */
	
	/* Destroy all magazines */
	_slab_reclaim(cache, SLAB_RECLAIM_ALL);

	/* All slabs must be empty */
	if (!list_empty(&cache->full_slabs) ||
	    !list_empty(&cache->partial_slabs))
		panic("Destroying cache that is not empty.");

	if (!(cache->flags & SLAB_CACHE_NOMAGAZINE))
		free(cache->mag_cache);
	slab_free(&slab_cache_cache, cache);
}

/** Allocate new object from cache - if no flags given, always returns memory */
void *slab_alloc(slab_cache_t *cache, int flags)
{
	ipl_t ipl;
	void *result = NULL;
	
	/* Disable interrupts to avoid deadlocks with interrupt handlers */
	ipl = interrupts_disable();

	if (!(cache->flags & SLAB_CACHE_NOMAGAZINE)) {
		result = magazine_obj_get(cache);
	}
	if (!result)
		result = slab_obj_create(cache, flags);

	interrupts_restore(ipl);

	if (result)
		atomic_inc(&cache->allocated_objs);

	return result;
}

/** Return object to cache, use slab if known  */
static void _slab_free(slab_cache_t *cache, void *obj, slab_t *slab)
{
	ipl_t ipl;

	ipl = interrupts_disable();

	if ((cache->flags & SLAB_CACHE_NOMAGAZINE) ||
	    magazine_obj_put(cache, obj)) {
		slab_obj_destroy(cache, obj, slab);

	}
	interrupts_restore(ipl);
	atomic_dec(&cache->allocated_objs);
}

/** Return slab object to cache */
void slab_free(slab_cache_t *cache, void *obj)
{
	_slab_free(cache, obj, NULL);
}

/* Go through all caches and reclaim what is possible */
count_t slab_reclaim(int flags)
{
	slab_cache_t *cache;
	link_t *cur;
	count_t frames = 0;

	spinlock_lock(&slab_cache_lock);

	/* TODO: Add assert, that interrupts are disabled, otherwise
	 * memory allocation from interrupts can deadlock.
	 */

	for (cur = slab_cache_list.next; cur != &slab_cache_list;
	    cur = cur->next) {
		cache = list_get_instance(cur, slab_cache_t, link);
		frames += _slab_reclaim(cache, flags);
	}

	spinlock_unlock(&slab_cache_lock);

	return frames;
}


/* Print list of slabs */
void slab_print_list(void)
{
	int skip = 0;

	printf("slab name        size     pages  obj/pg slabs  cached allocated"
	    " ctl\n");
	printf("---------------- -------- ------ ------ ------ ------ ---------"
	    " ---\n");

	while (true) {
		slab_cache_t *cache;
		link_t *cur;
		ipl_t ipl;
		int i;

		/*
		 * We must not hold the slab_cache_lock spinlock when printing
		 * the statistics. Otherwise we can easily deadlock if the print
		 * needs to allocate memory.
		 *
		 * Therefore, we walk through the slab cache list, skipping some
		 * amount of already processed caches during each iteration and
		 * gathering statistics about the first unprocessed cache. For
		 * the sake of printing the statistics, we realese the
		 * slab_cache_lock and reacquire it afterwards. Then the walk
		 * starts again.
		 *
		 * This limits both the efficiency and also accuracy of the
		 * obtained statistics. The efficiency is decreased because the
		 * time complexity of the algorithm is quadratic instead of
		 * linear. The accuracy is impacted because we drop the lock
		 * after processing one cache. If there is someone else
		 * manipulating the cache list, we might omit an arbitrary
		 * number of caches or process one cache multiple times.
		 * However, we don't bleed for this algorithm for it is only
		 * statistics.
		 */

		ipl = interrupts_disable();
		spinlock_lock(&slab_cache_lock);

		for (i = 0, cur = slab_cache_list.next;
		    i < skip && cur != &slab_cache_list;
		    i++, cur = cur->next)
			;

		if (cur == &slab_cache_list) {
			spinlock_unlock(&slab_cache_lock);
			interrupts_restore(ipl);
			break;
		}

		skip++;

		cache = list_get_instance(cur, slab_cache_t, link);

		char *name = cache->name;
		uint8_t order = cache->order;
		size_t size = cache->size;
		unsigned int objects = cache->objects;
		long allocated_slabs = atomic_get(&cache->allocated_slabs);
		long cached_objs = atomic_get(&cache->cached_objs);
		long allocated_objs = atomic_get(&cache->allocated_objs);
		int flags = cache->flags;
		
		spinlock_unlock(&slab_cache_lock);
		interrupts_restore(ipl);
		
		printf("%-16s %8" PRIs " %6d %6u %6ld %6ld %9ld %-3s\n",
		    name, size, (1 << order), objects, allocated_slabs,
		    cached_objs, allocated_objs,
		    flags & SLAB_CACHE_SLINSIDE ? "in" : "out");
	}
}

void slab_cache_init(void)
{
	int i, size;

	/* Initialize magazine cache */
	_slab_cache_create(&mag_cache, "slab_magazine",
	    sizeof(slab_magazine_t) + SLAB_MAG_SIZE * sizeof(void*),
	    sizeof(uintptr_t), NULL, NULL, SLAB_CACHE_NOMAGAZINE |
	    SLAB_CACHE_SLINSIDE);
	/* Initialize slab_cache cache */
	_slab_cache_create(&slab_cache_cache, "slab_cache",
	    sizeof(slab_cache_cache), sizeof(uintptr_t), NULL, NULL,
	    SLAB_CACHE_NOMAGAZINE | SLAB_CACHE_SLINSIDE);
	/* Initialize external slab cache */
	slab_extern_cache = slab_cache_create("slab_extern", sizeof(slab_t), 0,
	    NULL, NULL, SLAB_CACHE_SLINSIDE | SLAB_CACHE_MAGDEFERRED);

	/* Initialize structures for malloc */
	for (i = 0, size = (1 << SLAB_MIN_MALLOC_W);
	    i < (SLAB_MAX_MALLOC_W - SLAB_MIN_MALLOC_W + 1);
	    i++, size <<= 1) {
		malloc_caches[i] = slab_cache_create(malloc_names[i], size, 0,
		    NULL, NULL, SLAB_CACHE_MAGDEFERRED);
	}
#ifdef CONFIG_DEBUG       
	_slab_initialized = 1;
#endif
}

/** Enable cpu_cache
 *
 * Kernel calls this function, when it knows the real number of
 * processors. 
 * Allocate slab for cpucache and enable it on all existing
 * slabs that are SLAB_CACHE_MAGDEFERRED
 */
void slab_enable_cpucache(void)
{
	link_t *cur;
	slab_cache_t *s;

#ifdef CONFIG_DEBUG
	_slab_initialized = 2;
#endif

	spinlock_lock(&slab_cache_lock);
	
	for (cur = slab_cache_list.next; cur != &slab_cache_list;
	    cur = cur->next){
		s = list_get_instance(cur, slab_cache_t, link);
		if ((s->flags & SLAB_CACHE_MAGDEFERRED) !=
		    SLAB_CACHE_MAGDEFERRED)
			continue;
		make_magcache(s);
		s->flags &= ~SLAB_CACHE_MAGDEFERRED;
	}

	spinlock_unlock(&slab_cache_lock);
}

/**************************************/
/* kalloc/kfree functions             */
void *malloc(unsigned int size, int flags)
{
	ASSERT(_slab_initialized);
	ASSERT(size && size <= (1 << SLAB_MAX_MALLOC_W));
	
	if (size < (1 << SLAB_MIN_MALLOC_W))
		size = (1 << SLAB_MIN_MALLOC_W);

	int idx = fnzb(size - 1) - SLAB_MIN_MALLOC_W + 1;

	return slab_alloc(malloc_caches[idx], flags);
}

void *realloc(void *ptr, unsigned int size, int flags)
{
	ASSERT(_slab_initialized);
	ASSERT(size <= (1 << SLAB_MAX_MALLOC_W));
	
	void *new_ptr;
	
	if (size > 0) {
		if (size < (1 << SLAB_MIN_MALLOC_W))
			size = (1 << SLAB_MIN_MALLOC_W);
		int idx = fnzb(size - 1) - SLAB_MIN_MALLOC_W + 1;
		
		new_ptr = slab_alloc(malloc_caches[idx], flags);
	} else
		new_ptr = NULL;
	
	if ((new_ptr != NULL) && (ptr != NULL)) {
		slab_t *slab = obj2slab(ptr);
		memcpy(new_ptr, ptr, min(size, slab->cache->size));
	}
	
	if (ptr != NULL)
		free(ptr);
	
	return new_ptr;
}

void free(void *ptr)
{
	if (!ptr)
		return;

	slab_t *slab = obj2slab(ptr);
	_slab_free(slab->cache, ptr, slab);
}

/** @}
 */
