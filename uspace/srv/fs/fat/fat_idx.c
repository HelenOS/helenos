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
#include <str.h>
#include <adt/hash_table.h>
#include <adt/list.h>
#include <assert.h>
#include <fibril_synch.h>
#include <malloc.h>

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
	service_id_t	service_id;

	/** Next unassigned index. */
	fs_index_t next;
	/** Number of remaining unassigned indices. */
	uint64_t remaining;

	/** Sorted list of intervals of freed indices. */
	list_t freed_list;
} unused_t;

/** Mutex protecting the list of unused structures. */
static FIBRIL_MUTEX_INITIALIZE(unused_lock);

/** List of unused structures. */
static LIST_INITIALIZE(unused_list);

static void unused_initialize(unused_t *u, service_id_t service_id)
{
	link_initialize(&u->link);
	u->service_id = service_id;
	u->next = 0;
	u->remaining = ((uint64_t)((fs_index_t)-1)) + 1;
	list_initialize(&u->freed_list);
}

static unused_t *unused_find(service_id_t service_id, bool lock)
{
	unused_t *u;

	if (lock)
		fibril_mutex_lock(&unused_lock);

	list_foreach(unused_list, l) {
		u = list_get_instance(l, unused_t, link);
		if (u->service_id == service_id) 
			return u;
	}
	
	if (lock)
		fibril_mutex_unlock(&unused_lock);
	return NULL;
}

/** Mutex protecting the up_hash and ui_hash. */
static FIBRIL_MUTEX_INITIALIZE(used_lock);

/**
 * Global hash table of all used fat_idx_t structures.
 * The index structures are hashed by the service_id, parent node's first
 * cluster and index within the parent directory.
 */ 
static hash_table_t up_hash;

#define UPH_BUCKETS_LOG	12
#define UPH_BUCKETS	(1 << UPH_BUCKETS_LOG)

#define UPH_SID_KEY	0
#define UPH_PFC_KEY	1
#define UPH_PDI_KEY	2

static hash_index_t pos_hash(unsigned long key[])
{
	service_id_t service_id = (service_id_t)key[UPH_SID_KEY];
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
	h |= (service_id & ((1 << (UPH_BUCKETS_LOG / 4)) - 1)) <<
	    (3 * (UPH_BUCKETS_LOG / 4));

	return h;
}

static int pos_compare(unsigned long key[], hash_count_t keys, link_t *item)
{
	service_id_t service_id = (service_id_t)key[UPH_SID_KEY];
	fat_cluster_t pfc;
	unsigned pdi;
	fat_idx_t *fidx = list_get_instance(item, fat_idx_t, uph_link);

	switch (keys) {
	case 1:
		return (service_id == fidx->service_id);
	case 3:
		pfc = (fat_cluster_t) key[UPH_PFC_KEY];
		pdi = (unsigned) key[UPH_PDI_KEY];
		return (service_id == fidx->service_id) && (pfc == fidx->pfc) &&
		    (pdi == fidx->pdi);
	default:
		assert((keys == 1) || (keys == 3));
	}

	return 0;
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
 * The index structures are hashed by the service_id and index.
 */
static hash_table_t ui_hash;

#define UIH_BUCKETS_LOG	12
#define UIH_BUCKETS	(1 << UIH_BUCKETS_LOG)

#define UIH_SID_KEY	0
#define UIH_INDEX_KEY	1

static hash_index_t idx_hash(unsigned long key[])
{
	service_id_t service_id = (service_id_t)key[UIH_SID_KEY];
	fs_index_t index = (fs_index_t)key[UIH_INDEX_KEY];

	hash_index_t h;

	h = service_id & ((1 << (UIH_BUCKETS_LOG / 2)) - 1);
	h |= (index & ((1 << (UIH_BUCKETS_LOG / 2)) - 1)) <<
	    (UIH_BUCKETS_LOG / 2);

	return h;
}

static int idx_compare(unsigned long key[], hash_count_t keys, link_t *item)
{
	service_id_t service_id = (service_id_t)key[UIH_SID_KEY];
	fs_index_t index;
	fat_idx_t *fidx = list_get_instance(item, fat_idx_t, uih_link);

	switch (keys) {
	case 1:
		return (service_id == fidx->service_id);
	case 2:
		index = (fs_index_t) key[UIH_INDEX_KEY];
		return (service_id == fidx->service_id) &&
		    (index == fidx->index);
	default:
		assert((keys == 1) || (keys == 2));
	}

	return 0;
}

static void idx_remove_callback(link_t *item)
{
	fat_idx_t *fidx = list_get_instance(item, fat_idx_t, uih_link);

	free(fidx);
}

static hash_table_operations_t uih_ops = {
	.hash = idx_hash,
	.compare = idx_compare,
	.remove_callback = idx_remove_callback,
};

/** Allocate a VFS index which is not currently in use. */
static bool fat_index_alloc(service_id_t service_id, fs_index_t *index)
{
	unused_t *u;
	
	assert(index);
	u = unused_find(service_id, true);
	if (!u)
		return false;	

	if (list_empty(&u->freed_list)) {
		if (u->remaining) { 
			/*
			 * There are no freed indices, allocate one directly
			 * from the counter.
			 */
			*index = u->next++;
			--u->remaining;
			fibril_mutex_unlock(&unused_lock);
			return true;
		}
	} else {
		/* There are some freed indices which we can reuse. */
		freed_t *f = list_get_instance(list_first(&u->freed_list),
		    freed_t, link);
		*index = f->first;
		if (f->first++ == f->last) {
			/* Destroy the interval. */
			list_remove(&f->link);
			free(f);
		}
		fibril_mutex_unlock(&unused_lock);
		return true;
	}
	/*
	 * We ran out of indices, which is extremely unlikely with FAT16, but
	 * theoretically still possible (e.g. too many open unlinked nodes or
	 * too many zero-sized nodes).
	 */
	fibril_mutex_unlock(&unused_lock);
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
static void fat_index_free(service_id_t service_id, fs_index_t index)
{
	unused_t *u;

	u = unused_find(service_id, true);
	assert(u);

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
		for (lnk = u->freed_list.head.next; lnk != &u->freed_list.head;
		    lnk = lnk->next) {
			freed_t *f = list_get_instance(lnk, freed_t, link);
			if (f->first == index + 1) {
				f->first--;
				if (lnk->prev != &u->freed_list.head)
					try_coalesce_intervals(lnk->prev, lnk,
					    lnk);
				fibril_mutex_unlock(&unused_lock);
				return;
			}
			if (f->last == index - 1) {
				f->last++;
				if (lnk->next != &u->freed_list.head)
					try_coalesce_intervals(lnk, lnk->next,
					    lnk);
				fibril_mutex_unlock(&unused_lock);
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
				fibril_mutex_unlock(&unused_lock);
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
		list_append(&n->link, &u->freed_list);
	}
	fibril_mutex_unlock(&unused_lock);
}

static int fat_idx_create(fat_idx_t **fidxp, service_id_t service_id)
{
	fat_idx_t *fidx;

	fidx = (fat_idx_t *) malloc(sizeof(fat_idx_t));
	if (!fidx) 
		return ENOMEM;
	if (!fat_index_alloc(service_id, &fidx->index)) {
		free(fidx);
		return ENOSPC;
	}
		
	link_initialize(&fidx->uph_link);
	link_initialize(&fidx->uih_link);
	fibril_mutex_initialize(&fidx->lock);
	fidx->service_id = service_id;
	fidx->pfc = FAT_CLST_RES0;	/* no parent yet */
	fidx->pdi = 0;
	fidx->nodep = NULL;

	*fidxp = fidx;
	return EOK;
}

int fat_idx_get_new(fat_idx_t **fidxp, service_id_t service_id)
{
	fat_idx_t *fidx;
	int rc;

	fibril_mutex_lock(&used_lock);
	rc = fat_idx_create(&fidx, service_id);
	if (rc != EOK) {
		fibril_mutex_unlock(&used_lock);
		return rc;
	}
		
	unsigned long ikey[] = {
		[UIH_SID_KEY] = service_id,
		[UIH_INDEX_KEY] = fidx->index,
	};
	
	hash_table_insert(&ui_hash, ikey, &fidx->uih_link);
	fibril_mutex_lock(&fidx->lock);
	fibril_mutex_unlock(&used_lock);

	*fidxp = fidx;
	return EOK;
}

fat_idx_t *
fat_idx_get_by_pos(service_id_t service_id, fat_cluster_t pfc, unsigned pdi)
{
	fat_idx_t *fidx;
	link_t *l;
	unsigned long pkey[] = {
		[UPH_SID_KEY] = service_id,
		[UPH_PFC_KEY] = pfc,
		[UPH_PDI_KEY] = pdi,
	};

	fibril_mutex_lock(&used_lock);
	l = hash_table_find(&up_hash, pkey);
	if (l) {
		fidx = hash_table_get_instance(l, fat_idx_t, uph_link);
	} else {
		int rc;

		rc = fat_idx_create(&fidx, service_id);
		if (rc != EOK) {
			fibril_mutex_unlock(&used_lock);
			return NULL;
		}
		
		unsigned long ikey[] = {
			[UIH_SID_KEY] = service_id,
			[UIH_INDEX_KEY] = fidx->index,
		};
	
		fidx->pfc = pfc;
		fidx->pdi = pdi;

		hash_table_insert(&up_hash, pkey, &fidx->uph_link);
		hash_table_insert(&ui_hash, ikey, &fidx->uih_link);
	}
	fibril_mutex_lock(&fidx->lock);
	fibril_mutex_unlock(&used_lock);

	return fidx;
}

void fat_idx_hashin(fat_idx_t *idx)
{
	unsigned long pkey[] = {
		[UPH_SID_KEY] = idx->service_id,
		[UPH_PFC_KEY] = idx->pfc,
		[UPH_PDI_KEY] = idx->pdi,
	};

	fibril_mutex_lock(&used_lock);
	hash_table_insert(&up_hash, pkey, &idx->uph_link);
	fibril_mutex_unlock(&used_lock);
}

void fat_idx_hashout(fat_idx_t *idx)
{
	unsigned long pkey[] = {
		[UPH_SID_KEY] = idx->service_id,
		[UPH_PFC_KEY] = idx->pfc,
		[UPH_PDI_KEY] = idx->pdi,
	};

	fibril_mutex_lock(&used_lock);
	hash_table_remove(&up_hash, pkey, 3);
	fibril_mutex_unlock(&used_lock);
}

fat_idx_t *
fat_idx_get_by_index(service_id_t service_id, fs_index_t index)
{
	fat_idx_t *fidx = NULL;
	link_t *l;
	unsigned long ikey[] = {
		[UIH_SID_KEY] = service_id,
		[UIH_INDEX_KEY] = index,
	};

	fibril_mutex_lock(&used_lock);
	l = hash_table_find(&ui_hash, ikey);
	if (l) {
		fidx = hash_table_get_instance(l, fat_idx_t, uih_link);
		fibril_mutex_lock(&fidx->lock);
	}
	fibril_mutex_unlock(&used_lock);

	return fidx;
}

/** Destroy the index structure.
 *
 * @param idx		The index structure to be destroyed.
 */
void fat_idx_destroy(fat_idx_t *idx)
{
	unsigned long ikey[] = {
		[UIH_SID_KEY] = idx->service_id,
		[UIH_INDEX_KEY] = idx->index,
	};
	service_id_t service_id = idx->service_id;
	fs_index_t index = idx->index;

	assert(idx->pfc == FAT_CLST_RES0);

	fibril_mutex_lock(&used_lock);
	/*
	 * Since we can only free unlinked nodes, the index structure is not
	 * present in the position hash (uph). We therefore hash it out from
	 * the index hash only.
	 */
	hash_table_remove(&ui_hash, ikey, 2);
	fibril_mutex_unlock(&used_lock);
	/* Release the VFS index. */
	fat_index_free(service_id, index);
	/* The index structure itself is freed in idx_remove_callback(). */
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

int fat_idx_init_by_service_id(service_id_t service_id)
{
	unused_t *u;
	int rc = EOK;

	u = (unused_t *) malloc(sizeof(unused_t));
	if (!u)
		return ENOMEM;
	unused_initialize(u, service_id);
	fibril_mutex_lock(&unused_lock);
	if (!unused_find(service_id, false)) {
		list_append(&u->link, &unused_list);
	} else {
		free(u);
		rc = EEXIST;
	}
	fibril_mutex_unlock(&unused_lock);
	return rc;
}

void fat_idx_fini_by_service_id(service_id_t service_id)
{
	unsigned long ikey[] = {
		[UIH_SID_KEY] = service_id
	};
	unsigned long pkey[] = {
		[UPH_SID_KEY] = service_id
	};

	/*
	 * Remove this instance's index structure from up_hash and ui_hash.
	 * Process up_hash first and ui_hash second because the index structure
	 * is actually removed in idx_remove_callback(). 
	 */
	fibril_mutex_lock(&used_lock);
	hash_table_remove(&up_hash, pkey, 1);
	hash_table_remove(&ui_hash, ikey, 1);
	fibril_mutex_unlock(&used_lock);

	/*
	 * Free the unused and freed structures for this instance.
	 */
	unused_t *u = unused_find(service_id, true);
	assert(u);
	list_remove(&u->link);
	fibril_mutex_unlock(&unused_lock);

	while (!list_empty(&u->freed_list)) {
		freed_t *f;
		f = list_get_instance(list_first(&u->freed_list), freed_t, link);
		list_remove(&f->link);
		free(f);
	}
	free(u); 
}

/**
 * @}
 */ 
