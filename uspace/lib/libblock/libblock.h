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
 */

#ifndef LIBBLOCK_LIBBLOCK_H_
#define	LIBBLOCK_LIBBLOCK_H_ 

#include <stdint.h>
#include "../../srv/vfs/vfs.h"
#include <futex.h>
#include <rwlock.h>
#include <libadt/hash_table.h>
#include <libadt/list.h>

typedef struct block {
	/** Futex protecting the reference count. */
	futex_t lock;
	/** Number of references to the block_t structure. */
	unsigned refcnt;
	/** If true, the block needs to be written back to the block device. */
	bool dirty;
	/** Readers / Writer lock protecting the contents of the block. */
	rwlock_t contents_lock;
	/** Handle of the device where the block resides. */
	dev_handle_t dev_handle;
	/** Block offset on the block device. Counted in 'size'-byte blocks. */
	off_t boff;
	/** Size of the block. */
	size_t size;
	/** Link for placing the block into the free block list. */
	link_t free_link;
	/** Link for placing the block into the block hash table. */ 
	link_t hash_link;
	/** Buffer with the block data. */
	void *data;
} block_t;

extern int block_init(dev_handle_t, size_t);
extern void block_fini(dev_handle_t);

extern int block_bb_read(dev_handle_t, off_t, size_t);
extern void *block_bb_get(dev_handle_t);

extern int block_cache_init(dev_handle_t, size_t, unsigned);

extern block_t *block_get(dev_handle_t, off_t);
extern void block_put(block_t *);

extern int block_read(int, off_t *, size_t *, off_t *, void *, size_t, size_t);

#endif

/** @}
 */

