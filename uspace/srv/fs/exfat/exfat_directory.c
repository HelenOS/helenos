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

#include "exfat.h"
#include "exfat_directory.h"
#include "exfat_fat.h"
#include <block.h>
#include <errno.h>
#include <byteorder.h>
#include <mem.h>
#include <stdlib.h>
#include <str.h>
#include <align.h>

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

errno_t exfat_directory_open(exfat_node_t *nodep, exfat_directory_t *di)
{
	exfat_directory_init(di);
	di->nodep = nodep;	
	if (di->nodep->type != EXFAT_DIRECTORY)
		return EINVAL;
	di->service_id = nodep->idx->service_id;
	di->fragmented = nodep->fragmented;
	di->firstc = nodep->firstc;

	di->bs = block_bb_get(di->service_id);
/*	di->blocks = nodep->size / BPS(di->bs); */
	di->blocks = ROUND_UP(nodep->size, BPS(di->bs)) / BPS(di->bs);
	return EOK;
}

errno_t exfat_directory_open_parent(exfat_directory_t *di, 
    service_id_t service_id, exfat_cluster_t firstc, bool fragmented)
{
	exfat_directory_init(di);
	di->service_id = service_id;
	di->fragmented = fragmented;
	di->firstc = firstc;
	di->bs = block_bb_get(service_id);
	di->blocks = 0;
	return EOK;
}

errno_t exfat_directory_close(exfat_directory_t *di)
{
	errno_t rc = EOK;
	
	if (di->b) {
		rc = block_put(di->b);
		di->b = NULL;
	}
	
	return rc;
}

static errno_t exfat_directory_block_load(exfat_directory_t *di)
{
	uint32_t i;
	errno_t rc = EOK;

	i = (di->pos * sizeof(exfat_dentry_t)) / BPS(di->bs);
	if (di->nodep && (i >= di->blocks))
		return ENOENT;

	if (di->b && di->bnum != i) {
		rc = block_put(di->b);
		di->b = NULL;
		if (rc != EOK)
			return rc;
	}
	if (!di->b) {
		if (di->nodep) {
			rc = exfat_block_get(&di->b, di->bs, di->nodep, i,
			    BLOCK_FLAGS_NONE);
		} else {
			rc = exfat_block_get_by_clst(&di->b, di->bs,
			    di->service_id, di->fragmented, di->firstc, NULL, i,
			    BLOCK_FLAGS_NONE);
		}
		if (rc != EOK) {
			di->b = NULL;
			return rc;
		}
		di->bnum = i;
	}
	return rc;
}

errno_t exfat_directory_next(exfat_directory_t *di)
{
	errno_t rc;

	di->pos += 1;
	rc = exfat_directory_block_load(di);
	if (rc != EOK)
		di->pos -= 1;
	
	return rc;
}

errno_t exfat_directory_prev(exfat_directory_t *di)
{
	errno_t rc = EOK;
	
	if (di->pos > 0) {
		di->pos -= 1;
		rc = exfat_directory_block_load(di);
	} else
		return ENOENT;
	
	if (rc != EOK)
		di->pos += 1;
	
	return rc;
}

errno_t exfat_directory_seek(exfat_directory_t *di, aoff64_t pos)
{
	aoff64_t _pos = di->pos;
	errno_t rc;

	di->pos = pos;
	rc = exfat_directory_block_load(di);
	if (rc != EOK)
		di->pos = _pos;
	
	return rc;
}

errno_t exfat_directory_get(exfat_directory_t *di, exfat_dentry_t **d)
{
	errno_t rc;
	
	rc = exfat_directory_block_load(di);
	if (rc == EOK) {
		aoff64_t o = di->pos % (BPS(di->bs) / sizeof(exfat_dentry_t));
		*d = ((exfat_dentry_t *)di->b->data) + o;
	}
	
	return rc;
}

errno_t exfat_directory_find(exfat_directory_t *di, exfat_dentry_clsf_t type,
    exfat_dentry_t **d)
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

errno_t
exfat_directory_find_continue(exfat_directory_t *di, exfat_dentry_clsf_t type,
    exfat_dentry_t **d)
{
	errno_t rc;
	rc = exfat_directory_next(di);
	if (rc != EOK)
		return rc;
	return exfat_directory_find(di, type, d);
}


errno_t exfat_directory_read_file(exfat_directory_t *di, char *name, size_t size, 
    exfat_file_dentry_t *df, exfat_stream_dentry_t *ds)
{
	uint16_t wname[EXFAT_FILENAME_LEN + 1];
	exfat_dentry_t *d = NULL;
	errno_t rc;
	int i;
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

	for (i = 0; i < df->count - 1; i++) {
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

errno_t exfat_directory_read_vollabel(exfat_directory_t *di, char *label,
    size_t size)
{
	uint16_t wlabel[EXFAT_VOLLABEL_LEN + 1];
	exfat_dentry_t *d = NULL;
	errno_t rc;
	aoff64_t start_pos = 0;

	start_pos = di->pos;

	rc = exfat_directory_seek(di, 0);
	if (rc != EOK)
		return rc;

	rc = exfat_directory_find(di, EXFAT_DENTRY_VOLLABEL, &d);
	if (rc != EOK)
		return rc;

	exfat_dentry_get_vollabel(&d->vollabel, EXFAT_VOLLABEL_LEN, wlabel);

	rc = utf16_to_str(label, size, wlabel);
	if (rc != EOK)
		return rc;

	exfat_directory_seek(di, start_pos);
	return EOK;
}

static uint16_t exfat_directory_set_checksum(const uint8_t *bytes, size_t count)
{
	uint16_t checksum = 0;
	size_t idx;

	for (idx = 0; idx < count; idx++) {
		if (idx == 2 || idx == 3)
			continue;
		checksum = ((checksum << 15) | (checksum >> 1)) +
		    (uint16_t)bytes[idx];
	}
	return checksum;
}

errno_t exfat_directory_sync_file(exfat_directory_t *di, exfat_file_dentry_t *df, 
    exfat_stream_dentry_t *ds)
{
	errno_t rc;
	int i, count;
	exfat_dentry_t *array = NULL, *de;
	aoff64_t pos = di->pos;

	rc = exfat_directory_get(di, &de);
	if (rc != EOK)
		return rc;
	count = de->file.count + 1;
	array = (exfat_dentry_t *) malloc(count * sizeof(exfat_dentry_t));
	if (!array)
		return ENOMEM;
	for (i = 0; i < count; i++) {
		rc = exfat_directory_get(di, &de);
		if (rc != EOK) {
			free(array);
			return rc;
		}
		array[i] = *de;
		rc = exfat_directory_next(di);
		if (rc != EOK) {
			free(array);
			return rc;
		}
	}
	rc = exfat_directory_seek(di, pos);
	if (rc != EOK) {
		free(array);
		return rc;
	}

	/* Sync */
	array[0].file.attr = host2uint16_t_le(df->attr);
	array[1].stream.firstc = host2uint32_t_le(ds->firstc);
	array[1].stream.flags = ds->flags;
	array[1].stream.valid_data_size = host2uint64_t_le(ds->valid_data_size);
	array[1].stream.data_size = host2uint64_t_le(ds->data_size);
	array[0].file.checksum =
	    host2uint16_t_le(exfat_directory_set_checksum((uint8_t *)array,
	    count * sizeof(exfat_dentry_t)));

	/* Store */
	for (i = 0; i < count; i++) {
		rc = exfat_directory_get(di, &de);
		if (rc != EOK) {
			free(array);
			return rc;
		}
		*de = array[i];
		di->b->dirty = true;
		rc = exfat_directory_next(di);
		if (rc != EOK) {
			free(array);
			return rc;
		}
	}
	free(array);

	return EOK;
}

errno_t exfat_directory_write_file(exfat_directory_t *di, const char *name)
{
	fs_node_t *fn;
	exfat_node_t *uctablep;
	uint16_t *uctable;
	exfat_dentry_t df, ds, *de;
	uint16_t wname[EXFAT_FILENAME_LEN + 1];
	errno_t rc;
	int i;
	size_t uctable_chars, j;
	aoff64_t pos;

	rc = str_to_utf16(wname, EXFAT_FILENAME_LEN, name);
	if (rc != EOK)
		return rc;
	rc = exfat_uctable_get(&fn, di->service_id);
	if (rc != EOK)
		return rc;
	uctablep = EXFAT_NODE(fn);

	uctable_chars = ALIGN_DOWN(uctablep->size,
	    sizeof(uint16_t)) / sizeof(uint16_t); 
	uctable = (uint16_t *) malloc(uctable_chars * sizeof(uint16_t));
	rc = exfat_read_uctable(di->bs, uctablep, (uint8_t *)uctable);
	if (rc != EOK) {
		(void) exfat_node_put(fn);
		free(uctable);
		return rc;
	}

	/* Fill stream entry */
	ds.type = EXFAT_TYPE_STREAM;
	ds.stream.flags = 0;
	ds.stream.valid_data_size = 0;
	ds.stream.data_size = 0;
	ds.stream.name_size = utf16_wsize(wname);
	ds.stream.hash = host2uint16_t_le(exfat_name_hash(wname, uctable, 
	    uctable_chars));

	/* Fill file entry */
	df.type = EXFAT_TYPE_FILE;
	df.file.attr = 0;
	df.file.count = ROUND_UP(ds.stream.name_size, EXFAT_NAME_PART_LEN) / 
	    EXFAT_NAME_PART_LEN + 1;
	df.file.checksum = 0;

	free(uctable);
	rc = exfat_node_put(fn);
	if (rc != EOK)
		return rc;

	/* Looking for set of free entries */
	rc = exfat_directory_lookup_free(di, df.file.count + 1);
	if (rc != EOK)
		return rc;
	pos = di->pos;

	/* Write file entry */
	rc = exfat_directory_get(di, &de);
	if (rc != EOK)
		return rc;
	*de = df;
	di->b->dirty = true;
	rc = exfat_directory_next(di);
	if (rc != EOK)
		return rc;

	/* Write stream entry */
	rc = exfat_directory_get(di, &de);
	if (rc != EOK)
		return rc;
	*de = ds;
	di->b->dirty = true;

	/* Write file name */
	size_t chars = EXFAT_NAME_PART_LEN;
	uint16_t *sname = wname;

	for (i = 0; i < ds.stream.name_size; i++)
		wname[i] = host2uint16_t_le(wname[i]);

	for (i = 0; i < df.file.count - 1; i++) {
		rc = exfat_directory_next(di);
		if (rc != EOK)
			return rc;

		if (i == df.file.count - 2) {
			chars = ds.stream.name_size -
			    EXFAT_NAME_PART_LEN*(df.file.count - 2);
		}

		rc = exfat_directory_get(di, &de);
		if (rc != EOK)
			return rc;
		de->type = EXFAT_TYPE_NAME;
		/* test */
		for (j = 0; j < chars; j++) {
			de->name.name[j] = *sname;
			sname++;
		}

		di->b->dirty = true;
	}
	
	return exfat_directory_seek(di, pos);
}

errno_t exfat_directory_erase_file(exfat_directory_t *di, aoff64_t pos)
{
	errno_t rc;
	int count;
	exfat_dentry_t *de;

	di->pos = pos;

	rc = exfat_directory_get(di, &de);
	if (rc != EOK)
		return rc;
	count = de->file.count + 1;
	
	while (count) {
		rc = exfat_directory_get(di, &de);
		if (rc != EOK)
			return rc;
		de->type &= ~EXFAT_TYPE_USED;
		di->b->dirty = true;

		rc = exfat_directory_next(di);
		if (rc != EOK)
			return rc;
		count--;
	}
	return EOK;
}

errno_t exfat_directory_expand(exfat_directory_t *di)
{
	errno_t rc;

	if (!di->nodep)
		return ENOSPC;

	rc = exfat_node_expand(di->nodep->idx->service_id, di->nodep, 1);
	if (rc != EOK)
		return rc;

	di->fragmented = di->nodep->fragmented;
	di->nodep->size += BPC(di->bs);
	di->nodep->dirty = true;		/* need to sync node */
	di->blocks = di->nodep->size / BPS(di->bs);
	
	return EOK;
}

errno_t exfat_directory_lookup_free(exfat_directory_t *di, size_t count)
{
	errno_t rc;
	exfat_dentry_t *d;
	size_t found;
	aoff64_t pos;

	rc = exfat_directory_seek(di, 0);
	if (rc != EOK)
		return rc;

	do {
		found = 0;
		pos = 0;
		do {
			if (exfat_directory_get(di, &d) == EOK) {
				switch (exfat_classify_dentry(d)) {
				case EXFAT_DENTRY_LAST:
				case EXFAT_DENTRY_FREE:
					if (found == 0)
						pos = di->pos;
					found++;
					if (found == count) {
						exfat_directory_seek(di, pos);
						return EOK;
					}
					break;
				default:
					found = 0;
					break;
				}
			}
		} while (exfat_directory_next(di) == EOK);	
	} while (exfat_directory_expand(di) == EOK);
	return ENOSPC;
}


/**
 * @}
 */ 
