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
 * @file	fat_ops.c
 * @brief	Implementation of VFS operations for the FAT file system server.
 */

#include "fat.h"
#include "../../vfs/vfs.h"
#include <libfs.h>
#include <ipc/ipc.h>
#include <async.h>
#include <errno.h>
#include <string.h>
#include <byteorder.h>

#define FAT_NAME_LEN		8
#define FAT_EXT_LEN		3

#define FAT_PAD			' ' 

#define FAT_DENTRY_UNUSED	0x00
#define FAT_DENTRY_E5_ESC	0x05
#define FAT_DENTRY_DOT		0x2e
#define FAT_DENTRY_ERASED	0xe5

static void dentry_name_canonify(fat_dentry_t *d, char *buf)
{
	int i;

	for (i = 0; i < FAT_NAME_LEN; i++) {
		if (d->name[i] == FAT_PAD) {
			buf++;
			break;
		}
		if (d->name[i] == FAT_DENTRY_E5_ESC)
			*buf++ = 0xe5;
		else
			*buf++ = d->name[i];
	}
	if (d->ext[0] != FAT_PAD)
		*buf++ = '.';
	for (i = 0; i < FAT_EXT_LEN; i++) {
		if (d->ext[i] == FAT_PAD) {
			*buf = '\0';
			return;
		}
		if (d->ext[i] == FAT_DENTRY_E5_ESC)
			*buf++ = 0xe5;
		else
			*buf++ = d->ext[i];
	}
}

/* TODO and also move somewhere else */
typedef struct {
	void *data;
} block_t;

static block_t *block_get(dev_handle_t dev_handle, off_t offset)
{
	return NULL;	/* TODO */
}

static block_t *fat_block_get(fat_node_t *node, off_t offset) {
	return NULL;	/* TODO */
}

static void block_put(block_t *block)
{
	/* TODO */
}

static void *fat_node_get(dev_handle_t dev_handle, fs_index_t index)
{
	return NULL;	/* TODO */
}

#define BS_BLOCK	0

static void *fat_match(void *prnt, const char *component)
{
	fat_node_t *parentp = (fat_node_t *)prnt;
	char name[FAT_NAME_LEN + 1 + FAT_EXT_LEN + 1];
	unsigned i, j;
	unsigned bps;		/* bytes per sector */
	unsigned dps;		/* dentries per sector */
	unsigned blocks;
	fat_dentry_t *d;
	block_t *bb;
	block_t *b;

	bb = block_get(parentp->dev_handle, BS_BLOCK);
	if (!bb)
		return NULL;
	bps = uint16_t_le2host(((fat_bs_t *)bb->data)->bps);
	block_put(bb);
	dps = bps / sizeof(fat_dentry_t);
	blocks = parentp->size / bps + (parentp->size % bps != 0);
	for (i = 0; i < blocks; i++) {
		unsigned dentries;
		
		b = fat_block_get(parentp, i);
		if (!b) 
			return NULL;

		dentries = (i == blocks - 1) ?
		    parentp->size % sizeof(fat_dentry_t) :
		    dps;
		for (j = 0; j < dentries; j++) { 
			d = ((fat_dentry_t *)b->data) + j;
			if (d->attr & FAT_ATTR_VOLLABEL) {
				/* volume label entry */
				continue;
			}
			if (d->name[0] == FAT_DENTRY_ERASED) {
				/* not-currently-used entry */
				continue;
			}
			if (d->name[0] == FAT_DENTRY_UNUSED) {
				/* never used entry */
				block_put(b);
				return NULL;
			}
			if (d->name[0] == FAT_DENTRY_DOT) {
				/*
				 * Most likely '.' or '..'.
				 * It cannot occur in a regular file name.
				 */
				continue;
			}
		
			dentry_name_canonify(d, name);
			if (strcmp(name, component) == 0) {
				/* hit */
				void *node = fat_node_get(parentp->dev_handle,
				    (fs_index_t)uint16_t_le2host(d->firstc));
				block_put(b);
				return node;
			}
		}
		block_put(b);
	}

	return NULL;
}

/** libfs operations */
libfs_ops_t fat_libfs_ops = {
	.match = fat_match,
	.node_get = fat_node_get,
	.create = NULL,
	.destroy = NULL,
	.link = NULL,
	.unlink = NULL,
	.index_get = NULL,
	.size_get = NULL,
	.lnkcnt_get = NULL,
	.has_children = NULL,
	.root_get = NULL,
	.plb_get_char =	NULL,
	.is_directory = NULL,
	.is_file = NULL
};

void fat_lookup(ipc_callid_t rid, ipc_call_t *request)
{
	libfs_lookup(&fat_libfs_ops, fat_reg.fs_handle, rid, request);
}

/**
 * @}
 */ 
