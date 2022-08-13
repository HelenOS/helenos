/*
 * SPDX-FileCopyrightText: 2008 Jakub Jermar
 * SPDX-FileCopyrightText: 2008 Martin Decky
 * SPDX-FileCopyrightText: 2011 Martin Sucha
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup libblock
 * @{
 */
/**
 * @file
 */

#ifndef LIBBLOCK_LIBBLOCK_H_
#define LIBBLOCK_LIBBLOCK_H_

#include <offset.h>
#include <async.h>
#include <fibril_synch.h>
#include <adt/hash_table.h>
#include <adt/list.h>
#include <loc.h>

/*
 * Flags that can be used with block_get().
 */

/**
 * This macro is a symbolic value for situations where no special flags are
 * needed.
 */
#define BLOCK_FLAGS_NONE	0

/**
 * When the client of block_get() intends to overwrite the current contents of
 * the block, this flag is used to avoid the unnecessary read.
 */
#define BLOCK_FLAGS_NOREAD	1

typedef struct block {
	/** Mutex protecting the reference count. */
	fibril_mutex_t lock;
	/** Number of references to the block_t structure. */
	unsigned refcnt;
	/** If true, the block needs to be written back to the block device. */
	bool dirty;
	/** If true, the blcok does not contain valid data. */
	bool toxic;
	/** Readers / Writer lock protecting the contents of the block. */
	fibril_rwlock_t contents_lock;
	/** Service ID of service providing the block device. */
	service_id_t service_id;
	/** Logical block address */
	aoff64_t lba;
	/** Physical block address */
	aoff64_t pba;
	/** Size of the block. */
	size_t size;
	/** Number of write failures. */
	int write_failures;
	/** Link for placing the block into the free block list. */
	link_t free_link;
	/** Link for placing the block into the block hash table. */
	ht_link_t hash_link;
	/** Buffer with the block data. */
	void *data;
} block_t;

/** Caching mode */
enum cache_mode {
	/** Write-Through */
	CACHE_MODE_WT,
	/** Write-Back */
	CACHE_MODE_WB
};

extern errno_t block_init(service_id_t, size_t);
extern void block_fini(service_id_t);

extern errno_t block_bb_read(service_id_t, aoff64_t);
extern void *block_bb_get(service_id_t);

extern errno_t block_cache_init(service_id_t, size_t, unsigned, enum cache_mode);
extern errno_t block_cache_fini(service_id_t);

extern errno_t block_get(block_t **, service_id_t, aoff64_t, int);
extern errno_t block_put(block_t *);

extern errno_t block_seqread(service_id_t, void *, size_t *, size_t *, aoff64_t *,
    void *, size_t);

extern errno_t block_get_bsize(service_id_t, size_t *);
extern errno_t block_get_nblocks(service_id_t, aoff64_t *);
extern errno_t block_read_toc(service_id_t, uint8_t, void *, size_t);
extern errno_t block_read_direct(service_id_t, aoff64_t, size_t, void *);
extern errno_t block_read_bytes_direct(service_id_t, aoff64_t, size_t, void *);
extern errno_t block_write_direct(service_id_t, aoff64_t, size_t, const void *);
extern errno_t block_sync_cache(service_id_t, aoff64_t, size_t);

#endif

/** @}
 */
