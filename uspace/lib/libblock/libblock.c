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
#include <ipc/devmap.h>
#include <ipc/bd.h>
#include <ipc/services.h>
#include <errno.h>
#include <sys/mman.h>
#include <async.h>
#include <ipc/ipc.h>
#include <as.h>
#include <assert.h>
#include <fibril_sync.h>
#include <adt/list.h>
#include <adt/hash_table.h>
#include <mem.h>

/** Lock protecting the device connection list */
static FIBRIL_MUTEX_INITIALIZE(dcl_lock);
/** Device connection list head. */
static LIST_INITIALIZE(dcl_head);

#define CACHE_BUCKETS_LOG2		10
#define CACHE_BUCKETS			(1 << CACHE_BUCKETS_LOG2)

typedef struct {
	fibril_mutex_t lock;
	size_t block_size;		/**< Block size. */
	unsigned block_count;		/**< Total number of blocks. */
	unsigned blocks_cached;		/**< Number of cached blocks. */
	hash_table_t block_hash;
	link_t free_head;
	enum cache_mode mode;
} cache_t;

typedef struct {
	link_t link;
	dev_handle_t dev_handle;
	int dev_phone;
	fibril_mutex_t com_area_lock;
	void *com_area;
	size_t com_size;
	void *bb_buf;
	off_t bb_off;
	size_t bb_size;
	cache_t *cache;
} devcon_t;

static int read_block(devcon_t *devcon, bn_t boff, size_t block_size);
static int write_block(devcon_t *devcon, bn_t boff, size_t block_size);

static devcon_t *devcon_search(dev_handle_t dev_handle)
{
	link_t *cur;

	fibril_mutex_lock(&dcl_lock);
	for (cur = dcl_head.next; cur != &dcl_head; cur = cur->next) {
		devcon_t *devcon = list_get_instance(cur, devcon_t, link);
		if (devcon->dev_handle == dev_handle) {
			fibril_mutex_unlock(&dcl_lock);
			return devcon;
		}
	}
	fibril_mutex_unlock(&dcl_lock);
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
	fibril_mutex_initialize(&devcon->com_area_lock);
	devcon->com_area = com_area;
	devcon->com_size = com_size;
	devcon->bb_buf = NULL;
	devcon->bb_off = 0;
	devcon->bb_size = 0;
	devcon->cache = NULL;

	fibril_mutex_lock(&dcl_lock);
	for (cur = dcl_head.next; cur != &dcl_head; cur = cur->next) {
		devcon_t *d = list_get_instance(cur, devcon_t, link);
		if (d->dev_handle == dev_handle) {
			fibril_mutex_unlock(&dcl_lock);
			free(devcon);
			return EEXIST;
		}
	}
	list_append(&devcon->link, &dcl_head);
	fibril_mutex_unlock(&dcl_lock);
	return EOK;
}

static void devcon_remove(devcon_t *devcon)
{
	fibril_mutex_lock(&dcl_lock);
	list_remove(&devcon->link);
	fibril_mutex_unlock(&dcl_lock);
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

	dev_phone = devmap_device_connect(dev_handle, IPC_FLAG_BLOCKING);
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
	
	fibril_mutex_lock(&devcon->com_area_lock);
	rc = read_block(devcon, 0, size);
	if (rc != EOK) {
		fibril_mutex_unlock(&devcon->com_area_lock);
	    	free(bb_buf);
		return rc;
	}
	memcpy(bb_buf, devcon->com_area, size);
	fibril_mutex_unlock(&devcon->com_area_lock);

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

int block_cache_init(dev_handle_t dev_handle, size_t size, unsigned blocks,
    enum cache_mode mode)
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
	
	fibril_mutex_initialize(&cache->lock);
	list_initialize(&cache->free_head);
	cache->block_size = size;
	cache->block_count = blocks;
	cache->blocks_cached = 0;
	cache->mode = mode;

	if (!hash_table_create(&cache->block_hash, CACHE_BUCKETS, 1,
	    &cache_ops)) {
		free(cache);
		return ENOMEM;
	}

	devcon->cache = cache;
	return EOK;
}

#define CACHE_LO_WATERMARK	10	
#define CACHE_HI_WATERMARK	20	
static bool cache_can_grow(cache_t *cache)
{
	if (cache->blocks_cached < CACHE_LO_WATERMARK)
		return true;
	if (!list_empty(&cache->free_head))
		return false;
	return true;
}

static void block_initialize(block_t *b)
{
	fibril_mutex_initialize(&b->lock);
	b->refcnt = 1;
	b->dirty = false;
	fibril_rwlock_initialize(&b->contents_lock);
	link_initialize(&b->free_link);
	link_initialize(&b->hash_link);
}

/** Instantiate a block in memory and get a reference to it.
 *
 * @param dev_handle		Device handle of the block device.
 * @param boff			Block offset.
 * @param flags			If BLOCK_FLAGS_NOREAD is specified, block_get()
 * 				will not read the contents of the block from the
 *				device.
 *
 * @return			Block structure.
 */
block_t *block_get(dev_handle_t dev_handle, bn_t boff, int flags)
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

retry:
	fibril_mutex_lock(&cache->lock);
	l = hash_table_find(&cache->block_hash, &key);
	if (l) {
		/*
		 * We found the block in the cache.
		 */
		b = hash_table_get_instance(l, block_t, hash_link);
		fibril_mutex_lock(&b->lock);
		if (b->refcnt++ == 0)
			list_remove(&b->free_link);
		fibril_mutex_unlock(&b->lock);
		fibril_mutex_unlock(&cache->lock);
	} else {
		/*
		 * The block was not found in the cache.
		 */
		int rc;

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
			cache->blocks_cached++;
		} else {
			/*
			 * Try to recycle a block from the free list.
			 */
			unsigned long temp_key;
recycle:
			assert(!list_empty(&cache->free_head));
			l = cache->free_head.next;
			b = list_get_instance(l, block_t, free_link);

			fibril_mutex_lock(&b->lock);
			if (b->dirty) {
				/*
				 * The block needs to be written back to the
				 * device before it changes identity. Do this
				 * while not holding the cache lock so that
				 * concurrency is not impeded. Also move the
				 * block to the end of the free list so that we
				 * do not slow down other instances of
				 * block_get() draining the free list.
				 */
				list_remove(&b->free_link);
				list_append(&b->free_link, &cache->free_head);
				fibril_mutex_unlock(&cache->lock);
				fibril_mutex_lock(&devcon->com_area_lock);
				memcpy(devcon->com_area, b->data, b->size);
				rc = write_block(devcon, b->boff,
				    cache->block_size);
				fibril_mutex_unlock(&devcon->com_area_lock);
				assert(rc == EOK);
				b->dirty = false;
				if (!fibril_mutex_trylock(&cache->lock)) {
					/*
					 * Somebody is probably racing with us.
					 * Unlock the block and retry.
					 */
					fibril_mutex_unlock(&b->lock);
					goto retry;
				}

			}
			fibril_mutex_unlock(&b->lock);

			/*
			 * Unlink the block from the free list and the hash
			 * table.
			 */
			list_remove(&b->free_link);
			temp_key = b->boff;
			hash_table_remove(&cache->block_hash, &temp_key, 1);
		}

		block_initialize(b);
		b->dev_handle = dev_handle;
		b->size = cache->block_size;
		b->boff = boff;
		hash_table_insert(&cache->block_hash, &key, &b->hash_link);

		/*
		 * Lock the block before releasing the cache lock. Thus we don't
		 * kill concurrent operations on the cache while doing I/O on
		 * the block.
		 */
		fibril_mutex_lock(&b->lock);
		fibril_mutex_unlock(&cache->lock);

		if (!(flags & BLOCK_FLAGS_NOREAD)) {
			/*
			 * The block contains old or no data. We need to read
			 * the new contents from the device.
			 */
			fibril_mutex_lock(&devcon->com_area_lock);
			rc = read_block(devcon, b->boff, cache->block_size);
			assert(rc == EOK);
			memcpy(b->data, devcon->com_area, cache->block_size);
			fibril_mutex_unlock(&devcon->com_area_lock);
		}

		fibril_mutex_unlock(&b->lock);
	}
	return b;
}

/** Release a reference to a block.
 *
 * If the last reference is dropped, the block is put on the free list.
 *
 * @param block		Block of which a reference is to be released.
 */
void block_put(block_t *block)
{
	devcon_t *devcon = devcon_search(block->dev_handle);
	cache_t *cache;
	unsigned blocks_cached;
	enum cache_mode mode;
	int rc;

	assert(devcon);
	assert(devcon->cache);

	cache = devcon->cache;

retry:
	fibril_mutex_lock(&cache->lock);
	blocks_cached = cache->blocks_cached;
	mode = cache->mode;
	fibril_mutex_unlock(&cache->lock);

	/*
	 * Determine whether to sync the block. Syncing the block is best done
	 * when not holding the cache lock as it does not impede concurrency.
	 * Since the situation may have changed when we unlocked the cache, the
	 * blocks_cached and mode variables are mere hints. We will recheck the
	 * conditions later when the cache lock is held again.
	 */
	fibril_mutex_lock(&block->lock);
	if (block->dirty && (block->refcnt == 1) &&
	    (blocks_cached > CACHE_HI_WATERMARK || mode != CACHE_MODE_WB)) {
		fibril_mutex_lock(&devcon->com_area_lock);
		memcpy(devcon->com_area, block->data, block->size);
		rc = write_block(devcon, block->boff, block->size);
		assert(rc == EOK);
		fibril_mutex_unlock(&devcon->com_area_lock);
		block->dirty = false;
	}
	fibril_mutex_unlock(&block->lock);

	fibril_mutex_lock(&cache->lock);
	fibril_mutex_lock(&block->lock);
	if (!--block->refcnt) {
		/*
		 * Last reference to the block was dropped. Either free the
		 * block or put it on the free list.
		 */
		if (cache->blocks_cached > CACHE_HI_WATERMARK) {
			/*
			 * Currently there are too many cached blocks.
			 */
			if (block->dirty) {
				/*
				 * We cannot sync the block while holding the
				 * cache lock. Release everything and retry.
				 */
				block->refcnt++;
				fibril_mutex_unlock(&block->lock);
				fibril_mutex_unlock(&cache->lock);
				goto retry;
			}
			/*
			 * Take the block out of the cache and free it.
			 */
			unsigned long key = block->boff;
			hash_table_remove(&cache->block_hash, &key, 1);
			free(block);
			free(block->data);
			cache->blocks_cached--;
			fibril_mutex_unlock(&cache->lock);
			return;
		}
		/*
		 * Put the block on the free list.
		 */
		if (cache->mode != CACHE_MODE_WB && block->dirty) {
			/*
			 * We cannot sync the block while holding the cache
			 * lock. Release everything and retry.
			 */
			block->refcnt++;
			fibril_mutex_unlock(&block->lock);
			fibril_mutex_unlock(&cache->lock);
			goto retry;
		}
		list_append(&block->free_link, &cache->free_head);
	}
	fibril_mutex_unlock(&block->lock);
	fibril_mutex_unlock(&cache->lock);
}

/** Read sequential data from a block device.
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
int block_seqread(dev_handle_t dev_handle, off_t *bufpos, size_t *buflen,
    off_t *pos, void *dst, size_t size, size_t block_size)
{
	off_t offset = 0;
	size_t left = size;
	devcon_t *devcon = devcon_search(dev_handle);
	assert(devcon);
	
	fibril_mutex_lock(&devcon->com_area_lock);
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
		
		if (*bufpos == (off_t) *buflen) {
			/* Refill the communication buffer with a new block. */
			int rc;

			rc = read_block(devcon, *pos / block_size, block_size);
			if (rc != EOK) {
				fibril_mutex_unlock(&devcon->com_area_lock);
				return rc;
			}
			
			*bufpos = 0;
			*buflen = block_size;
		}
	}
	fibril_mutex_unlock(&devcon->com_area_lock);
	
	return EOK;
}

/** Read block from block device.
 *
 * @param devcon	Device connection.
 * @param boff		Block index.
 * @param block_size	Block size.
 * @param src		Buffer for storing the data.
 *
 * @return		EOK on success or negative error code on failure.
 */
static int read_block(devcon_t *devcon, bn_t boff, size_t block_size)
{
	ipcarg_t retval;
	int rc;

	assert(devcon);
	rc = async_req_2_1(devcon->dev_phone, BD_READ_BLOCK, boff, block_size,
	    &retval);
	if ((rc != EOK) || (retval != EOK))
		return (rc != EOK ? rc : (int) retval);

	return EOK;
}

/** Write block to block device.
 *
 * @param devcon	Device connection.
 * @param boff		Block index.
 * @param block_size	Block size.
 * @param src		Buffer containing the data to write.
 *
 * @return		EOK on success or negative error code on failure.
 */
static int write_block(devcon_t *devcon, bn_t boff, size_t block_size)
{
	ipcarg_t retval;
	int rc;

	assert(devcon);
	rc = async_req_2_1(devcon->dev_phone, BD_WRITE_BLOCK, boff, block_size,
	    &retval);
	if ((rc != EOK) || (retval != EOK))
		return (rc != EOK ? rc : (int) retval);

	return EOK;
}

/** @}
 */
