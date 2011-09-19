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

#include "libblock.h"
#include "../../srv/vfs/vfs.h"
#include <ipc/loc.h>
#include <ipc/bd.h>
#include <ipc/services.h>
#include <errno.h>
#include <sys/mman.h>
#include <async.h>
#include <as.h>
#include <assert.h>
#include <fibril_synch.h>
#include <adt/list.h>
#include <adt/hash_table.h>
#include <macros.h>
#include <mem.h>
#include <malloc.h>
#include <stdio.h>
#include <sys/typefmt.h>
#include <stacktrace.h>

/** Lock protecting the device connection list */
static FIBRIL_MUTEX_INITIALIZE(dcl_lock);
/** Device connection list head. */
static LIST_INITIALIZE(dcl);

#define CACHE_BUCKETS_LOG2  10
#define CACHE_BUCKETS       (1 << CACHE_BUCKETS_LOG2)

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
	fibril_mutex_t comm_area_lock;
	void *comm_area;
	size_t comm_size;
	void *bb_buf;
	aoff64_t bb_addr;
	size_t pblock_size;  /**< Physical block size. */
	cache_t *cache;
} devcon_t;

static int read_blocks(devcon_t *, aoff64_t, size_t);
static int write_blocks(devcon_t *, aoff64_t, size_t);
static int get_block_size(async_sess_t *, size_t *);
static int get_num_blocks(async_sess_t *, aoff64_t *);
static aoff64_t ba_ltop(devcon_t *, aoff64_t);

static devcon_t *devcon_search(service_id_t service_id)
{
	fibril_mutex_lock(&dcl_lock);
	
	list_foreach(dcl, cur) {
		devcon_t *devcon = list_get_instance(cur, devcon_t, link);
		if (devcon->service_id == service_id) {
			fibril_mutex_unlock(&dcl_lock);
			return devcon;
		}
	}
	
	fibril_mutex_unlock(&dcl_lock);
	return NULL;
}

static int devcon_add(service_id_t service_id, async_sess_t *sess,
    size_t bsize, void *comm_area, size_t comm_size)
{
	devcon_t *devcon;
	
	if (comm_size < bsize)
		return EINVAL;
	
	devcon = malloc(sizeof(devcon_t));
	if (!devcon)
		return ENOMEM;
	
	link_initialize(&devcon->link);
	devcon->service_id = service_id;
	devcon->sess = sess;
	fibril_mutex_initialize(&devcon->comm_area_lock);
	devcon->comm_area = comm_area;
	devcon->comm_size = comm_size;
	devcon->bb_buf = NULL;
	devcon->bb_addr = 0;
	devcon->pblock_size = bsize;
	devcon->cache = NULL;
	
	fibril_mutex_lock(&dcl_lock);
	list_foreach(dcl, cur) {
		devcon_t *d = list_get_instance(cur, devcon_t, link);
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

int block_init(exch_mgmt_t mgmt, service_id_t service_id,
    size_t comm_size)
{
	void *comm_area = mmap(NULL, comm_size, PROTO_READ | PROTO_WRITE,
	    MAP_ANONYMOUS | MAP_PRIVATE, 0, 0);
	if (!comm_area)
		return ENOMEM;
	
	async_sess_t *sess = loc_service_connect(mgmt, service_id,
	    IPC_FLAG_BLOCKING);
	if (!sess) {
		munmap(comm_area, comm_size);
		return ENOENT;
	}
	
	async_exch_t *exch = async_exchange_begin(sess);
	int rc = async_share_out_start(exch, comm_area,
	    AS_AREA_READ | AS_AREA_WRITE);
	async_exchange_end(exch);
	
	if (rc != EOK) {
		munmap(comm_area, comm_size);
		async_hangup(sess);
		return rc;
	}
	
	size_t bsize;
	rc = get_block_size(sess, &bsize);
	
	if (rc != EOK) {
		munmap(comm_area, comm_size);
		async_hangup(sess);
		return rc;
	}
	
	rc = devcon_add(service_id, sess, bsize, comm_area, comm_size);
	if (rc != EOK) {
		munmap(comm_area, comm_size);
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
	
	devcon_remove(devcon);
	
	if (devcon->bb_buf)
		free(devcon->bb_buf);
	
	munmap(devcon->comm_area, devcon->comm_size);
	async_hangup(devcon->sess);
	
	free(devcon);
}

int block_bb_read(service_id_t service_id, aoff64_t ba)
{
	void *bb_buf;
	int rc;

	devcon_t *devcon = devcon_search(service_id);
	if (!devcon)
		return ENOENT;
	if (devcon->bb_buf)
		return EEXIST;
	bb_buf = malloc(devcon->pblock_size);
	if (!bb_buf)
		return ENOMEM;

	fibril_mutex_lock(&devcon->comm_area_lock);
	rc = read_blocks(devcon, 0, 1);
	if (rc != EOK) {
		fibril_mutex_unlock(&devcon->comm_area_lock);
	    	free(bb_buf);
		return rc;
	}
	memcpy(bb_buf, devcon->comm_area, devcon->pblock_size);
	fibril_mutex_unlock(&devcon->comm_area_lock);

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

static hash_index_t cache_hash(unsigned long *key)
{
	return MERGE_LOUP32(key[0], key[1]) & (CACHE_BUCKETS - 1);
}

static int cache_compare(unsigned long *key, hash_count_t keys, link_t *item)
{
	block_t *b = hash_table_get_instance(item, block_t, hash_link);
	return b->lba == MERGE_LOUP32(key[0], key[1]);
}

static void cache_remove_callback(link_t *item)
{
}

static hash_table_operations_t cache_ops = {
	.hash = cache_hash,
	.compare = cache_compare,
	.remove_callback = cache_remove_callback
};

int block_cache_init(service_id_t service_id, size_t size, unsigned blocks,
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

	if (!hash_table_create(&cache->block_hash, CACHE_BUCKETS, 2,
	    &cache_ops)) {
		free(cache);
		return ENOMEM;
	}

	devcon->cache = cache;
	return EOK;
}

int block_cache_fini(service_id_t service_id)
{
	devcon_t *devcon = devcon_search(service_id);
	cache_t *cache;
	int rc;

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
			memcpy(devcon->comm_area, b->data, b->size);
			rc = write_blocks(devcon, b->pba, cache->blocks_cluster);
			if (rc != EOK)
				return rc;
		}

		unsigned long key[2] = {
			LOWER32(b->lba),
			UPPER32(b->lba)
		};
		hash_table_remove(&cache->block_hash, key, 2);
		
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
	b->dirty = false;
	b->toxic = false;
	fibril_rwlock_initialize(&b->contents_lock);
	link_initialize(&b->free_link);
	link_initialize(&b->hash_link);
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
 * @return			EOK on success or a negative error code.
 */
int block_get(block_t **block, service_id_t service_id, aoff64_t ba, int flags)
{
	devcon_t *devcon;
	cache_t *cache;
	block_t *b;
	link_t *l;
	unsigned long key[2] = {
		LOWER32(ba),
		UPPER32(ba)
	};

	int rc;
	
	devcon = devcon_search(service_id);

	assert(devcon);
	assert(devcon->cache);
	
	cache = devcon->cache;

retry:
	rc = EOK;
	b = NULL;

	fibril_mutex_lock(&cache->lock);
	l = hash_table_find(&cache->block_hash, key);
	if (l) {
found:
		/*
		 * We found the block in the cache.
		 */
		b = hash_table_get_instance(l, block_t, hash_link);
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
			l = list_first(&cache->free_list);
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
				list_append(&b->free_link, &cache->free_list);
				fibril_mutex_unlock(&cache->lock);
				fibril_mutex_lock(&devcon->comm_area_lock);
				memcpy(devcon->comm_area, b->data, b->size);
				rc = write_blocks(devcon, b->pba,
				    cache->blocks_cluster);
				fibril_mutex_unlock(&devcon->comm_area_lock);
				if (rc != EOK) {
					/*
					 * We did not manage to write the block
					 * to the device. Keep it around for
					 * another try. Hopefully, we will grab
					 * another block next time.
					 */
					fibril_mutex_unlock(&b->lock);
					goto retry;
				}
				b->dirty = false;
				if (!fibril_mutex_trylock(&cache->lock)) {
					/*
					 * Somebody is probably racing with us.
					 * Unlock the block and retry.
					 */
					fibril_mutex_unlock(&b->lock);
					goto retry;
				}
				l = hash_table_find(&cache->block_hash, key);
				if (l) {
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
			unsigned long temp_key[2] = {
				LOWER32(b->lba),
				UPPER32(b->lba)
			};
			hash_table_remove(&cache->block_hash, temp_key, 2);
		}

		block_initialize(b);
		b->service_id = service_id;
		b->size = cache->lblock_size;
		b->lba = ba;
		b->pba = ba_ltop(devcon, b->lba);
		hash_table_insert(&cache->block_hash, key, &b->hash_link);

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
			fibril_mutex_lock(&devcon->comm_area_lock);
			rc = read_blocks(devcon, b->pba, cache->blocks_cluster);
			memcpy(b->data, devcon->comm_area, cache->lblock_size);
			fibril_mutex_unlock(&devcon->comm_area_lock);
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
 * @return		EOK on success or a negative error code.
 */
int block_put(block_t *block)
{
	devcon_t *devcon = devcon_search(block->service_id);
	cache_t *cache;
	unsigned blocks_cached;
	enum cache_mode mode;
	int rc = EOK;

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
		fibril_mutex_lock(&devcon->comm_area_lock);
		memcpy(devcon->comm_area, block->data, block->size);
		rc = write_blocks(devcon, block->pba, cache->blocks_cluster);
		fibril_mutex_unlock(&devcon->comm_area_lock);
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
				fibril_mutex_unlock(&block->lock);
				fibril_mutex_unlock(&cache->lock);
				goto retry;
			}
			/*
			 * Take the block out of the cache and free it.
			 */
			unsigned long key[2] = {
				LOWER32(block->lba),
				UPPER32(block->lba)
			};
			hash_table_remove(&cache->block_hash, key, 2);
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
int block_seqread(service_id_t service_id, size_t *bufpos, size_t *buflen,
    aoff64_t *pos, void *dst, size_t size)
{
	size_t offset = 0;
	size_t left = size;
	size_t block_size;
	devcon_t *devcon;

	devcon = devcon_search(service_id);
	assert(devcon);
	block_size = devcon->pblock_size;
	
	fibril_mutex_lock(&devcon->comm_area_lock);
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
			memcpy(dst + offset, devcon->comm_area + *bufpos, rd);
			offset += rd;
			*bufpos += rd;
			*pos += rd;
			left -= rd;
		}
		
		if (*bufpos == *buflen) {
			/* Refill the communication buffer with a new block. */
			int rc;

			rc = read_blocks(devcon, *pos / block_size, 1);
			if (rc != EOK) {
				fibril_mutex_unlock(&devcon->comm_area_lock);
				return rc;
			}
			
			*bufpos = 0;
			*buflen = block_size;
		}
	}
	fibril_mutex_unlock(&devcon->comm_area_lock);
	
	return EOK;
}

/** Read blocks directly from device (bypass cache).
 *
 * @param service_id	Service ID of the block device.
 * @param ba		Address of first block (physical).
 * @param cnt		Number of blocks.
 * @param src		Buffer for storing the data.
 *
 * @return		EOK on success or negative error code on failure.
 */
int block_read_direct(service_id_t service_id, aoff64_t ba, size_t cnt, void *buf)
{
	devcon_t *devcon;
	int rc;

	devcon = devcon_search(service_id);
	assert(devcon);
	
	fibril_mutex_lock(&devcon->comm_area_lock);

	rc = read_blocks(devcon, ba, cnt);
	if (rc == EOK)
		memcpy(buf, devcon->comm_area, devcon->pblock_size * cnt);

	fibril_mutex_unlock(&devcon->comm_area_lock);

	return rc;
}

/** Write blocks directly to device (bypass cache).
 *
 * @param service_id	Service ID of the block device.
 * @param ba		Address of first block (physical).
 * @param cnt		Number of blocks.
 * @param src		The data to be written.
 *
 * @return		EOK on success or negative error code on failure.
 */
int block_write_direct(service_id_t service_id, aoff64_t ba, size_t cnt,
    const void *data)
{
	devcon_t *devcon;
	int rc;

	devcon = devcon_search(service_id);
	assert(devcon);
	
	fibril_mutex_lock(&devcon->comm_area_lock);

	memcpy(devcon->comm_area, data, devcon->pblock_size * cnt);
	rc = write_blocks(devcon, ba, cnt);

	fibril_mutex_unlock(&devcon->comm_area_lock);

	return rc;
}

/** Get device block size.
 *
 * @param service_id	Service ID of the block device.
 * @param bsize		Output block size.
 *
 * @return		EOK on success or negative error code on failure.
 */
int block_get_bsize(service_id_t service_id, size_t *bsize)
{
	devcon_t *devcon;

	devcon = devcon_search(service_id);
	assert(devcon);
	
	return get_block_size(devcon->sess, bsize);
}

/** Get number of blocks on device.
 *
 * @param service_id	Service ID of the block device.
 * @param nblocks	Output number of blocks.
 *
 * @return		EOK on success or negative error code on failure.
 */
int block_get_nblocks(service_id_t service_id, aoff64_t *nblocks)
{
	devcon_t *devcon = devcon_search(service_id);
	assert(devcon);
	
	return get_num_blocks(devcon->sess, nblocks);
}

/** Read bytes directly from the device (bypass cache)
 * 
 * @param service_id	Service ID of the block device.
 * @param abs_offset	Absolute offset in bytes where to start reading
 * @param bytes			Number of bytes to read
 * @param data			Buffer that receives the data
 * 
 * @return		EOK on success or negative error code on failure.
 */
int block_read_bytes_direct(service_id_t service_id, aoff64_t abs_offset,
    size_t bytes, void *data)
{
	int rc;
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
 * @return NULL on failure.
 *
 */
toc_block_t *block_get_toc(service_id_t service_id, uint8_t session)
{
	devcon_t *devcon = devcon_search(service_id);
	assert(devcon);
	
	toc_block_t *toc = NULL;
	
	fibril_mutex_lock(&devcon->comm_area_lock);
	
	async_exch_t *exch = async_exchange_begin(devcon->sess);
	int rc = async_req_1_0(exch, BD_READ_TOC, session);
	async_exchange_end(exch);
	
	if (rc == EOK) {
		toc = (toc_block_t *) malloc(sizeof(toc_block_t));
		if (toc != NULL) {
			memset(toc, 0, sizeof(toc_block_t));
			memcpy(toc, devcon->comm_area,
			    min(devcon->pblock_size, sizeof(toc_block_t)));
		}
	}
	
	
	fibril_mutex_unlock(&devcon->comm_area_lock);
	
	return toc;
}

/** Read blocks from block device.
 *
 * @param devcon	Device connection.
 * @param ba		Address of first block.
 * @param cnt		Number of blocks.
 * @param src		Buffer for storing the data.
 *
 * @return		EOK on success or negative error code on failure.
 */
static int read_blocks(devcon_t *devcon, aoff64_t ba, size_t cnt)
{
	assert(devcon);
	
	async_exch_t *exch = async_exchange_begin(devcon->sess);
	int rc = async_req_3_0(exch, BD_READ_BLOCKS, LOWER32(ba),
	    UPPER32(ba), cnt);
	async_exchange_end(exch);
	
	if (rc != EOK) {
		printf("Error %d reading %zu blocks starting at block %" PRIuOFF64
		    " from device handle %" PRIun "\n", rc, cnt, ba,
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
 * @return		EOK on success or negative error code on failure.
 */
static int write_blocks(devcon_t *devcon, aoff64_t ba, size_t cnt)
{
	assert(devcon);
	
	async_exch_t *exch = async_exchange_begin(devcon->sess);
	int rc = async_req_3_0(exch, BD_WRITE_BLOCKS, LOWER32(ba),
	    UPPER32(ba), cnt);
	async_exchange_end(exch);
	
	if (rc != EOK) {
		printf("Error %d writing %zu blocks starting at block %" PRIuOFF64
		    " to device handle %" PRIun "\n", rc, cnt, ba, devcon->service_id);
#ifndef NDEBUG
		stacktrace_print();
#endif
	}
	
	return rc;
}

/** Get block size used by the device. */
static int get_block_size(async_sess_t *sess, size_t *bsize)
{
	sysarg_t bs;
	
	async_exch_t *exch = async_exchange_begin(sess);
	int rc = async_req_0_1(exch, BD_GET_BLOCK_SIZE, &bs);
	async_exchange_end(exch);
	
	if (rc == EOK)
		*bsize = (size_t) bs;
	
	return rc;
}

/** Get total number of blocks on block device. */
static int get_num_blocks(async_sess_t *sess, aoff64_t *nblocks)
{
	sysarg_t nb_l;
	sysarg_t nb_h;
	
	async_exch_t *exch = async_exchange_begin(sess);
	int rc = async_req_0_2(exch, BD_GET_NUM_BLOCKS, &nb_l, &nb_h);
	async_exchange_end(exch);
	
	if (rc == EOK)
		*nblocks = (aoff64_t) MERGE_LOUP32(nb_l, nb_h);
	
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
