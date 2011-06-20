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
 * @file	fat_directory.c
 * @brief	Functions that work with FAT directory.
 */

#include "fat_directory.h"
#include <libblock.h>
#include <errno.h>
#include <byteorder.h>
#include <mem.h>

int fat_directory_block_load(fat_directory_t *);


int fat_directory_open(fat_node_t *nodep, fat_directory_t *di)
{
	di->b = NULL;
	di->nodep = nodep;	
	if (di->nodep->type != FAT_DIRECTORY)
		return EINVAL;

	di->bs = block_bb_get(di->nodep->idx->devmap_handle);
	di->blocks = di->nodep->size / BPS(di->bs);
	di->pos = 0;
	di->bnum = 0;
	di->last = false;

	di->lfn_utf16[0] = '\0';
	di->lfn_offset = 0;
	di->lfn_size = 0;
	di->long_entry = false;
	di->long_entry_count = 0;
	di->checksum=0;

	return EOK;
}

int fat_directory_close(fat_directory_t *di)
{
	int rc=EOK;
	
	if (di->b)
		rc = block_put(di->b);
	
	return rc;
}

int fat_directory_block_load(fat_directory_t *di)
{
	uint32_t i;
	int rc;

	i = (di->pos * sizeof(fat_dentry_t)) / BPS(di->bs);
	if (i < di->blocks) {
		if (di->b && di->bnum != i) {
			block_put(di->b);
			di->b = NULL;
		}
		if (!di->b) {
			rc = fat_block_get(&di->b, di->bs, di->nodep, i, BLOCK_FLAGS_NONE);
			if (rc != EOK) {
				di->b = NULL;
				return rc;
			}
			di->bnum = i;
		}
		return EOK;
	}
	return ENOENT;
}

int fat_directory_next(fat_directory_t *di)
{
	int rc;

	di->pos += 1;
	rc = fat_directory_block_load(di);
	if (rc!=EOK)
		di->pos -= 1;
	
	return rc;
}

int fat_directory_prev(fat_directory_t *di)
{
	int rc=EOK;
	
	if (di->pos > 0) {
		di->pos -= 1;
		rc=fat_directory_block_load(di);
	}
	else
		return ENOENT;
	
	if (rc!=EOK)
		di->pos += 1;
	
	return rc;
}

int fat_directory_seek(fat_directory_t *di, aoff64_t pos)
{
	aoff64_t _pos = di->pos;
	int rc;
	di->pos = pos;
	rc = fat_directory_block_load(di);
	if (rc!=EOK)
		di->pos = _pos;
	
	return rc;
}

int fat_directory_get(fat_directory_t *di, fat_dentry_t **d)
{
	int rc;
	
	rc = fat_directory_block_load(di);
	if (rc == EOK) {
		aoff64_t o = di->pos % (BPS(di->bs) / sizeof(fat_dentry_t));
		*d = ((fat_dentry_t *)di->b->data) + o;
	}
	
	return rc;
}

int fat_directory_read(fat_directory_t *di, char *name, fat_dentry_t **de)
{
	fat_dentry_t *d = NULL;

	do {
		if (fat_directory_get(di, &d) == EOK) {
			switch (fat_classify_dentry(d)) {
			case FAT_DENTRY_LAST:
				di->long_entry_count = 0;
				di->long_entry = false;
				return ENOENT;
			case FAT_DENTRY_LFN:
				if (di->long_entry) {
					/* We found long entry */
					di->long_entry_count--;
					if ((FAT_LFN_ORDER(d) == di->long_entry_count) && 
						(di->checksum == FAT_LFN_CHKSUM(d))) {
						/* Right order! */
						di->lfn_offset = fat_lfn_copy_entry(d, di->lfn_utf16, 
							di->lfn_offset);
					} else {
						/* Something wrong with order. Skip this long entries set */
						di->long_entry_count = 0;
						di->long_entry = false;
					}
				} else {
					if (FAT_IS_LFN(d)) {
						/* We found Last long entry! */
						if (FAT_LFN_COUNT(d) <= FAT_LFN_MAX_COUNT) {
							di->long_entry = true;
							di->long_entry_count = FAT_LFN_COUNT(d);
							di->lfn_size = (FAT_LFN_ENTRY_SIZE * 
								(FAT_LFN_COUNT(d) - 1)) + fat_lfn_size(d);
							di->lfn_offset = di->lfn_size;
							di->lfn_offset = fat_lfn_copy_entry(d, di->lfn_utf16, 
								di->lfn_offset);
							di->checksum = FAT_LFN_CHKSUM(d);
						}
					}
				}
				break;
			case FAT_DENTRY_VALID:
				if (di->long_entry && 
					(di->checksum == fat_dentry_chksum(d->name))) {
					int rc;
					rc = fat_lfn_convert_name(di->lfn_utf16, di->lfn_size, 
						(uint8_t*)name, FAT_LFN_NAME_SIZE);
					if (rc!=EOK)
						fat_dentry_name_get(d, name);
				}
				else
					fat_dentry_name_get(d, name);
				
				*de = d;
				di->long_entry_count = 0;
				di->long_entry = false;
				return EOK;
			default:
			case FAT_DENTRY_SKIP:
			case FAT_DENTRY_FREE:
				di->long_entry_count = 0;
				di->long_entry = false;
				break;
			}
		}
	} while (fat_directory_next(di) == EOK);
	
	return ENOENT;
}

int fat_directory_erase(fat_directory_t *di)
{
	int rc;
	fat_dentry_t *d;
	bool flag = false;

	rc = fat_directory_get(di, &d);
	if (rc != EOK)
		return rc;
	di->checksum = fat_dentry_chksum(d->name);

	d->name[0] = FAT_DENTRY_ERASED;
	di->b->dirty = true;
	
	while (!flag && fat_directory_prev(di) == EOK) {
		if (fat_directory_get(di, &d) == EOK &&
			fat_classify_dentry(d) == FAT_DENTRY_LFN &&			
			di->checksum == FAT_LFN_CHKSUM(d)) {
				if (FAT_IS_LFN(d))
					flag = true;
				memset(d, 0, sizeof(fat_dentry_t));
				d->name[0] = FAT_DENTRY_ERASED;
				di->b->dirty = true;
		}
		else
			break;
	}

	return EOK;
}


/**
 * @}
 */ 
