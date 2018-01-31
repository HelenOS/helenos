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

/** @addtogroup fs
 * @{
 */ 

/**
 * @file	tmpfs_dump.c
 * @brief	Support for loading TMPFS file system dump.
 */

#include "tmpfs.h"
#include "../../vfs/vfs.h"
#include <errno.h>
#include <stdlib.h>
#include <str.h>
#include <stddef.h>
#include <stdint.h>
#include <as.h>
#include <block.h>
#include <byteorder.h>

#define TMPFS_COMM_SIZE		1024

static uint8_t tmpfs_buf[TMPFS_COMM_SIZE];

struct rdentry {
	uint8_t type;
	uint32_t len;
} __attribute__((packed));

static bool
tmpfs_restore_recursion(service_id_t dsid, size_t *bufpos, size_t *buflen,
    aoff64_t *pos, fs_node_t *pfn)
{
	struct rdentry entry;
	libfs_ops_t *ops = &tmpfs_libfs_ops;
	errno_t rc;
	
	do {
		char *fname;
		fs_node_t *fn;
		tmpfs_node_t *nodep;
		uint32_t size;
		
		if (block_seqread(dsid, tmpfs_buf, bufpos, buflen, pos, &entry,
		    sizeof(entry)) != EOK)
			return false;
		
		entry.len = uint32_t_le2host(entry.len);
		
		switch (entry.type) {
		case TMPFS_NONE:
			break;
		case TMPFS_FILE:
			fname = malloc(entry.len + 1);
			if (fname == NULL)
				return false;
			
			rc = ops->create(&fn, dsid, L_FILE);
			if (rc != EOK || fn == NULL) {
				free(fname);
				return false;
			}
			
			if (block_seqread(dsid, tmpfs_buf, bufpos, buflen, pos, fname,
			    entry.len) != EOK) {
				(void) ops->destroy(fn);
				free(fname);
				return false;
			}
			fname[entry.len] = 0;
			
			rc = ops->link(pfn, fn, fname);
			if (rc != EOK) {
				(void) ops->destroy(fn);
				free(fname);
				return false;
			}
			free(fname);
			
			if (block_seqread(dsid, tmpfs_buf, bufpos, buflen, pos, &size,
			    sizeof(size)) != EOK)
				return false;
			
			size = uint32_t_le2host(size);
			
			nodep = TMPFS_NODE(fn);
			nodep->data = malloc(size);
			if (nodep->data == NULL)
				return false;
			
			nodep->size = size;
			if (block_seqread(dsid, tmpfs_buf, bufpos, buflen, pos, nodep->data,
			    size) != EOK)
				return false;
			
			break;
		case TMPFS_DIRECTORY:
			fname = malloc(entry.len + 1);
			if (fname == NULL)
				return false;
			
			rc = ops->create(&fn, dsid, L_DIRECTORY);
			if (rc != EOK || fn == NULL) {
				free(fname);
				return false;
			}
			
			if (block_seqread(dsid, tmpfs_buf, bufpos, buflen, pos, fname,
			    entry.len) != EOK) {
				(void) ops->destroy(fn);
				free(fname);
				return false;
			}
			fname[entry.len] = 0;

			rc = ops->link(pfn, fn, fname);
			if (rc != EOK) {
				(void) ops->destroy(fn);
				free(fname);
				return false;
			}
			free(fname);
			
			if (!tmpfs_restore_recursion(dsid, bufpos, buflen, pos,
			    fn))
				return false;
			
			break;
		default:
			return false;
		}
	} while (entry.type != TMPFS_NONE);
	
	return true;
}

bool tmpfs_restore(service_id_t dsid)
{
	libfs_ops_t *ops = &tmpfs_libfs_ops;
	fs_node_t *fn;
	errno_t rc;

	rc = block_init(dsid, TMPFS_COMM_SIZE);
	if (rc != EOK)
		return false; 
	
	size_t bufpos = 0;
	size_t buflen = 0;
	aoff64_t pos = 0;
	
	char tag[6];
	if (block_seqread(dsid, tmpfs_buf, &bufpos, &buflen, &pos, tag, 5) != EOK)
		goto error;
	
	tag[5] = 0;
	if (str_cmp(tag, "TMPFS") != 0)
		goto error;
	
	rc = ops->root_get(&fn, dsid);
	if (rc != EOK)
		goto error;

	if (!tmpfs_restore_recursion(dsid, &bufpos, &buflen, &pos, fn))
		goto error;
		
	block_fini(dsid);
	return true;
	
error:
	block_fini(dsid);
	return false;
}

/**
 * @}
 */ 
