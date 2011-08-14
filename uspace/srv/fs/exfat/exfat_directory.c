/*
 * Copyright (c) 2011 Oleg Romanenko
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
 * @file	exfat_directory.c
 * @brief	Functions that work with FAT directory.
 */

#include "exfat_directory.h"
#include "exfat_fat.h"
#include <libblock.h>
#include <errno.h>
#include <byteorder.h>
#include <mem.h>
#include <str.h>


void exfat_directory_init(exfat_directory_t *di)
{
	di->b = NULL;
	di->nodep = NULL;
	di->bs = NULL;
	di->blocks = 0;
	di->pos = 0;
	di->bnum = 0;
	di->last = false;
	di->fragmented = false;
	di->firstc = 0;
}

int exfat_directory_open(exfat_node_t *nodep, exfat_directory_t *di)
{
	exfat_directory_init(di);
	di->nodep = nodep;	
	if (di->nodep->type != EXFAT_DIRECTORY)
		return EINVAL;
	di->devmap_handle = nodep->idx->devmap_handle;
	di->fragmented = nodep->fragmented;
	di->firstc = nodep->firstc;

	di->bs = block_bb_get(di->devmap_handle);
	di->blocks = nodep->size / BPS(di->bs);
	return EOK;
}

int exfat_directory_open_parent(exfat_directory_t *di, 
    devmap_handle_t devmap_handle, exfat_cluster_t firstc, bool fragmented)
{
	exfat_directory_init(di);
	di->devmap_handle = devmap_handle;
	di->fragmented = fragmented;
	di->firstc = firstc;
	di->bs = block_bb_get(devmap_handle);
	di->blocks = 0;
	return EOK;
}

int exfat_directory_close(exfat_directory_t *di)
{
	int rc=EOK;
	
	if (di->b)
		rc = block_put(di->b);
	
	return rc;
}

static int exfat_directory_block_load(exfat_directory_t *di)
{
	uint32_t i;
	int rc;

	i = (di->pos * sizeof(exfat_dentry_t)) / BPS(di->bs);
	if (di->nodep && (i >= di->blocks))
		return ENOENT;

	if (di->b && di->bnum != i) {
		block_put(di->b);
		di->b = NULL;
	}
	if (!di->b) {
		if (di->nodep) {
			rc = exfat_block_get(&di->b, di->bs, di->nodep, i, BLOCK_FLAGS_NONE);
		} else {
			rc = exfat_block_get_by_clst(&di->b, di->bs, di->devmap_handle,
			    di->fragmented, di->firstc, NULL, i, BLOCK_FLAGS_NONE);
		}
		if (rc != EOK) {
			di->b = NULL;
			return rc;
		}
		di->bnum = i;
	}
	return EOK;
}

int exfat_directory_next(exfat_directory_t *di)
{
	int rc;

	di->pos += 1;
	rc = exfat_directory_block_load(di);
	if (rc!=EOK)
		di->pos -= 1;
	
	return rc;
}

int exfat_directory_prev(exfat_directory_t *di)
{
	int rc=EOK;
	
	if (di->pos > 0) {
		di->pos -= 1;
		rc=exfat_directory_block_load(di);
	}
	else
		return ENOENT;
	
	if (rc!=EOK)
		di->pos += 1;
	
	return rc;
}

int exfat_directory_seek(exfat_directory_t *di, aoff64_t pos)
{
	aoff64_t _pos = di->pos;
	int rc;

	di->pos = pos;
	rc = exfat_directory_block_load(di);
	if (rc!=EOK)
		di->pos = _pos;
	
	return rc;
}

int exfat_directory_get(exfat_directory_t *di, exfat_dentry_t **d)
{
	int rc;
	
	rc = exfat_directory_block_load(di);
	if (rc == EOK) {
		aoff64_t o = di->pos % (BPS(di->bs) / sizeof(exfat_dentry_t));
		*d = ((exfat_dentry_t *)di->b->data) + o;
	}
	
	return rc;
}

int exfat_directory_find(exfat_directory_t *di, exfat_dentry_clsf_t type, exfat_dentry_t **d)
{
	do {
		if (exfat_directory_get(di, d) == EOK) {
			if (exfat_classify_dentry(*d) == type)
				return EOK;
		} else
			return ENOENT;
	} while (exfat_directory_next(di) == EOK);
	
	return ENOENT;
}

int exfat_directory_find_continue(exfat_directory_t *di, exfat_dentry_clsf_t type, exfat_dentry_t **d)
{
	int rc;
	rc = exfat_directory_next(di);
	if (rc != EOK)
		return rc;
	return exfat_directory_find(di, type, d);
}


int exfat_directory_read_file(exfat_directory_t *di, char *name, size_t size, 
    exfat_file_dentry_t *df, exfat_stream_dentry_t *ds)
{
	uint16_t wname[EXFAT_FILENAME_LEN+1];
	exfat_dentry_t *d = NULL;
	int rc, i;
	size_t offset = 0;
	aoff64_t start_pos = 0;
	
	rc = exfat_directory_find(di, EXFAT_DENTRY_FILE, &d);
	if (rc != EOK)
		return rc;
	start_pos = di->pos;
	*df = d->file;

	rc = exfat_directory_next(di);
	if (rc != EOK)
		return rc;
	rc = exfat_directory_get(di, &d); 
	if (rc != EOK)
		return rc;
	if (exfat_classify_dentry(d) != EXFAT_DENTRY_STREAM)
		return ENOENT;
	*ds  = d->stream;
	
	if (ds->name_size > size)
		return EOVERFLOW;

	for (i=0; i<df->count-1; i++) {
		rc = exfat_directory_next(di);
		if (rc != EOK)
			return rc;
		rc = exfat_directory_get(di, &d); 
		if (rc != EOK)
			return rc;
		if (exfat_classify_dentry(d) != EXFAT_DENTRY_NAME)
			return ENOENT;
		exfat_dentry_get_name(&d->name, ds->name_size, wname, &offset);
	}
	rc = utf16_to_str(name, size, wname);
	if (rc != EOK)
		return rc;

	exfat_directory_seek(di, start_pos);
	return EOK;
}

int exfat_directory_write_file(exfat_directory_t *di, const char *name)
{
	/* TODO */
	return EOK;
}

int exfat_directory_erase_file(exfat_directory_t *di, aoff64_t pos)
{
	/* TODO */
	return EOK;
}


/**
 * @}
 */ 
