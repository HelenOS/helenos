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

/** @addtogroup fat
 * @{
 */

/**
 * @file	fat_directory.c
 * @brief	Functions that work with FAT directory.
 */

#include "fat_directory.h"
#include "fat_fat.h"
#include <block.h>
#include <errno.h>
#include <byteorder.h>
#include <mem.h>
#include <str.h>
#include <align.h>
#include <stdio.h>

errno_t fat_directory_open(fat_node_t *nodep, fat_directory_t *di)
{
	di->b = NULL;
	di->nodep = nodep;
	if (di->nodep->type != FAT_DIRECTORY)
		return EINVAL;

	di->bs = block_bb_get(di->nodep->idx->service_id);
	di->blocks = ROUND_UP(nodep->size, BPS(di->bs)) / BPS(di->bs);
	di->pos = 0;
	di->bnum = 0;
	di->last = false;

	return EOK;
}

errno_t fat_directory_close(fat_directory_t *di)
{
	errno_t rc = EOK;

	if (di->b)
		rc = block_put(di->b);

	return rc;
}

static errno_t fat_directory_block_load(fat_directory_t *di)
{
	uint32_t i;
	errno_t rc;

	i = (di->pos * sizeof(fat_dentry_t)) / BPS(di->bs);
	if (i < di->blocks) {
		if (di->b && di->bnum != i) {
			block_put(di->b);
			di->b = NULL;
		}
		if (!di->b) {
			rc = fat_block_get(&di->b, di->bs, di->nodep, i,
			    BLOCK_FLAGS_NONE);
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

errno_t fat_directory_next(fat_directory_t *di)
{
	errno_t rc;

	di->pos += 1;
	rc = fat_directory_block_load(di);
	if (rc != EOK)
		di->pos -= 1;

	return rc;
}

errno_t fat_directory_prev(fat_directory_t *di)
{
	errno_t rc = EOK;

	if (di->pos > 0) {
		di->pos -= 1;
		rc = fat_directory_block_load(di);
	} else
		return ENOENT;

	if (rc != EOK)
		di->pos += 1;

	return rc;
}

errno_t fat_directory_seek(fat_directory_t *di, aoff64_t pos)
{
	aoff64_t _pos = di->pos;
	errno_t rc;

	di->pos = pos;
	rc = fat_directory_block_load(di);
	if (rc != EOK)
		di->pos = _pos;

	return rc;
}

errno_t fat_directory_get(fat_directory_t *di, fat_dentry_t **d)
{
	errno_t rc;

	rc = fat_directory_block_load(di);
	if (rc == EOK) {
		aoff64_t o = di->pos % (BPS(di->bs) / sizeof(fat_dentry_t));
		*d = ((fat_dentry_t *)di->b->data) + o;
	}

	return rc;
}

errno_t fat_directory_read(fat_directory_t *di, char *name, fat_dentry_t **de)
{
	fat_dentry_t *d = NULL;
	uint16_t wname[FAT_LFN_NAME_LEN];
	size_t lfn_offset, lfn_size;
	bool long_entry = false;
	int long_entry_count = 0;
	uint8_t checksum = 0;
	errno_t rc;

	void *data;
	fat_instance_t *instance;

	rc = fs_instance_get(di->nodep->idx->service_id, &data);
	assert(rc == EOK);
	instance = (fat_instance_t *) data;

	do {
		rc = fat_directory_get(di, &d);
		if (rc != EOK)
			return rc;

		switch (fat_classify_dentry(d)) {
		case FAT_DENTRY_LAST:
			long_entry_count = 0;
			long_entry = false;
			return ENOENT;
		case FAT_DENTRY_LFN:
			if (long_entry) {
				/* We found long entry */
				long_entry_count--;
				if ((FAT_LFN_ORDER(d) == long_entry_count) &&
				    (checksum == FAT_LFN_CHKSUM(d))) {
					/* Right order! */
					fat_lfn_get_entry(d, wname,
					    &lfn_offset);
				} else {
					/*
					 * Something wrong with order.
					 * Skip this long entries set.
					 */
					long_entry_count = 0;
					long_entry = false;
				}
			} else if (FAT_IS_LFN(d) && instance->lfn_enabled) {
				/* We found Last long entry! */
				if (FAT_LFN_COUNT(d) <= FAT_LFN_MAX_COUNT) {
					long_entry = true;
					long_entry_count = FAT_LFN_COUNT(d);
					lfn_size = (FAT_LFN_ENTRY_SIZE *
					    (FAT_LFN_COUNT(d) - 1)) +
					    fat_lfn_size(d);
					lfn_offset = lfn_size;
					fat_lfn_get_entry(d, wname,
					    &lfn_offset);
					checksum = FAT_LFN_CHKSUM(d);
				}
			}
			break;
		case FAT_DENTRY_VALID:
			if (long_entry &&
			    (checksum == fat_dentry_chksum(d->name))) {
				wname[lfn_size] = '\0';
				if (utf16_to_str(name, FAT_LFN_NAME_SIZE,
				    wname) != EOK)
					fat_dentry_name_get(d, name);
			} else
				fat_dentry_name_get(d, name);

			*de = d;
			return EOK;
		case FAT_DENTRY_SKIP:
		case FAT_DENTRY_FREE:
		case FAT_DENTRY_VOLLABEL:
		default:
			long_entry_count = 0;
			long_entry = false;
			break;
		}
	} while (fat_directory_next(di) == EOK);

	return ENOENT;
}

errno_t fat_directory_erase(fat_directory_t *di)
{
	errno_t rc;
	fat_dentry_t *d;
	bool flag = false;
	uint8_t checksum;

	rc = fat_directory_get(di, &d);
	if (rc != EOK)
		return rc;
	checksum = fat_dentry_chksum(d->name);

	d->name[0] = FAT_DENTRY_ERASED;
	di->b->dirty = true;

	while (!flag && fat_directory_prev(di) == EOK) {
		if (fat_directory_get(di, &d) == EOK &&
		    fat_classify_dentry(d) == FAT_DENTRY_LFN &&
		    checksum == FAT_LFN_CHKSUM(d)) {
			if (FAT_IS_LFN(d))
				flag = true;
			memset(d, 0, sizeof(fat_dentry_t));
			d->name[0] = FAT_DENTRY_ERASED;
			di->b->dirty = true;
		} else
			break;
	}

	return EOK;
}

errno_t fat_directory_write(fat_directory_t *di, const char *name, fat_dentry_t *de)
{
	errno_t rc;
	void *data;
	fat_instance_t *instance;

	rc = fs_instance_get(di->nodep->idx->service_id, &data);
	assert(rc == EOK);
	instance = (fat_instance_t *) data;

	if (fat_valid_short_name(name)) {
		/*
		 * NAME could be directly stored in dentry without creating
		 * LFN.
		 */
		fat_dentry_name_set(de, name);
		if (fat_directory_is_sfn_exist(di, de))
			return EEXIST;
		rc = fat_directory_lookup_free(di, 1);
		if (rc != EOK)
			return rc;
		rc = fat_directory_write_dentry(di, de);
		return rc;
	} else if (instance->lfn_enabled && fat_valid_name(name)) {
		/* We should create long entries to store name */
		int long_entry_count;
		uint8_t checksum;
		uint16_t wname[FAT_LFN_NAME_LEN];
		size_t lfn_size, lfn_offset;

		rc = str_to_utf16(wname, FAT_LFN_NAME_LEN, name);
		if (rc != EOK)
			return rc;

		lfn_size = utf16_wsize(wname);
		long_entry_count = lfn_size / FAT_LFN_ENTRY_SIZE;
		if (lfn_size % FAT_LFN_ENTRY_SIZE)
			long_entry_count++;
		rc = fat_directory_lookup_free(di, long_entry_count + 1);
		if (rc != EOK)
			return rc;
		aoff64_t start_pos = di->pos;

		/* Write Short entry */
		rc = fat_directory_create_sfn(di, de, name);
		if (rc != EOK)
			return rc;
		checksum = fat_dentry_chksum(de->name);

		rc = fat_directory_seek(di, start_pos + long_entry_count);
		if (rc != EOK)
			return rc;
		rc = fat_directory_write_dentry(di, de);
		if (rc != EOK)
			return rc;

		/* Write Long entry by parts */
		lfn_offset = 0;
		fat_dentry_t *d;
		size_t idx = 0;
		do {
			rc = fat_directory_prev(di);
			if (rc != EOK)
				return rc;
			rc = fat_directory_get(di, &d);
			if (rc != EOK)
				return rc;
			fat_lfn_set_entry(wname, &lfn_offset, lfn_size + 1, d);
			FAT_LFN_CHKSUM(d) = checksum;
			FAT_LFN_ORDER(d) = ++idx;
			di->b->dirty = true;
		} while (lfn_offset < lfn_size);
		FAT_LFN_ORDER(d) |= FAT_LFN_LAST;

		rc = fat_directory_seek(di, start_pos + long_entry_count);
		return rc;
	}

	return ENOTSUP;
}

errno_t fat_directory_create_sfn(fat_directory_t *di, fat_dentry_t *de,
    const char *lname)
{
	char name[FAT_NAME_LEN + 1];
	char ext[FAT_EXT_LEN + 1];
	char number[FAT_NAME_LEN + 1];
	memset(name, FAT_PAD, FAT_NAME_LEN);
	memset(ext, FAT_PAD, FAT_EXT_LEN);
	memset(number, FAT_PAD, FAT_NAME_LEN);

	size_t name_len = str_size(lname);
	char *pdot = str_rchr(lname, '.');
	ext[FAT_EXT_LEN] = '\0';
	if (pdot) {
		pdot++;
		str_to_ascii(ext, pdot, FAT_EXT_LEN, FAT_SFN_CHAR);
		name_len = (pdot - lname - 1);
	}
	if (name_len > FAT_NAME_LEN)
		name_len = FAT_NAME_LEN;
	str_to_ascii(name, lname, name_len, FAT_SFN_CHAR);

	unsigned idx;
	for (idx = 1; idx <= FAT_MAX_SFN; idx++) {
		snprintf(number, sizeof(number), "%u", idx);

		/* Fill de->name with FAT_PAD */
		memset(de->name, FAT_PAD, FAT_NAME_LEN + FAT_EXT_LEN);
		/* Copy ext */
		memcpy(de->ext, ext, str_size(ext));
		/* Copy name */
		memcpy(de->name, name, str_size(name));

		/* Copy number */
		size_t offset;
		if (str_size(name) + str_size(number) + 1 > FAT_NAME_LEN)
			offset = FAT_NAME_LEN - str_size(number) - 1;
		else
			offset = str_size(name);
		de->name[offset] = '~';
		offset++;
		memcpy(de->name + offset, number, str_size(number));

		if (!fat_directory_is_sfn_exist(di, de))
			return EOK;
	}

	return ERANGE;
}

errno_t fat_directory_write_dentry(fat_directory_t *di, fat_dentry_t *de)
{
	fat_dentry_t *d;
	errno_t rc;

	rc = fat_directory_get(di, &d);
	if (rc != EOK)
		return rc;
	memcpy(d, de, sizeof(fat_dentry_t));
	di->b->dirty = true;

	return EOK;
}

errno_t fat_directory_expand(fat_directory_t *di)
{
	errno_t rc;
	fat_cluster_t mcl, lcl;

	if (!FAT_IS_FAT32(di->bs) && di->nodep->firstc == FAT_CLST_ROOT) {
		/* Can't grow the root directory on FAT12/16. */
		return ENOSPC;
	}
	rc = fat_alloc_clusters(di->bs, di->nodep->idx->service_id, 1, &mcl,
	    &lcl);
	if (rc != EOK)
		return rc;
	rc = fat_zero_cluster(di->bs, di->nodep->idx->service_id, mcl);
	if (rc != EOK) {
		(void) fat_free_clusters(di->bs, di->nodep->idx->service_id,
		    mcl);
		return rc;
	}
	rc = fat_append_clusters(di->bs, di->nodep, mcl, lcl);
	if (rc != EOK) {
		(void) fat_free_clusters(di->bs, di->nodep->idx->service_id,
		    mcl);
		return rc;
	}
	di->nodep->size += BPS(di->bs) * SPC(di->bs);
	di->nodep->dirty = true;		/* need to sync node */
	di->blocks = di->nodep->size / BPS(di->bs);

	return EOK;
}

errno_t fat_directory_lookup_free(fat_directory_t *di, size_t count)
{
	fat_dentry_t *d;
	size_t found;
	aoff64_t pos;
	errno_t rc;

	do {
		found = 0;
		pos = 0;
		fat_directory_seek(di, 0);
		do {
			rc = fat_directory_get(di, &d);
			if (rc != EOK)
				return rc;

			switch (fat_classify_dentry(d)) {
			case FAT_DENTRY_LAST:
			case FAT_DENTRY_FREE:
				if (found == 0)
					pos = di->pos;
				found++;
				if (found == count) {
					fat_directory_seek(di, pos);
					return EOK;
				}
				break;
			case FAT_DENTRY_VALID:
			case FAT_DENTRY_LFN:
			case FAT_DENTRY_SKIP:
			case FAT_DENTRY_VOLLABEL:
			default:
				found = 0;
				break;
			}
		} while (fat_directory_next(di) == EOK);
	} while (fat_directory_expand(di) == EOK);

	return ENOSPC;
}

errno_t fat_directory_lookup_name(fat_directory_t *di, const char *name,
    fat_dentry_t **de)
{
	char entry[FAT_LFN_NAME_SIZE];

	fat_directory_seek(di, 0);
	while (fat_directory_read(di, entry, de) == EOK) {
		if (fat_dentry_namecmp(entry, name) == 0) {
			return EOK;
		} else {
			if (fat_directory_next(di) != EOK)
				break;
		}
	}

	return ENOENT;
}

bool fat_directory_is_sfn_exist(fat_directory_t *di, fat_dentry_t *de)
{
	fat_dentry_t *d;
	errno_t rc;

	fat_directory_seek(di, 0);
	do {
		rc = fat_directory_get(di, &d);
		if (rc != EOK)
			return false;

		switch (fat_classify_dentry(d)) {
		case FAT_DENTRY_LAST:
			return false;
		case FAT_DENTRY_VALID:
			if (memcmp(de->name, d->name,
			    FAT_NAME_LEN + FAT_EXT_LEN) == 0)
				return true;
			break;
		default:
		case FAT_DENTRY_LFN:
		case FAT_DENTRY_SKIP:
		case FAT_DENTRY_VOLLABEL:
		case FAT_DENTRY_FREE:
			break;
		}
	} while (fat_directory_next(di) == EOK);

	return false;
}

/** Find volume label entry in a directory.
 *
 * @return EOK on success, ENOENT if not found, EIO on I/O error
 */
errno_t fat_directory_vollabel_get(fat_directory_t *di, char *label)
{
	fat_dentry_t *d;
	errno_t rc;

	fat_directory_seek(di, 0);
	do {
		rc = fat_directory_get(di, &d);
		if (rc != EOK)
			return EIO;

		switch (fat_classify_dentry(d)) {
		case FAT_DENTRY_VOLLABEL:
			fat_dentry_vollabel_get(d, label);
			return EOK;
		default:
			break;
		}
	} while (fat_directory_next(di) == EOK);

	/* Not found */
	return ENOENT;
}

/**
 * @}
 */
