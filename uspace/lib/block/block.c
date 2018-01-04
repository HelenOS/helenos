/*
 * Copyright (c) 2008 Jakub Jermar
 * Copyright (c) 2008 Martin Decky
 * Copyright (c) 2011 Martin Sucha
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

#include <ipc/loc.h>
#include <ipc/services.h>
#include <errno.h>
#include <async.h>
#include <as.h>
#include <assert.h>
#include <bd.h>
#include <fibril_synch.h>
#include <adt/list.h>
#include <adt/hash_table.h>
#include <macros.h>
#include <mem.h>
#include <stdlib.h>
#include <stdio.h>
#include <stacktrace.h>
#include <str_error.h>
#include <offset.h>
#include <inttypes.h>
#include "block.h"

#define MAX_WRITE_RETRIES 10

/** Lock protecting the device connection list */
static FIBRIL_MUTEX_INITIALIZE(dcl_lock);
/** Device connection list head. */
static LIST_INITIALIZE(dcl);


typedef struct {
	fibril_mutex_t lock;
	size_t lblock_size;       /**< Logical block size. */
	unsigned blocks_cluster;  /**< Physical blocks per block_t */
	unsigned block_count;     /**< Total number of blocks. */
	unsigned blocks_cached;   /**< Number of cached blocks. */
	hash_table_t block_hash;
	list_t free_list;
	enum cache_mode mode;
} cache_t;

typedef struct {
	link_t link;
	service_id_t service_id;
	async_sess_t *sess;
	bd_t *bd;
	void *bb_buf;
	aoff64_t bb_addr;
	aoff64_t pblocks;    /**< Number of physical blocks */
	size_t pblock_size;  /**< Physical block size. */
	cache_t *cache;
} devcon_t;

static errno_t read_blocks(devcon_t *, aoff64_t, size_t, void *, size_t);
static errno_t write_blocks(devcon_t *, aoff64_t, size_t, void *, size_t);
static aoff64_t ba_ltop(devcon_t *, aoff64_t);

static devcon_t *devcon_search(service_id_t service_id)
{
	fibril_mutex_lock(&dcl_lock);
	
	list_foreach(dcl, link, devcon_t, devcon) {
		if (devcon->service_id == service_id) {
			fibril_mutex_unlock(&dcl_lock);
			return devcon;
		}
	}
	
	fibril_mutex_unlock(&dcl_lock);
	return NULL;
}

static errno_t devcon_add(service_id_t service_id, async_sess_t *sess,
    size_t bsize, aoff64_t dev_size, bd_t *bd)
{
	devcon_t *devcon;
	
	devcon = malloc(sizeof(devcon_t));
	if (!devcon)
		return ENOMEM;
	
	link_initialize(&devcon->link);
	devcon->service_id = service_id;
	devcon->sess = sess;
	devcon->bd = bd;
	devcon->bb_buf = NULL;
	devcon->bb_addr = 0;
	devcon->pblock_size = bsize;
	devcon->pblocks = dev_size;
	devcon->cache = NULL;
	
	fibril_mutex_lock(&dcl_lock);
	list_foreach(dcl, link, devcon_t, d) {
		if (d->service_id == service_id) {
			fibril_mutex_unlock(&dcl_lock);
			free(devcon);
			return EEXIST;
		}
	}
	list_append(&devcon->link, &dcl);
	fibril_mutex_unlock(&dcl_lock);
	return EOK;
}

static void devcon_remove(devcon_t *devcon)
{
	fibril_mutex_lock(&dcl_lock);
	list_remove(&devcon->link);
	fibril_mutex_unlock(&dcl_lock);
}

errno_t block_init(service_id_t service_id, size_t comm_size)
{
	bd_t *bd;

	async_sess_t *sess = loc_service_connect(service_id, INTERFACE_BLOCK,
	    IPC_FLAG_BLOCKING);
	if (!sess) {
		return ENOENT;
	}
	
	errno_t rc = bd_open(sess, &bd);
	if (rc != EOK) {
		async_hangup(sess);
		return rc;
	}
	
	size_t bsize;
	rc = bd_get_block_size(bd, &bsize);
	if (rc != EOK) {
		bd_close(bd);
		async_hangup(sess);
		return rc;
	}

	aoff64_t dev_size;
	rc = bd_get_num_blocks(bd, &dev_size);
	if (rc != EOK) {
		bd_close(bd);
		async_hangup(sess);
		return rc;
	}
	
	rc = devcon_add(service_id, sess, bsize, dev_size, bd);
	if (rc != EOK) {
		bd_close(bd);
		async_hangup(sess);
		return rc;
	}
	
	return EOK;
}

void block_fini(service_id_t service_id)
{
	devcon_t *devcon = devcon_search(service_id);
	assert(devcon);
	
	if (devcon->cache)
		(void) block_cache_fini(service_id);
	
	(void)bd_sync_cache(devcon->bd, 0, 0);
	
	devcon_remove(devcon);
	
	if (devcon->bb_buf)
		free(devcon->bb_buf);
	
	bd_close(devcon->bd);
	async_hangup(devcon->sess);
	
	free(devcon);
}

errno_t block_bb_read(service_id_t service_id, aoff64_t ba)
{
	void *bb_buf;
	errno_t rc;

	devcon_t *devcon = devcon_search(service_id);
	if (!devcon)
		return ENOENT;
	if (devcon->bb_buf)
		return EEXIST;
	bb_buf = malloc(devcon->pblock_size);
	if (!bb_buf)
		return ENOMEM;

	rc = read_blocks(devcon, 0, 1, bb_buf, devcon->pblock_size);
	if (rc != EOK) {
	    	free(bb_buf);
		return rc;
	}

	devcon->bb_buf = bb_buf;
	devcon->bb_addr = ba;

	return EOK;
}

void *block_bb_get(service_id_t service_id)
{
	devcon_t *devcon = devcon_search(service_id);
	assert(devcon);
	return devcon->bb_buf;
}

static size_t cache_key_hash(void *key)
{
	aoff64_t *lba = (aoff64_t*)key;
	return *lba;
}

static size_t cache_hash(const ht_link_t *item)
{
	block_t *b = hash_table_get_inst(item, block_t, hash_link);
	return b->lba;
}

static bool cache_key_equal(void *key, const ht_link_t *item)
{
	aoff64_t *lba = (aoff64_t*)key;
	block_t *b = hash_table_get_inst(item, block_t, hash_link);
	return b->lba == *lba;
}


static hash_table_ops_t cache_ops = {
	.hash = cache_hash,
	.key_hash = cache_key_hash,
	.key_equal = cache_key_equal,
	.equal = NULL,
	.remove_callback = NULL
};

errno_t block_cache_init(service_id_t service_id, size_t size, unsigned blocks,
    enum cache_mode mode)
{
	devcon_t *devcon = devcon_search(service_id);
	cache_t *cache;
	if (!devcon)
		return ENOENT;
	if (devcon->cache)
		return EEXIST;
	cache = malloc(sizeof(cache_t));
	if (!cache)
		return ENOMEM;
	
	fibril_mutex_initialize(&cache->lock);
	list_initialize(&cache->free_list);
	cache->lblock_size = size;
	cache->block_count = blocks;
	cache->blocks_cached = 0;
	cache->mode = mode;

	/* Allow 1:1 or small-to-large block size translation */
	if (cache->lblock_size % devcon->pblock_size != 0) {
		free(cache);
		return ENOTSUP;
	}

	cache->blocks_cluster = cache->lblock_size / devcon->pblock_size;

	if (!hash_table_create(&cache->block_hash, 0, 0, &cache_ops)) {
		free(cache);
		return ENOMEM;
	}

	devcon->cache = cache;
	return EOK;
}

errno_t block_cache_fini(service_id_t service_id)
{
	devcon_t *devcon = devcon_search(service_id);
	cache_t *cache;
	errno_t rc;

	if (!devcon)
		return ENOENT;
	if (!devcon->cache)
		return EOK;
	cache = devcon->cache;
	
	/*
	 * We are expecting to find all blocks for this device handle on the
	 * free list, i.e. the block reference count should be zero. Do not
	 * bother with the cache and block locks because we are single-threaded.
	 */
	while (!list_empty(&cache->free_list)) {
		block_t *b = list_get_instance(list_first(&cache->free_list),
		    block_t, free_link);

		list_remove(&b->free_link);
		if (b->dirty) {
			rc = write_blocks(devcon, b->pba, cache->blocks_cluster,
			    b->data, b->size);
			if (rc != EOK)
				return rc;
		}

		hash_table_remove_item(&cache->block_hash, &b->hash_link);
		
		free(b->data);
		free(b);
	}

	hash_table_destroy(&cache->block_hash);
	devcon->cache = NULL;
	free(cache);

	return EOK;
}

#define CACHE_LO_WATERMARK	10	
#define CACHE_HI_WATERMARK	20	
static bool cache_can_grow(cache_t *cache)
{
	if (cache->blocks_cached < CACHE_LO_WATERMARK)
		return true;
	if (!list_empty(&cache->free_list))
		return false;
	return true;
}

static void block_initialize(block_t *b)
{
	fibril_mutex_initialize(&b->lock);
	b->refcnt = 1;
	b->write_failures = 0;
	b->dirty = false;
	b->toxic = false;
	fibril_rwlock_initialize(&b->contents_lock);
	link_initialize(&b->free_link);
}

/** Instantiate a block in memory and get a reference to it.
 *
 * @param block			Pointer to where the function will store the
 * 				block pointer on success.
 * @param service_id		Service ID of the block device.
 * @param ba			Block address (logical).
 * @param flags			If BLOCK_FLAGS_NOREAD is specified, block_get()
 * 				will not read the contents of the block from the
 *				device.
 *
 * @return			EOK on success or an error code.
 */
errno_t block_get(block_t **block, service_id_t service_id, aoff64_t ba, int flags)
{
	devcon_t *devcon;
	cache_t *cache;
	block_t *b;
	link_t *link;
	aoff64_t p_ba;
	errno_t rc;
	
	devcon = devcon_search(service_id);

	assert(devcon);
	assert(devcon->cache);
	
	cache = devcon->cache;

	/* Check whether the logical block (or part of it) is beyond
	 * the end of the device or not.
	 */
	p_ba = ba_ltop(devcon, ba);
	p_ba += cache->blocks_cluster;
	if (p_ba >= devcon->pblocks) {
		/* This request cannot be satisfied */
		return EIO;
	}


retry:
	rc = EOK;
	b = NULL;

	fibril_mutex_lock(&cache->lock);
	ht_link_t *hlink = hash_table_find(&cache->block_hash, &ba);
	if (hlink) {
found:
		/*
		 * We found the block in the cache.
		 */
		b = hash_table_get_inst(hlink, block_t, hash_link);
		fibril_mutex_lock(&b->lock);
		if (b->refcnt++ == 0)
			list_remove(&b->free_link);
		if (b->toxic)
			rc = EIO;
		fibril_mutex_unlock(&b->lock);
		fibril_mutex_unlock(&cache->lock);
	} else {
		/*
		 * The block was not found in the cache.
		 */
		if (cache_can_grow(cache)) {
			/*
			 * We can grow the cache by allocating new blocks.
			 * Should the allocation fail, we fail over and try to
			 * recycle a block from the cache.
			 */
			b = malloc(sizeof(block_t));
			if (!b)
				goto recycle;
			b->data = malloc(cache->lblock_size);
			if (!b->data) {
				free(b);
				b = NULL;
				goto recycle;
			}
			cache->blocks_cached++;
		} else {
			/*
			 * Try to recycle a block from the free list.
			 */
recycle:
			if (list_empty(&cache->free_list)) {
				fibril_mutex_unlock(&cache->lock);
				rc = ENOMEM;
				goto out;
			}
			link = list_first(&cache->free_list);
			b = list_get_instance(link, block_t, free_link);

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
				list_append(&b->free_link, &cache->free_list);
				fibril_mutex_unlock(&cache->lock);
				rc = write_blocks(devcon, b->pba,
				    cache->blocks_cluster, b->data, b->size);
				if (rc != EOK) {
					/*
					 * We did not manage to write the block
					 * to the device. Keep it around for
					 * another try. Hopefully, we will grab
					 * another block next time.
					 */
					if (b->write_failures < MAX_WRITE_RETRIES) {
						b->write_failures++;
						fibril_mutex_unlock(&b->lock);
						goto retry;
					} else {
						printf("Too many errors writing block %"
				    		    PRIuOFF64 "from device handle %" PRIun "\n"
						    "SEVERE DATA LOSS POSSIBLE\n",
				    		    b->lba, devcon->service_id);
					}
				} else
					b->write_failures = 0;

				b->dirty = false;
				if (!fibril_mutex_trylock(&cache->lock)) {
					/*
					 * Somebody is probably racing with us.
					 * Unlock the block and retry.
					 */
					fibril_mutex_unlock(&b->lock);
					goto retry;
				}
				hlink = hash_table_find(&cache->block_hash, &ba);
				if (hlink) {
					/*
					 * Someone else must have already
					 * instantiated the block while we were
					 * not holding the cache lock.
					 * Leave the recycled block on the
					 * freelist and continue as if we
					 * found the block of interest during
					 * the first try.
					 */
					fibril_mutex_unlock(&b->lock);
					goto found;
				}

			}
			fibril_mutex_unlock(&b->lock);

			/*
			 * Unlink the block from the free list and the hash
			 * table.
			 */
			list_remove(&b->free_link);
			hash_table_remove_item(&cache->block_hash, &b->hash_link);
		}

		block_initialize(b);
		b->service_id = service_id;
		b->size = cache->lblock_size;
		b->lba = ba;
		b->pba = ba_ltop(devcon, b->lba);
		hash_table_insert(&cache->block_hash, &b->hash_link);

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
			rc = read_blocks(devcon, b->pba, cache->blocks_cluster,
			    b->data, cache->lblock_size);
			if (rc != EOK) 
				b->toxic = true;
		} else
			rc = EOK;

		fibril_mutex_unlock(&b->lock);
	}
out:
	if ((rc != EOK) && b) {
		assert(b->toxic);
		(void) block_put(b);
		b = NULL;
	}
	*block = b;
	return rc;
}

/** Release a reference to a block.
 *
 * If the last reference is dropped, the block is put on the free list.
 *
 * @param block		Block of which a reference is to be released.
 *
 * @return		EOK on success or an error code.
 */
errno_t block_put(block_t *block)
{
	devcon_t *devcon = devcon_search(block->service_id);
	cache_t *cache;
	unsigned blocks_cached;
	enum cache_mode mode;
	errno_t rc = EOK;

	assert(devcon);
	assert(devcon->cache);
	assert(block->refcnt >= 1);

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
	if (block->toxic)
		block->dirty = false;	/* will not write back toxic block */
	if (block->dirty && (block->refcnt == 1) &&
	    (blocks_cached > CACHE_HI_WATERMARK || mode != CACHE_MODE_WB)) {
		rc = write_blocks(devcon, block->pba, cache->blocks_cluster,
		    block->data, block->size);
		if (rc == EOK)
			block->write_failures = 0;
		block->dirty = false;
	}
	fibril_mutex_unlock(&block->lock);

	fibril_mutex_lock(&cache->lock);
	fibril_mutex_lock(&block->lock);
	if (!--block->refcnt) {
		/*
		 * Last reference to the block was dropped. Either free the
		 * block or put it on the free list. In case of an I/O error,
		 * free the block.
		 */
		if ((cache->blocks_cached > CACHE_HI_WATERMARK) ||
		    (rc != EOK)) {
			/*
			 * Currently there are too many cached blocks or there
			 * was an I/O error when writing the block back to the
			 * device.
			 */
			if (block->dirty) {
				/*
				 * We cannot sync the block while holding the
				 * cache lock. Release everything and retry.
				 */
				block->refcnt++;

				if (block->write_failures < MAX_WRITE_RETRIES) {
					block->write_failures++;
					fibril_mutex_unlock(&block->lock);
					fibril_mutex_unlock(&cache->lock);
					goto retry;
				} else {
					printf("Too many errors writing block %"
				            PRIuOFF64 "from device handle %" PRIun "\n"
					    "SEVERE DATA LOSS POSSIBLE\n",
				    	    block->lba, devcon->service_id);
				}
			}
			/*
			 * Take the block out of the cache and free it.
			 */
			hash_table_remove_item(&cache->block_hash, &block->hash_link);
			fibril_mutex_unlock(&block->lock);
			free(block->data);
			free(block);
			cache->blocks_cached--;
			fibril_mutex_unlock(&cache->lock);
			return rc;
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
		list_append(&block->free_link, &cache->free_list);
	}
	fibril_mutex_unlock(&block->lock);
	fibril_mutex_unlock(&cache->lock);

	return rc;
}

/** Read sequential data from a block device.
 *
 * @param service_id	Service ID of the block device.
 * @param buf		Buffer for holding one block
 * @param bufpos	Pointer to the first unread valid offset within the
 * 			communication buffer.
 * @param buflen	Pointer to the number of unread bytes that are ready in
 * 			the communication buffer.
 * @param pos		Device position to be read.
 * @param dst		Destination buffer.
 * @param size		Size of the destination buffer.
 * @param block_size	Block size to be used for the transfer.
 *
 * @return		EOK on success or an error code on failure.
 */
errno_t block_seqread(service_id_t service_id, void *buf, size_t *bufpos,
    size_t *buflen, aoff64_t *pos, void *dst, size_t size)
{
	size_t offset = 0;
	size_t left = size;
	size_t block_size;
	devcon_t *devcon;

	devcon = devcon_search(service_id);
	assert(devcon);
	block_size = devcon->pblock_size;
	
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
			memcpy(dst + offset, buf + *bufpos, rd);
			offset += rd;
			*bufpos += rd;
			*pos += rd;
			left -= rd;
		}
		
		if (*bufpos == *buflen) {
			/* Refill the communication buffer with a new block. */
			errno_t rc;

			rc = read_blocks(devcon, *pos / block_size, 1, buf,
			    devcon->pblock_size);
			if (rc != EOK) {
				return rc;
			}
			
			*bufpos = 0;
			*buflen = block_size;
		}
	}
	
	return EOK;
}

/** Read blocks directly from device (bypass cache).
 *
 * @param service_id	Service ID of the block device.
 * @param ba		Address of first block (physical).
 * @param cnt		Number of blocks.
 * @param src		Buffer for storing the data.
 *
 * @return		EOK on success or an error code on failure.
 */
errno_t block_read_direct(service_id_t service_id, aoff64_t ba, size_t cnt, void *buf)
{
	devcon_t *devcon;

	devcon = devcon_search(service_id);
	assert(devcon);

	return read_blocks(devcon, ba, cnt, buf, devcon->pblock_size * cnt);
}

/** Write blocks directly to device (bypass cache).
 *
 * @param service_id	Service ID of the block device.
 * @param ba		Address of first block (physical).
 * @param cnt		Number of blocks.
 * @param src		The data to be written.
 *
 * @return		EOK on success or an error code on failure.
 */
errno_t block_write_direct(service_id_t service_id, aoff64_t ba, size_t cnt,
    const void *data)
{
	devcon_t *devcon;

	devcon = devcon_search(service_id);
	assert(devcon);

	return write_blocks(devcon, ba, cnt, (void *)data, devcon->pblock_size * cnt);
}

/** Synchronize blocks to persistent storage.
 *
 * @param service_id	Service ID of the block device.
 * @param ba		Address of first block (physical).
 * @param cnt		Number of blocks.
 *
 * @return		EOK on success or an error code on failure.
 */
errno_t block_sync_cache(service_id_t service_id, aoff64_t ba, size_t cnt)
{
	devcon_t *devcon;

	devcon = devcon_search(service_id);
	assert(devcon);

	return bd_sync_cache(devcon->bd, ba, cnt);
}

/** Get device block size.
 *
 * @param service_id	Service ID of the block device.
 * @param bsize		Output block size.
 *
 * @return		EOK on success or an error code on failure.
 */
errno_t block_get_bsize(service_id_t service_id, size_t *bsize)
{
	devcon_t *devcon;

	devcon = devcon_search(service_id);
	assert(devcon);

	return bd_get_block_size(devcon->bd, bsize);
}

/** Get number of blocks on device.
 *
 * @param service_id	Service ID of the block device.
 * @param nblocks	Output number of blocks.
 *
 * @return		EOK on success or an error code on failure.
 */
errno_t block_get_nblocks(service_id_t service_id, aoff64_t *nblocks)
{
	devcon_t *devcon = devcon_search(service_id);
	assert(devcon);

	return bd_get_num_blocks(devcon->bd, nblocks);
}

/** Read bytes directly from the device (bypass cache)
 * 
 * @param service_id	Service ID of the block device.
 * @param abs_offset	Absolute offset in bytes where to start reading
 * @param bytes			Number of bytes to read
 * @param data			Buffer that receives the data
 * 
 * @return		EOK on success or an error code on failure.
 */
errno_t block_read_bytes_direct(service_id_t service_id, aoff64_t abs_offset,
    size_t bytes, void *data)
{
	errno_t rc;
	size_t phys_block_size;
	size_t buf_size;
	void *buffer;
	aoff64_t first_block;
	aoff64_t last_block;
	size_t blocks;
	size_t offset;
	
	rc = block_get_bsize(service_id, &phys_block_size);
	if (rc != EOK) {
		return rc;
	}
	
	/* calculate data position and required space */
	first_block = abs_offset / phys_block_size;
	offset = abs_offset % phys_block_size;
	last_block = (abs_offset + bytes - 1) / phys_block_size;
	blocks = last_block - first_block + 1;
	buf_size = blocks * phys_block_size;
	
	/* read the data into memory */
	buffer = malloc(buf_size);
	if (buffer == NULL) {
		return ENOMEM;
	}
	
	rc = block_read_direct(service_id, first_block, blocks, buffer);
	if (rc != EOK) {
		free(buffer);
		return rc;
	}
	
	/* copy the data from the buffer */
	memcpy(data, buffer + offset, bytes);
	free(buffer);
	
	return EOK;
}

/** Get TOC from device.
 *
 * @param service_id Service ID of the block device.
 * @param session    Starting session.
 *
 * @return Allocated TOC structure.
 * @return EOK on success or an error code.
 *
 */
errno_t block_read_toc(service_id_t service_id, uint8_t session, void *buf,
    size_t bufsize)
{
	devcon_t *devcon = devcon_search(service_id);
	
	assert(devcon);
	return bd_read_toc(devcon->bd, session, buf, bufsize);
}

/** Read blocks from block device.
 *
 * @param devcon	Device connection.
 * @param ba		Address of first block.
 * @param cnt		Number of blocks.
 * @param src		Buffer for storing the data.
 *
 * @return		EOK on success or an error code on failure.
 */
static errno_t read_blocks(devcon_t *devcon, aoff64_t ba, size_t cnt, void *buf,
    size_t size)
{
	assert(devcon);
	
	errno_t rc = bd_read_blocks(devcon->bd, ba, cnt, buf, size);
	if (rc != EOK) {
		printf("Error %s reading %zu blocks starting at block %" PRIuOFF64
		    " from device handle %" PRIun "\n", str_error_name(rc), cnt, ba,
		    devcon->service_id);
#ifndef NDEBUG
		stacktrace_print();
#endif
	}
	
	return rc;
}

/** Write block to block device.
 *
 * @param devcon	Device connection.
 * @param ba		Address of first block.
 * @param cnt		Number of blocks.
 * @param src		Buffer containing the data to write.
 *
 * @return		EOK on success or an error code on failure.
 */
static errno_t write_blocks(devcon_t *devcon, aoff64_t ba, size_t cnt, void *data,
    size_t size)
{
	assert(devcon);
	
	errno_t rc = bd_write_blocks(devcon->bd, ba, cnt, data, size);
	if (rc != EOK) {
		printf("Error %s writing %zu blocks starting at block %" PRIuOFF64
		    " to device handle %" PRIun "\n", str_error_name(rc), cnt, ba, devcon->service_id);
#ifndef NDEBUG
		stacktrace_print();
#endif
	}
	
	return rc;
}

/** Convert logical block address to physical block address. */
static aoff64_t ba_ltop(devcon_t *devcon, aoff64_t lba)
{
	assert(devcon->cache != NULL);
	return lba * devcon->cache->blocks_cluster;
}

/** @}
 */
