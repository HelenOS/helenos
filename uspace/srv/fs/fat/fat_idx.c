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

static LIST_INITIALIZE(unused_head);

/** Allocate a VFS index which is not currently in use. */
static bool fat_idx_alloc(dev_handle_t dev_handle, fs_index_t *index)
{
	link_t *l;
	unused_t *u;
	
	assert(index);
	for (l = unused_head.next; l != &unused_head; l = l->next) {
		u = list_get_instance(l, unused_t, link);
		if (u->dev_handle == dev_handle) 
			goto hit;
	}

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
		return true;
	}
	/*
	 * We ran out of indices, which is extremely unlikely with FAT16, but
	 * theoretically still possible (e.g. too many open unlinked nodes or
	 * too many zero-sized nodes).
	 */
	return false;
}

/** If possible, merge two intervals of freed indices. */
static void try_merge_intervals(link_t *l, link_t *r, link_t *cur)
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

	for (l = unused_head.next; l != &unused_head; l = l->next) {
		u = list_get_instance(l, unused_t, link);
		if (u->dev_handle == dev_handle)
			goto hit;
	}

	/* should not happen */
	assert(0);

hit:
	if (u->next == index + 1) {
		/* The index can be returned directly to the counter. */
		u->next--;
		u->remaining++;
		return;
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
					try_merge_intervals(lnk->prev, lnk,
					    lnk);
				return;
			}
			if (f->last == index - 1) {
				f->last++;
				if (lnk->next != &u->freed_head)
					try_merge_intervals(lnk, lnk->next,
					    lnk);
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
}

fat_idx_t *fat_idx_map(dev_handle_t dev_handle, fat_cluster_t pfc, unsigned pdi)
{
	return NULL;	/* TODO */
}
