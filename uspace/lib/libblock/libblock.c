/*
 * Copyright (c) 2008 Jakub Jermar 
 * Copyright (c) 2008 Martin Decky 
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

/** @addtogroup libblock 
 * @{
 */ 
/**
 * @file
 * @brief
 */

#include "libblock.h" 
#include "../../srv/vfs/vfs.h"
#include "../../srv/rd/rd.h"
#include <ipc/devmap.h>
#include <ipc/services.h>
#include <errno.h>
#include <sys/mman.h>
#include <async.h>
#include <ipc/ipc.h>
#include <as.h>
#include <assert.h>
#include <futex.h>
#include <libadt/list.h>
#include <libadt/hash_table.h>

/** Lock protecting the device connection list */
static futex_t dcl_lock = FUTEX_INITIALIZER;
/** Device connection list head. */
static LIST_INITIALIZE(dcl_head);

#define CACHE_BUCKETS_LOG2		10
#define CACHE_BUCKETS			(1 << CACHE_BUCKETS_LOG2)

typedef struct {
	futex_t lock;
	size_t block_size;		/**< Block size. */
	unsigned block_count;		/**< Total number of blocks. */
	hash_table_t block_hash;
	link_t free_head;
} cache_t;

typedef struct {
	link_t link;
	int dev_handle;
	int dev_phone;
	void *com_area;
	size_t com_size;
	void *bb_buf;
	off_t bb_off;
	size_t bb_size;
	cache_t *cache;
} devcon_t;

static devcon_t *devcon_search(dev_handle_t dev_handle)
{
	link_t *cur;

	futex_down(&dcl_lock);
	for (cur = dcl_head.next; cur != &dcl_head; cur = cur->next) {
		devcon_t *devcon = list_get_instance(cur, devcon_t, link);
		if (devcon->dev_handle == dev_handle) {
			futex_up(&dcl_lock);
			return devcon;
		}
	}
	futex_up(&dcl_lock);
	return NULL;
}

static int devcon_add(dev_handle_t dev_handle, int dev_phone, void *com_area,
   size_t com_size)
{
	link_t *cur;
	devcon_t *devcon;

	devcon = malloc(sizeof(devcon_t));
	if (!devcon)
		return ENOMEM;
	
	link_initialize(&devcon->link);
	devcon->dev_handle = dev_handle;
	devcon->dev_phone = dev_phone;
	devcon->com_area = com_area;
	devcon->com_size = com_size;
	devcon->bb_buf = NULL;
	devcon->bb_off = 0;
	devcon->bb_size = 0;
	devcon->cache = NULL;

	futex_down(&dcl_lock);
	for (cur = dcl_head.next; cur != &dcl_head; cur = cur->next) {
		devcon_t *d = list_get_instance(cur, devcon_t, link);
		if (d->dev_handle == dev_handle) {
			futex_up(&dcl_lock);
			free(devcon);
			return EEXIST;
		}
	}
	list_append(&devcon->link, &dcl_head);
	futex_up(&dcl_lock);
	return EOK;
}

static void devcon_remove(devcon_t *devcon)
{
	futex_down(&dcl_lock);
	list_remove(&devcon->link);
	futex_up(&dcl_lock);
}

int block_init(dev_handle_t dev_handle, size_t com_size)
{
	int rc;
	int dev_phone;
	void *com_area;
	
	com_area = mmap(NULL, com_size, PROTO_READ | PROTO_WRITE,
	    MAP_ANONYMOUS | MAP_PRIVATE, 0, 0);
	if (!com_area) {
		return ENOMEM;
	}
	dev_phone = ipc_connect_me_to(PHONE_NS, SERVICE_DEVMAP,
	    DEVMAP_CONNECT_TO_DEVICE, dev_handle);

	if (dev_phone < 0) {
		munmap(com_area, com_size);
		return dev_phone;
	}

	rc = ipc_share_out_start(dev_phone, com_area,
	    AS_AREA_READ | AS_AREA_WRITE);
	if (rc != EOK) {
	    	munmap(com_area, com_size);
		ipc_hangup(dev_phone);
		return rc;
	}
	
	rc = devcon_add(dev_handle, dev_phone, com_area, com_size);
	if (rc != EOK) {
		munmap(com_area, com_size);
		ipc_hangup(dev_phone);
		return rc;
	}

	return EOK;
}

void block_fini(dev_handle_t dev_handle)
{
	devcon_t *devcon = devcon_search(dev_handle);
	assert(devcon);
	
	devcon_remove(devcon);

	if (devcon->bb_buf)
		free(devcon->bb_buf);

	if (devcon->cache) {
		hash_table_destroy(&devcon->cache->block_hash);
		free(devcon->cache);
	}

	munmap(devcon->com_area, devcon->com_size);
	ipc_hangup(devcon->dev_phone);

	free(devcon);	
}

int block_bb_read(dev_handle_t dev_handle, off_t off, size_t size)
{
	void *bb_buf;
	int rc;

	devcon_t *devcon = devcon_search(dev_handle);
	if (!devcon)
		return ENOENT;
	if (devcon->bb_buf)
		return EEXIST;
	bb_buf = malloc(size);
	if (!bb_buf)
		return ENOMEM;
	
	off_t bufpos = 0;
	size_t buflen = 0;
	rc = block_read(dev_handle, &bufpos, &buflen, &off,
	    bb_buf, size, size);
	if (rc != EOK) {
	    	free(bb_buf);
		return rc;
	}
	devcon->bb_buf = bb_buf;
	devcon->bb_off = off;
	devcon->bb_size = size;

	return EOK;
}

void *block_bb_get(dev_handle_t dev_handle)
{
	devcon_t *devcon = devcon_search(dev_handle);
	assert(devcon);
	return devcon->bb_buf;
}

static hash_index_t cache_hash(unsigned long *key)
{
	return *key & (CACHE_BUCKETS - 1);
}

static int cache_compare(unsigned long *key, hash_count_t keys, link_t *item)
{
	block_t *b = hash_table_get_instance(item, block_t, hash_link);
	return b->boff == *key;
}

static void cache_remove_callback(link_t *item)
{
}

static hash_table_operations_t cache_ops = {
	.hash = cache_hash,
	.compare = cache_compare,
	.remove_callback = cache_remove_callback
};

int block_cache_init(dev_handle_t dev_handle, size_t size, unsigned blocks)
{
	devcon_t *devcon = devcon_search(dev_handle);
	cache_t *cache;
	if (!devcon)
		return ENOENT;
	if (devcon->cache)
		return EEXIST;
	cache = malloc(sizeof(cache_t));
	if (!cache)
		return ENOMEM;
	
	futex_initialize(&cache->lock, 1);
	list_initialize(&cache->free_head);
	cache->block_size = size;
	cache->block_count = blocks;

	if (!hash_table_create(&cache->block_hash, CACHE_BUCKETS, 1,
	    &cache_ops)) {
		free(cache);
		return ENOMEM;
	}

	devcon->cache = cache;
	return EOK;
}

/** Read data from a block device.
 *
 * @param dev_handle	Device handle of the block device.
 * @param bufpos	Pointer to the first unread valid offset within the
 * 			communication buffer.
 * @param buflen	Pointer to the number of unread bytes that are ready in
 * 			the communication buffer.
 * @param pos		Device position to be read.
 * @param dst		Destination buffer.
 * @param size		Size of the destination buffer.
 * @param block_size	Block size to be used for the transfer.
 *
 * @return		EOK on success or a negative return code on failure.
 */
int
block_read(int dev_handle, off_t *bufpos, size_t *buflen, off_t *pos, void *dst,
    size_t size, size_t block_size)
{
	off_t offset = 0;
	size_t left = size;
	devcon_t *devcon = devcon_search(dev_handle);
	assert(devcon);
	
	while (left > 0) {
		size_t rd;
		
		if (*bufpos + left < *buflen)
			rd = left;
		else
			rd = *buflen - *bufpos;
		
		if (rd > 0) {
			/*
			 * Copy the contents of the communication buffer to the
			 * destination buffer.
			 */
			memcpy(dst + offset, devcon->com_area + *bufpos, rd);
			offset += rd;
			*bufpos += rd;
			*pos += rd;
			left -= rd;
		}
		
		if (*bufpos == *buflen) {
			/* Refill the communication buffer with a new block. */
			ipcarg_t retval;
			int rc = async_req_2_1(devcon->dev_phone, RD_READ_BLOCK,
			    *pos / block_size, block_size, &retval);
			if ((rc != EOK) || (retval != EOK))
				return (rc != EOK ? rc : retval);
			
			*bufpos = 0;
			*buflen = block_size;
		}
	}
	
	return EOK;
}

static bool cache_can_grow(cache_t *cache)
{
	return true;
}

static void block_initialize(block_t *b)
{
	futex_initialize(&b->lock, 1);
	b->refcnt = 1;
	b->dirty = false;
	rwlock_initialize(&b->contents_lock);
	link_initialize(&b->free_link);
	link_initialize(&b->hash_link);
}

/** Instantiate a block in memory and get a reference to it.
 *
 * @param dev_handle		Device handle of the block device.
 * @param boff			Block offset.
 *
 * @return			Block structure.
 */
block_t *block_get(dev_handle_t dev_handle, off_t boff)
{
	devcon_t *devcon;
	cache_t *cache;
	block_t *b;
	link_t *l;
	unsigned long key = boff;
	
	devcon = devcon_search(dev_handle);

	assert(devcon);
	assert(devcon->cache);
	
	cache = devcon->cache;
	futex_down(&cache->lock);
	l = hash_table_find(&cache->block_hash, &key);
	if (l) {
		/*
		 * We found the block in the cache.
		 */
		b = hash_table_get_instance(l, block_t, hash_link);
		futex_down(&b->lock);
		if (b->refcnt++ == 0)
			list_remove(&b->free_link);
		futex_up(&b->lock);
	} else {
		/*
		 * The block was not found in the cache.
		 */
		int rc;
		off_t bufpos = 0;
		size_t buflen = 0;
		off_t pos = boff * cache->block_size;

		if (cache_can_grow(cache)) {
			/*
			 * We can grow the cache by allocating new blocks.
			 * Should the allocation fail, we fail over and try to
			 * recycle a block from the cache.
			 */
			b = malloc(sizeof(block_t));
			if (!b)
				goto recycle;
			b->data = malloc(cache->block_size);
			if (!b->data) {
				free(b);
				goto recycle;
			}
		} else {
			/*
			 * Try to recycle a block from the free list.
			 */
			unsigned long temp_key;
recycle:
			assert(!list_empty(&cache->free_head));
			l = cache->free_head.next;
			list_remove(l);
			b = hash_table_get_instance(l, block_t, hash_link);
			assert(!b->dirty);
			temp_key = b->boff;
			hash_table_remove(&cache->block_hash, &temp_key, 1);
		}

		block_initialize(b);
		b->dev_handle = dev_handle;
		b->size = cache->block_size;
		b->boff = boff;
		/* read block from the device */
		rc = block_read(dev_handle, &bufpos, &buflen, &pos, b->data,
		    cache->block_size, cache->block_size);
		assert(rc == EOK);
		hash_table_insert(&cache->block_hash, &key, &b->hash_link);
	}

	futex_up(&cache->lock);
	return b;
}

void block_put(block_t *block)
{
	
}

/** @}
 */
