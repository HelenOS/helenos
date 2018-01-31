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
#include <adt/hash.h>
#include <adt/list.h>
#include <assert.h>
#include <fibril_synch.h>
#include <stdlib.h>

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
	link_t link;
	service_id_t service_id;

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
	if (lock)
		fibril_mutex_lock(&unused_lock);

	list_foreach(unused_list, link, unused_t, u) {
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

typedef struct {
	service_id_t service_id;
	fat_cluster_t pfc;
	unsigned pdi;
} pos_key_t;

static inline size_t pos_key_hash(void *key)
{
	pos_key_t *pos = (pos_key_t*)key;
	
	size_t hash = 0;
	hash = hash_combine(pos->pfc, pos->pdi);
	return hash_combine(hash, pos->service_id);
}

static size_t pos_hash(const ht_link_t *item)
{
	fat_idx_t *fidx = hash_table_get_inst(item, fat_idx_t, uph_link);
	
	pos_key_t pkey = {
		.service_id = fidx->service_id,
		.pfc = fidx->pfc,
		.pdi = fidx->pdi,
	};
	
	return pos_key_hash(&pkey);
}

static bool pos_key_equal(void *key, const ht_link_t *item)
{
	pos_key_t *pos = (pos_key_t*)key;
	fat_idx_t *fidx = hash_table_get_inst(item, fat_idx_t, uph_link);
	
	return pos->service_id == fidx->service_id
		&& pos->pdi == fidx->pdi
		&& pos->pfc == fidx->pfc;
}

static hash_table_ops_t uph_ops = {
	.hash = pos_hash,
	.key_hash = pos_key_hash,
	.key_equal = pos_key_equal,
	.equal = NULL,
	.remove_callback = NULL,
};

/**
 * Global hash table of all used fat_idx_t structures.
 * The index structures are hashed by the service_id and index.
 */
static hash_table_t ui_hash;

typedef struct {
	service_id_t service_id;
	fs_index_t index;
} idx_key_t;

static size_t idx_key_hash(void *key_arg)
{
	idx_key_t *key = (idx_key_t*)key_arg;
	return hash_combine(key->service_id, key->index);
}

static size_t idx_hash(const ht_link_t *item)
{
	fat_idx_t *fidx = hash_table_get_inst(item, fat_idx_t, uih_link);
	return hash_combine(fidx->service_id, fidx->index);
}

static bool idx_key_equal(void *key_arg, const ht_link_t *item)
{
	fat_idx_t *fidx = hash_table_get_inst(item, fat_idx_t, uih_link);
	idx_key_t *key = (idx_key_t*)key_arg;
	
	return key->index == fidx->index && key->service_id == fidx->service_id;
}

static void idx_remove_callback(ht_link_t *item)
{
	fat_idx_t *fidx = hash_table_get_inst(item, fat_idx_t, uih_link);

	free(fidx);
}

static hash_table_ops_t uih_ops = {
	.hash = idx_hash,
	.key_hash = idx_key_hash,
	.key_equal = idx_key_equal,
	.equal = NULL,
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

static errno_t fat_idx_create(fat_idx_t **fidxp, service_id_t service_id)
{
	fat_idx_t *fidx;

	fidx = (fat_idx_t *) malloc(sizeof(fat_idx_t));
	if (!fidx) 
		return ENOMEM;
	if (!fat_index_alloc(service_id, &fidx->index)) {
		free(fidx);
		return ENOSPC;
	}
		
	fibril_mutex_initialize(&fidx->lock);
	fidx->service_id = service_id;
	fidx->pfc = FAT_CLST_RES0;	/* no parent yet */
	fidx->pdi = 0;
	fidx->nodep = NULL;

	*fidxp = fidx;
	return EOK;
}

errno_t fat_idx_get_new(fat_idx_t **fidxp, service_id_t service_id)
{
	fat_idx_t *fidx;
	errno_t rc;

	fibril_mutex_lock(&used_lock);
	rc = fat_idx_create(&fidx, service_id);
	if (rc != EOK) {
		fibril_mutex_unlock(&used_lock);
		return rc;
	}
		
	hash_table_insert(&ui_hash, &fidx->uih_link);
	fibril_mutex_lock(&fidx->lock);
	fibril_mutex_unlock(&used_lock);

	*fidxp = fidx;
	return EOK;
}

fat_idx_t *
fat_idx_get_by_pos(service_id_t service_id, fat_cluster_t pfc, unsigned pdi)
{
	fat_idx_t *fidx;

	pos_key_t pos_key = {
		.service_id = service_id,
		.pfc = pfc,
		.pdi = pdi,
	};

	fibril_mutex_lock(&used_lock);
	ht_link_t *l = hash_table_find(&up_hash, &pos_key);
	if (l) {
		fidx = hash_table_get_inst(l, fat_idx_t, uph_link);
	} else {
		errno_t rc;

		rc = fat_idx_create(&fidx, service_id);
		if (rc != EOK) {
			fibril_mutex_unlock(&used_lock);
			return NULL;
		}
		
		fidx->pfc = pfc;
		fidx->pdi = pdi;

		hash_table_insert(&up_hash, &fidx->uph_link);
		hash_table_insert(&ui_hash, &fidx->uih_link);
	}
	fibril_mutex_lock(&fidx->lock);
	fibril_mutex_unlock(&used_lock);

	return fidx;
}

void fat_idx_hashin(fat_idx_t *idx)
{
	fibril_mutex_lock(&used_lock);
	hash_table_insert(&up_hash, &idx->uph_link);
	fibril_mutex_unlock(&used_lock);
}

void fat_idx_hashout(fat_idx_t *idx)
{
	fibril_mutex_lock(&used_lock);
	hash_table_remove_item(&up_hash, &idx->uph_link);
	fibril_mutex_unlock(&used_lock);
}

fat_idx_t *
fat_idx_get_by_index(service_id_t service_id, fs_index_t index)
{
	fat_idx_t *fidx = NULL;

	idx_key_t idx_key = {
		.service_id = service_id,
		.index = index,
	};

	fibril_mutex_lock(&used_lock);
	ht_link_t *l = hash_table_find(&ui_hash, &idx_key);
	if (l) {
		fidx = hash_table_get_inst(l, fat_idx_t, uih_link);
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
	idx_key_t idx_key = {
		.service_id = idx->service_id,
		.index = idx->index,
	};

	assert(idx->pfc == FAT_CLST_RES0);

	fibril_mutex_lock(&used_lock);
	/*
	 * Since we can only free unlinked nodes, the index structure is not
	 * present in the position hash (uph). We therefore hash it out from
	 * the index hash only.
	 */
	hash_table_remove(&ui_hash, &idx_key);
	fibril_mutex_unlock(&used_lock);
	/* Release the VFS index. */
	fat_index_free(idx_key.service_id, idx_key.index);
	/* The index structure itself is freed in idx_remove_callback(). */
}

errno_t fat_idx_init(void)
{
	if (!hash_table_create(&up_hash, 0, 0, &uph_ops)) 
		return ENOMEM;
	if (!hash_table_create(&ui_hash, 0, 0, &uih_ops)) {
		hash_table_destroy(&up_hash);
		return ENOMEM;
	}
	return EOK;
}

void fat_idx_fini(void)
{
	/* We assume the hash tables are empty. */
	assert(hash_table_empty(&up_hash) && hash_table_empty(&ui_hash));
	hash_table_destroy(&up_hash);
	hash_table_destroy(&ui_hash);
}

errno_t fat_idx_init_by_service_id(service_id_t service_id)
{
	unused_t *u;
	errno_t rc = EOK;

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

static bool rm_pos_service_id(ht_link_t *item, void *arg)
{
	service_id_t service_id = *(service_id_t*)arg;
	fat_idx_t *fidx = hash_table_get_inst(item, fat_idx_t, uph_link);

	if (fidx->service_id == service_id) {
		hash_table_remove_item(&up_hash, item);
	}
	
	return true;
}

static bool rm_idx_service_id(ht_link_t *item, void *arg)
{
	service_id_t service_id = *(service_id_t*)arg;
	fat_idx_t *fidx = hash_table_get_inst(item, fat_idx_t, uih_link);

	if (fidx->service_id == service_id) {
		hash_table_remove_item(&ui_hash, item);
	}
	
	return true;
}

void fat_idx_fini_by_service_id(service_id_t service_id)
{
	/*
	 * Remove this instance's index structure from up_hash and ui_hash.
	 * Process up_hash first and ui_hash second because the index structure
	 * is actually removed in idx_remove_callback(). 
	 */
	fibril_mutex_lock(&used_lock);
	hash_table_apply(&up_hash, rm_pos_service_id, &service_id);
	hash_table_apply(&ui_hash, rm_idx_service_id, &service_id);
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
