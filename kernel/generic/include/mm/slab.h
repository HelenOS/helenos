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
/** @file
 */

#ifndef KERN_SLAB_H_
#define KERN_SLAB_H_

#include <adt/list.h>
#include <synch/spinlock.h>
#include <atomic.h>
#include <mm/frame.h>

/** Minimum size to be allocated by malloc */
#define SLAB_MIN_MALLOC_W 4

/** Maximum size to be allocated by malloc */
#define SLAB_MAX_MALLOC_W 18

/** Initial Magazine size (TODO: dynamically growing magazines) */
#define SLAB_MAG_SIZE  4

/** If object size is less, store control structure inside SLAB */
#define SLAB_INSIDE_SIZE   (PAGE_SIZE >> 3)

/** Maximum wasted space we allow for cache */
#define SLAB_MAX_BADNESS(cache)	\
	(((unsigned int) PAGE_SIZE << (cache)->order) >> 2)

/* slab_reclaim constants */

/** Reclaim all possible memory, because we are in memory stress */
#define SLAB_RECLAIM_ALL  0x1 

/* cache_create flags */

/** Do not use per-cpu cache */
#define SLAB_CACHE_NOMAGAZINE 0x1
/** Have control structure inside SLAB */
#define SLAB_CACHE_SLINSIDE   0x2
/** We add magazine cache later, if we have this flag */
#define SLAB_CACHE_MAGDEFERRED (0x4 | SLAB_CACHE_NOMAGAZINE)

typedef struct {
	link_t link;
	count_t busy;	/**< Count of full slots in magazine */
	count_t size;	/**< Number of slots in magazine */
	void *objs[];	/**< Slots in magazine */
} slab_magazine_t;

typedef struct {
	slab_magazine_t *current;
	slab_magazine_t *last;
	SPINLOCK_DECLARE(lock);
} slab_mag_cache_t;


typedef struct {
	char *name;

	link_t link;

	/* Configuration */
	/** Size of slab position - align_up(sizeof(obj)) */
	size_t size;

	int (*constructor)(void *obj, int kmflag);
	int (*destructor)(void *obj);

	/** Flags changing behaviour of cache */
	int flags;

	/* Computed values */
	uint8_t order;			/**< Order of frames to be allocated */
	unsigned int objects;		/**< Number of objects that fit in */

	/* Statistics */
	atomic_t allocated_slabs;
	atomic_t allocated_objs;
	atomic_t cached_objs;
	/** How many magazines in magazines list */
	atomic_t magazine_counter; 

	/* Slabs */
	link_t full_slabs;	/**< List of full slabs */
	link_t partial_slabs;	/**< List of partial slabs */
	SPINLOCK_DECLARE(slablock);
	/* Magazines  */
	link_t magazines;	/**< List o full magazines */
	SPINLOCK_DECLARE(maglock);

	/** CPU cache */
	slab_mag_cache_t *mag_cache;
} slab_cache_t;

extern slab_cache_t *slab_cache_create(char *, size_t, size_t,
    int (*)(void *, int), int (*)(void *), int);
extern void slab_cache_destroy(slab_cache_t *);

extern void * slab_alloc(slab_cache_t *, int);
extern void slab_free(slab_cache_t *, void *);
extern count_t slab_reclaim(int);

/* slab subsytem initialization */
extern void slab_cache_init(void);
extern void slab_enable_cpucache(void);

/* kconsole debug */
extern void slab_print_list(void);

/* malloc support */
extern void *malloc(unsigned int, int);
extern void *realloc(void *, unsigned int, int);
extern void free(void *);
#endif

/** @}
 */
