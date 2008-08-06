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
#include <ipc/ipc.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <as.h>
#include <libfs.h>
#include <ipc/services.h>
#include <ipc/devmap.h>
#include <sys/mman.h>
#include <byteorder.h>

#define BLOCK_SIZE			1024	// FIXME
#define RD_BASE				1024	// FIXME
#define	RD_READ_BLOCK	(RD_BASE + 1)

struct rdentry {
	uint8_t type;
	uint32_t len;
} __attribute__((packed));

static bool 
tmpfs_blockread(int phone, void *buffer, size_t *bufpos, size_t *buflen,
    size_t *pos, void *dst, size_t size)
{
	size_t offset = 0;
	size_t left = size;
	
	while (left > 0) {
		size_t rd;
		
		if (*bufpos + left < *buflen)
			rd = left;
		else
			rd = *buflen - *bufpos;
		
		if (rd > 0) {
			memcpy(dst + offset, buffer + *bufpos, rd);
			offset += rd;
			*bufpos += rd;
			*pos += rd;
			left -= rd;
		}
		
		if (*bufpos == *buflen) {
			ipcarg_t retval;
			int rc = ipc_call_sync_2_1(phone, RD_READ_BLOCK,
			    *pos / BLOCK_SIZE, BLOCK_SIZE,
			    &retval);
			if ((rc != EOK) || (retval != EOK))
				return false;
			
			*bufpos = 0;
			*buflen = BLOCK_SIZE;
		}
	}
	
	return true;
}

static bool
tmpfs_restore_recursion(int phone, void *block, size_t *bufpos, size_t *buflen,
    size_t *pos, tmpfs_dentry_t *parent)
{
	struct rdentry entry;
	libfs_ops_t *ops = &tmpfs_libfs_ops;
	
	do {
		char *fname;
		tmpfs_dentry_t *node;
		uint32_t size;
		
		if (!tmpfs_blockread(phone, block, bufpos, buflen, pos, &entry,
		    sizeof(entry)))
			return false;
		
		entry.len = uint32_t_le2host(entry.len);
		
		switch (entry.type) {
		case TMPFS_NONE:
			break;
		case TMPFS_FILE:
			fname = malloc(entry.len + 1);
			if (fname == NULL)
				return false;
			
			node = (tmpfs_dentry_t *) ops->create(L_FILE);
			if (node == NULL) {
				free(fname);
				return false;
			}
			
			if (!tmpfs_blockread(phone, block, bufpos, buflen, pos,
			    fname, entry.len)) {
				ops->destroy((void *) node);
				free(fname);
				return false;
			}
			fname[entry.len] = 0;
			
			if (!ops->link((void *) parent, (void *) node, fname)) {
				ops->destroy((void *) node);
				free(fname);
				return false;
			}
			free(fname);
			
			if (!tmpfs_blockread(phone, block, bufpos, buflen, pos,
			    &size, sizeof(size)))
				return false;
			
			size = uint32_t_le2host(size);
			
			node->data = malloc(size);
			if (node->data == NULL)
				return false;
			
			node->size = size;
			if (!tmpfs_blockread(phone, block, bufpos, buflen, pos,
			    node->data, size))
				return false;
			
			break;
		case TMPFS_DIRECTORY:
			fname = malloc(entry.len + 1);
			if (fname == NULL)
				return false;
			
			node = (tmpfs_dentry_t *) ops->create(L_DIRECTORY);
			if (node == NULL) {
				free(fname);
				return false;
			}
			
			if (!tmpfs_blockread(phone, block, bufpos, buflen, pos,
			    fname, entry.len)) {
				ops->destroy((void *) node);
				free(fname);
				return false;
			}
			fname[entry.len] = 0;
			
			if (!ops->link((void *) parent, (void *) node, fname)) {
				ops->destroy((void *) node);
				free(fname);
				return false;
			}
			free(fname);
			
			if (!tmpfs_restore_recursion(phone, block, bufpos,
			    buflen, pos, node))
				return false;
			
			break;
		default:
			return false;
		}
	} while (entry.type != TMPFS_NONE);
	
	return true;
}

bool tmpfs_restore(dev_handle_t dev)
{
	libfs_ops_t *ops = &tmpfs_libfs_ops;

	void *block = mmap(NULL, BLOCK_SIZE,
	    PROTO_READ | PROTO_WRITE, MAP_ANONYMOUS | MAP_PRIVATE, 0, 0);
	
	if (block == NULL)
		return false;
	
	int phone = ipc_connect_me_to(PHONE_NS, SERVICE_DEVMAP,
	    DEVMAP_CONNECT_TO_DEVICE, dev);

	if (phone < 0) {
		munmap(block, BLOCK_SIZE);
		return false;
	}
	
	if (ipc_share_out_start(phone, block, AS_AREA_READ | AS_AREA_WRITE) !=
	    EOK)
		goto error;
	
	size_t bufpos = 0;
	size_t buflen = 0;
	size_t pos = 0;
	
	char tag[6];
	if (!tmpfs_blockread(phone, block, &bufpos, &buflen, &pos, tag, 5))
		goto error;
	
	tag[5] = 0;
	if (strcmp(tag, "TMPFS") != 0)
		goto error;
	
	if (!tmpfs_restore_recursion(phone, block, &bufpos, &buflen, &pos,
	    ops->root_get(dev)))
		goto error;
		
	ipc_hangup(phone);
	munmap(block, BLOCK_SIZE);
	return true;
	
error:
	ipc_hangup(phone);
	munmap(block, BLOCK_SIZE);
	return false;
}

/**
 * @}
 */ 
