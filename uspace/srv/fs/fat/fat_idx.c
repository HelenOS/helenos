/*
 * Copyright (c) 2008 Jakub Jermar
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

/** @addtogroup fs
 * @{
 */ 

/**
 * @file	fat_idx.c
 * @brief	Layer for translating FAT entities to VFS node indices.
 */

#include "fat.h"
#include "../../vfs/vfs.h"
#include <errno.h>
#include <string.h>
#include <libadt/hash_table.h>
#include <libadt/list.h>
#include <assert.h>
#include <futex.h>

/** Each instance of this type describes one interval of freed VFS indices. */
typedef struct {
	link_t		link;
	fs_index_t	first;
	fs_index_t	last;
} freed_t;

/**
 * Each instance of this type describes state of all VFS indices that
 * are currently unused.
 */
typedef struct {
	link_t		link;
	dev_handle_t	dev_handle;

	/** Next unassigned index. */
	fs_index_t	next;
	/** Number of remaining unassigned indices. */
	uint64_t	remaining;

	/** Sorted list of intervals of freed indices. */
	link_t		freed_head;
} unused_t;

/** Futex protecting the list of unused structures. */
static futex_t unused_futex = FUTEX_INITIALIZER;

/** List of unused structures. */
static LIST_INITIALIZE(unused_head);

static void unused_initialize(unused_t *u, dev_handle_t dev_handle)
{
	link_initialize(&u->link);
	u->dev_handle = dev_handle;
	u->next = 0;
	u->remaining = ((uint64_t)((fs_index_t)-1)) + 1;
	list_initialize(&u->freed_head);
}

/** Futex protecting the up_hash and ui_hash. */
static futex_t used_futex = FUTEX_INITIALIZER; 

/**
 * Global hash table of all used fat_idx_t structures.
 * The index structures are hashed by the dev_handle, parent node's first
 * cluster and index within the parent directory.
 */ 
static hash_table_t up_hash;

#define UPH_BUCKETS_LOG	12
#define UPH_BUCKETS	(1 << UPH_BUCKETS_LOG)

#define UPH_DH_KEY	0
#define UPH_PFC_KEY	1
#define UPH_PDI_KEY	2

static hash_index_t pos_hash(unsigned long key[])
{
	dev_handle_t dev_handle = (dev_handle_t)key[UPH_DH_KEY];
	fat_cluster_t pfc = (fat_cluster_t)key[UPH_PFC_KEY];
	unsigned pdi = (unsigned)key[UPH_PDI_KEY];

	hash_index_t h;

	/*
	 * The least significant half of all bits are the least significant bits
	 * of the parent node's first cluster.
	 *
	 * The least significant half of the most significant half of all bits
	 * are the least significant bits of the node's dentry index within the
	 * parent directory node.
	 *
	 * The most significant half of the most significant half of all bits
	 * are the least significant bits of the device handle.
	 */
	h = pfc & ((1 << (UPH_BUCKETS_LOG / 2)) - 1);
	h |= (pdi & ((1 << (UPH_BUCKETS_LOG / 4)) - 1)) <<
	    (UPH_BUCKETS_LOG / 2); 
	h |= (dev_handle & ((1 << (UPH_BUCKETS_LOG / 4)) - 1)) <<
	    (3 * (UPH_BUCKETS_LOG / 4));

	return h;
}

static int pos_compare(unsigned long key[], hash_count_t keys, link_t *item)
{
	dev_handle_t dev_handle = (dev_handle_t)key[UPH_DH_KEY];
	fat_cluster_t pfc = (fat_cluster_t)key[UPH_PFC_KEY];
	unsigned pdi = (unsigned)key[UPH_PDI_KEY];
	fat_idx_t *fidx = list_get_instance(item, fat_idx_t, uph_link);

	return (dev_handle == fidx->dev_handle) && (pfc == fidx->pfc) &&
	    (pdi == fidx->pdi);
}

static void pos_remove_callback(link_t *item)
{
	/* nothing to do */
}

static hash_table_operations_t uph_ops = {
	.hash = pos_hash,
	.compare = pos_compare,
	.remove_callback = pos_remove_callback,
};

/**
 * Global hash table of all used fat_idx_t structures.
 * The index structures are hashed by the dev_handle and index.
 */
static hash_table_t ui_hash;

#define UIH_BUCKETS_LOG	12
#define UIH_BUCKETS	(1 << UIH_BUCKETS_LOG)

#define UIH_DH_KEY	0
#define UIH_INDEX_KEY	1

static hash_index_t idx_hash(unsigned long key[])
{
	dev_handle_t dev_handle = (dev_handle_t)key[UIH_DH_KEY];
	fs_index_t index = (fs_index_t)key[UIH_INDEX_KEY];

	hash_index_t h;

	h = dev_handle & ((1 << (UIH_BUCKETS_LOG / 2)) - 1);
	h |= (index & ((1 << (UIH_BUCKETS_LOG / 2)) - 1)) <<
	    (UIH_BUCKETS_LOG / 2);

	return h;
}

static int idx_compare(unsigned long key[], hash_count_t keys, link_t *item)
{
	dev_handle_t dev_handle = (dev_handle_t)key[UIH_DH_KEY];
	fs_index_t index = (fs_index_t)key[UIH_INDEX_KEY];
	fat_idx_t *fidx = list_get_instance(item, fat_idx_t, uih_link);

	return (dev_handle == fidx->dev_handle) && (index == fidx->index);
}

static void idx_remove_callback(link_t *item)
{
	/* nothing to do */
}

static hash_table_operations_t uih_ops = {
	.hash = idx_hash,
	.compare = idx_compare,
	.remove_callback = idx_remove_callback,
};

/** Allocate a VFS index which is not currently in use. */
static bool fat_idx_alloc(dev_handle_t dev_handle, fs_index_t *index)
{
	link_t *l;
	unused_t *u;
	
	assert(index);
	futex_down(&unused_futex);
	for (l = unused_head.next; l != &unused_head; l = l->next) {
		u = list_get_instance(l, unused_t, link);
		if (u->dev_handle == dev_handle) 
			goto hit;
	}
	futex_up(&unused_futex);
	
	/* dev_handle not found */
	return false;	

hit:
	if (list_empty(&u->freed_head)) {
		if (u->remaining) { 
			/*
			 * There are no freed indices, allocate one directly
			 * from the counter.
			 */
			*index = u->next++;
			--u->remaining;
			futex_up(&unused_futex);
			return true;
		}
	} else {
		/* There are some freed indices which we can reuse. */
		freed_t *f = list_get_instance(u->freed_head.next, freed_t,
		    link);
		*index = f->first;
		if (f->first++ == f->last) {
			/* Destroy the interval. */
			list_remove(&f->link);
			free(f);
		}
		futex_up(&unused_futex);
		return true;
	}
	/*
	 * We ran out of indices, which is extremely unlikely with FAT16, but
	 * theoretically still possible (e.g. too many open unlinked nodes or
	 * too many zero-sized nodes).
	 */
	futex_up(&unused_futex);
	return false;
}

/** If possible, coalesce two intervals of freed indices. */
static void try_coalesce_intervals(link_t *l, link_t *r, link_t *cur)
{
	freed_t *fl = list_get_instance(l, freed_t, link);
	freed_t *fr = list_get_instance(r, freed_t, link);

	if (fl->last + 1 == fr->first) {
		if (cur == l) {
			fl->last = fr->last;
			list_remove(r);
			free(r);
		} else {
			fr->first = fl->first;
			list_remove(l);
			free(l);
		}
	}
}

/** Free a VFS index, which is no longer in use. */
static void fat_idx_free(dev_handle_t dev_handle, fs_index_t index)
{
	link_t *l;
	unused_t *u;

	futex_down(&unused_futex);
	for (l = unused_head.next; l != &unused_head; l = l->next) {
		u = list_get_instance(l, unused_t, link);
		if (u->dev_handle == dev_handle)
			goto hit;
	}
	futex_up(&unused_futex);

	/* should not happen */
	assert(0);

hit:
	if (u->next == index + 1) {
		/* The index can be returned directly to the counter. */
		u->next--;
		u->remaining++;
	} else {
		/*
		 * The index must be returned either to an existing freed
		 * interval or a new interval must be created.
		 */
		link_t *lnk;
		freed_t *n;
		for (lnk = u->freed_head.next; lnk != &u->freed_head;
		    lnk = lnk->next) {
			freed_t *f = list_get_instance(lnk, freed_t, link);
			if (f->first == index + 1) {
				f->first--;
				if (lnk->prev != &u->freed_head)
					try_coalesce_intervals(lnk->prev, lnk,
					    lnk);
				futex_up(&unused_futex);
				return;
			}
			if (f->last == index - 1) {
				f->last++;
				if (lnk->next != &u->freed_head)
					try_coalesce_intervals(lnk, lnk->next,
					    lnk);
				futex_up(&unused_futex);
				return;
			}
			if (index > f->first) {
				n = malloc(sizeof(freed_t));
				/* TODO: sleep until allocation succeeds */
				assert(n);
				link_initialize(&n->link);
				n->first = index;
				n->last = index;
				list_insert_before(&n->link, lnk);
				futex_up(&unused_futex);
				return;
			}

		}
		/* The index will form the last interval. */
		n = malloc(sizeof(freed_t));
		/* TODO: sleep until allocation succeeds */
		assert(n);
		link_initialize(&n->link);
		n->first = index;
		n->last = index;
		list_append(&n->link, &u->freed_head);
	}
	futex_up(&unused_futex);
}

fat_idx_t *
fat_idx_get_by_pos(dev_handle_t dev_handle, fat_cluster_t pfc, unsigned pdi)
{
	fat_idx_t *fidx;
	link_t *l;
	unsigned long pkey[] = {
		[UPH_DH_KEY] = dev_handle,
		[UPH_PFC_KEY] = pfc,
		[UPH_PDI_KEY] = pdi,
	};

	futex_down(&used_futex);
	l = hash_table_find(&up_hash, pkey);
	if (l) {
		fidx = hash_table_get_instance(l, fat_idx_t, uph_link);
	} else {
		fidx = (fat_idx_t *) malloc(sizeof(fat_idx_t));
		if (!fidx) {
			futex_up(&used_futex);
			return NULL;
		}
		if (!fat_idx_alloc(dev_handle, &fidx->index)) {
			free(fidx);
			futex_up(&used_futex);
			return NULL;
		}
		
		unsigned long ikey[] = {
			[UIH_DH_KEY] = dev_handle,
			[UIH_INDEX_KEY] = fidx->index,
		};
	
		link_initialize(&fidx->uph_link);
		link_initialize(&fidx->uih_link);
		futex_initialize(&fidx->lock, 1);
		fidx->dev_handle = dev_handle;
		fidx->pfc = pfc;
		fidx->pdi = pdi;
		fidx->nodep = NULL;

		hash_table_insert(&up_hash, pkey, &fidx->uph_link);
		hash_table_insert(&ui_hash, ikey, &fidx->uih_link);
	}
	futex_down(&fidx->lock);
	futex_up(&used_futex);

	return fidx;
}

fat_idx_t *
fat_idx_get_by_index(dev_handle_t dev_handle, fs_index_t index)
{
	fat_idx_t *fidx = NULL;
	link_t *l;
	unsigned long ikey[] = {
		[UIH_DH_KEY] = dev_handle,
		[UIH_INDEX_KEY] = index,
	};

	futex_down(&used_futex);
	l = hash_table_find(&ui_hash, ikey);
	if (l) {
		fidx = hash_table_get_instance(l, fat_idx_t, uih_link);
		futex_down(&fidx->lock);
	}
	futex_up(&used_futex);

	return fidx;
}

int fat_idx_init(void)
{
	if (!hash_table_create(&up_hash, UPH_BUCKETS, 3, &uph_ops)) 
		return ENOMEM;
	if (!hash_table_create(&ui_hash, UIH_BUCKETS, 2, &uih_ops)) {
		hash_table_destroy(&up_hash);
		return ENOMEM;
	}
	return EOK;
}

void fat_idx_fini(void)
{
	/* We assume the hash tables are empty. */
	hash_table_destroy(&up_hash);
	hash_table_destroy(&ui_hash);
}

int fat_idx_init_by_dev_handle(dev_handle_t dev_handle)
{
	unused_t *u = (unused_t *) malloc(sizeof(unused_t));
	if (!u)
		return ENOMEM;
	unused_initialize(u, dev_handle);
	futex_down(&unused_futex);
	list_append(&u->link, &unused_head);
	futex_up(&unused_futex);
	return EOK;
}

void fat_idx_fini_by_dev_handle(dev_handle_t dev_handle)
{
	unused_t *u;
	link_t *l;

	futex_down(&unused_futex);
	for (l = unused_head.next; l != &unused_head; l = l->next) {
		u = list_get_instance(l, unused_t, link);
		if (u->dev_handle == dev_handle) 
			goto hit;
	}
	futex_up(&unused_futex);

	assert(false);	/* should not happen */

hit:
	list_remove(&u->link);
	futex_up(&unused_futex);

	while (!list_empty(&u->freed_head)) {
		freed_t *f;
		f = list_get_instance(u->freed_head.next, freed_t, link);
		list_remove(&f->link);
		free(f);
	}
	free(u); 
}

/**
 * @}
 */ 
