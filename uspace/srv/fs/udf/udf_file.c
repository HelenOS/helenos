/*
 * Copyright (c) 2012 Julia Medvedeva
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

/** @addtogroup udf
 * @{
 */
/**
 * @file udf_file.c
 * @brief Implementation of file operations. Reading and writing functions.
 */

#include <block.h>
#include <libfs.h>
#include <errno.h>
#include <stdlib.h>
#include <inttypes.h>
#include <io/log.h>
#include <mem.h>
#include "udf.h"
#include "udf_file.h"
#include "udf_cksum.h"
#include "udf_volume.h"

/** Read extended allocator in allocation sequence
 *
 * @paran node     UDF node
 * @param icb_flag Type of allocators in sequence.
 *                 According to ECMA 167 4/14.8.8
 * @param pos      Position with which we read
 *
 * @return EOK on success or an error code.
 *
 */
static errno_t udf_read_extended_allocator(udf_node_t *node, uint16_t icb_flag,
    uint32_t pos)
{
	block_t *block = NULL;
	errno_t rc = block_get(&block, node->instance->service_id, pos,
	    BLOCK_FLAGS_NONE);
	if (rc != EOK)
		return rc;

	udf_ext_ad_t *exd = (udf_ext_ad_t *) block->data;
	uint32_t start = node->instance->partitions[
	    FLE16(exd->extent_location.partition_num)].start +
	    FLE32(exd->extent_location.lblock_num);

	log_msg(LOG_DEFAULT, LVL_DEBUG,
	    "Extended allocator: start=%d, block_num=%d, len=%d", start,
	    FLE32(exd->extent_location.lblock_num), FLE32(exd->info_length));

	uint32_t len = FLE32(exd->info_length);
	block_put(block);

	return udf_read_allocation_sequence(node, NULL, icb_flag, start, len);
}

/** Read ICB sequence of allocators in (Extended) File entry descriptor
 *
 * @parem node     UDF node
 * @param af       (Extended) File entry descriptor
 * @param icb_flag Type of allocators in sequence.
 *                 According to ECMA 167 4/14.8.8
 * @param start_alloc Offset of the allocator
 * @param len         Length of sequence
 *
 * @return EOK on success or an error code.
 *
 */
errno_t udf_read_allocation_sequence(udf_node_t *node, uint8_t *af,
    uint16_t icb_flag, uint32_t start_alloc, uint32_t len)
{
	node->alloc_size = 0;

	switch (icb_flag) {
	case UDF_SHORT_AD:
		log_msg(LOG_DEFAULT, LVL_DEBUG,
		    "ICB: sequence of allocation descriptors - icbflag = short_ad_t");

		/*
		 * Identify number of current partition. Virtual partition
		 * could placed inside of physical partition. It means that same
		 * sector could be inside of both partition physical and virtual.
		 */
		size_t pd_num = (size_t) -1;
		size_t min_start = 0;

		for (size_t i = 0; i < node->instance->partition_cnt; i++) {
			if ((node->index >= node->instance->partitions[i].start) &&
			    (node->index < node->instance->partitions[i].start +
			    node->instance->partitions[i].length)) {
				if (node->instance->partitions[i].start >= min_start) {
					min_start = node->instance->partitions[i].start;
					pd_num = i;
				}
			}
		}

		if (pd_num == (size_t) -1)
			return ENOENT;

		/*
		 * According to doc, in this we should stop our loop if pass
		 * all allocators. Count of items in sequence of allocators
		 * cnt = len / sizeof(udf_long_ad_t)
		 * But in case of Blu-Ray data len could be zero.
		 * It means that we have only two conditions for stopping
		 * which we check inside of loop.
		 */

		while (true) {
			udf_short_ad_t *short_d =
			    (udf_short_ad_t *) (af + start_alloc +
			    node->alloc_size * sizeof(udf_short_ad_t));

			if (FLE32(short_d->length) == 0)
				break;

			/*
			 * ECMA 167 4/12 - next sequence of allocation descriptors
			 * condition according to 167 4/14.6.8
			 */
			if (FLE32(short_d->length) >> 30 == 3) {
				udf_read_extended_allocator(node, icb_flag,
				    node->instance->partitions[pd_num].start +
				    FLE32(short_d->position));
				break;
			}

			node->allocators = realloc(node->allocators,
			    (node->alloc_size + 1) * sizeof(udf_allocator_t));
			node->allocators[node->alloc_size].length =
			    EXT_LENGTH(FLE32(short_d->length));
			node->allocators[node->alloc_size].position =
			    node->instance->partitions[pd_num].start + FLE32(short_d->position);
			node->alloc_size++;
		}

		node->allocators = realloc(node->allocators,
		    node->alloc_size * sizeof(udf_allocator_t));
		break;

	case UDF_LONG_AD:
		log_msg(LOG_DEFAULT, LVL_DEBUG,
		    "ICB: sequence of allocation descriptors - icbflag = long_ad_t");

		while (true) {
			udf_long_ad_t *long_d =
			    (udf_long_ad_t *) (af + start_alloc +
			    node->alloc_size * sizeof(udf_long_ad_t));

			if (FLE32(long_d->length) == 0)
				break;

			uint32_t pos_long_ad = udf_long_ad_to_pos(node->instance, long_d);

			/*
			 * ECMA 167 4/12 - next sequence of allocation descriptors
			 * condition according to 167 4/14.6.8
			 */
			if (FLE32(long_d->length) >> 30 == 3) {
				udf_read_extended_allocator(node, icb_flag, pos_long_ad);
				break;
			}

			node->allocators = realloc(node->allocators,
			    (node->alloc_size + 1) * sizeof(udf_allocator_t));
			node->allocators[node->alloc_size].length =
			    EXT_LENGTH(FLE32(long_d->length));
			node->allocators[node->alloc_size].position = pos_long_ad;

			node->alloc_size++;
		}

		node->allocators = realloc(node->allocators,
		    node->alloc_size * sizeof(udf_allocator_t));
		break;

	case UDF_EXTENDED_AD:
		log_msg(LOG_DEFAULT, LVL_DEBUG,
		    "ICB: sequence of allocation descriptors - icbflag = extended_ad_t");
		break;

	case UDF_DATA_AD:
		log_msg(LOG_DEFAULT, LVL_DEBUG,
		    "ICB: sequence of allocation descriptors - icbflag = 3, node contains data itself");

		node->data = malloc(node->data_size);
		if (!node->data)
			return ENOMEM;

		memcpy(node->data, (af + start_alloc), node->data_size);
		node->alloc_size = 0;
		break;
	}

	return EOK;
}

/** Read ICB sequence of descriptors
 *
 * Read sequence of descriptors (file entry descriptors or
 * extended file entry descriptors). Each descriptor contains
 * sequence of allocators.
 *
 * @param node    UDF node
 *
 * @return    EOK on success or an error code.
 */
errno_t udf_read_icb(udf_node_t *node)
{
	while (true) {
		fs_index_t pos = node->index;

		block_t *block = NULL;
		errno_t rc = block_get(&block, node->instance->service_id, pos,
		    BLOCK_FLAGS_NONE);
		if (rc != EOK)
			return rc;

		udf_descriptor_tag_t *data = (udf_descriptor_tag_t *) block->data;
		if (data->checksum != udf_tag_checksum((uint8_t *) data)) {
			block_put(block);
			return EINVAL;
		}

		/* One sector size descriptors */
		switch (FLE16(data->id)) {
		case UDF_FILE_ENTRY:
			log_msg(LOG_DEFAULT, LVL_DEBUG, "ICB: File entry descriptor found");

			udf_file_entry_descriptor_t *file =
			    (udf_file_entry_descriptor_t *) block->data;
			uint16_t icb_flag = FLE16(file->icbtag.flags) & UDF_ICBFLAG_MASK;
			node->data_size = FLE64(file->info_length);
			node->type = (file->icbtag.file_type == UDF_ICBTYPE_DIR) ? NODE_DIR : NODE_FILE;

			rc = udf_read_allocation_sequence(node, (uint8_t *) file, icb_flag,
			    FLE32(file->ea_length) + UDF_FE_OFFSET, FLE32(file->ad_length));
			block_put(block);
			return rc;

		case UDF_EFILE_ENTRY:
			log_msg(LOG_DEFAULT, LVL_DEBUG, "ICB: Extended file entry descriptor found");

			udf_extended_file_entry_descriptor_t *efile =
			    (udf_extended_file_entry_descriptor_t *) block->data;
			icb_flag = FLE16(efile->icbtag.flags) & UDF_ICBFLAG_MASK;
			node->data_size = FLE64(efile->info_length);
			node->type = (efile->icbtag.file_type == UDF_ICBTYPE_DIR) ? NODE_DIR : NODE_FILE;

			rc = udf_read_allocation_sequence(node, (uint8_t *) efile, icb_flag,
			    FLE32(efile->ea_length) + UDF_EFE_OFFSET, FLE32(efile->ad_length));
			block_put(block);
			return rc;

		case UDF_ICB_TERMINAL:
			log_msg(LOG_DEFAULT, LVL_DEBUG, "ICB: Terminal entry descriptor found");
			block_put(block);
			return EOK;
		}

		pos++;

		rc = block_put(block);
		if (rc != EOK)
			return rc;
	}

	return EOK;
}

/** Read data from disk - filling UDF node by allocators
 *
 * @param node UDF node
 *
 * @return EOK on success or an error code.
 *
 */
errno_t udf_node_get_core(udf_node_t *node)
{
	node->link_cnt = 1;
	return udf_read_icb(node);
}

/** Read directory entry if all FIDs is saved inside of descriptor
 *
 * @param fid  Returned value
 * @param node UDF node
 * @param pos  Number of FID which we need to find
 *
 * @return EOK on success or an error code.
 *
 */
static errno_t udf_get_fid_in_data(udf_file_identifier_descriptor_t **fid,
    udf_node_t *node, aoff64_t pos)
{
	size_t fid_sum = 0;
	size_t n = 0;

	while (node->data_size - fid_sum >= MIN_FID_LEN) {
		udf_descriptor_tag_t *desc =
		    (udf_descriptor_tag_t *) (node->data + fid_sum);
		if (desc->checksum != udf_tag_checksum((uint8_t *) desc)) {
			if (fid_sum == 0)
				return EINVAL;
			else
				return ENOENT;
		}

		*fid = (udf_file_identifier_descriptor_t *)
		    (node->data + fid_sum);

		/* According to ECMA 167 4/14.4.9 */
		size_t padding = 4 * (((*fid)->length_file_id +
		    FLE16((*fid)->length_iu) + 38 + 3) / 4) -
		    ((*fid)->length_file_id + FLE16((*fid)->length_iu) + 38);
		size_t size_fid = (*fid)->length_file_id +
		    FLE16((*fid)->length_iu) + padding + 38;

		fid_sum += size_fid;

		/* aAcording to ECMA 167 4/8.6 */
		if (((*fid)->length_file_id != 0) &&
		    (((*fid)->file_characteristics & 4) == 0)) {
			n++;

			if (n == pos + 1)
				return EOK;
		}
	}

	return ENOENT;
}

/** Read directory entry
 *
 * @param fid   Returned value
 * @param block Returned value
 * @param node  UDF node
 * @param pos   Number of FID which we need to find
 *
 * @return EOK on success or an error code.
 *
 */
errno_t udf_get_fid(udf_file_identifier_descriptor_t **fid, block_t **block,
    udf_node_t *node, aoff64_t pos)
{
	if (node->data == NULL)
		return udf_get_fid_in_allocator(fid, block, node, pos);

	return udf_get_fid_in_data(fid, node, pos);
}

/** Read directory entry if all FIDS is saved in allocators.
 *
 * @param fid   Returned value
 * @param block Returned value
 * @param node  UDF node
 * @param pos   Number of FID which we need to find
 *
 * @return EOK on success or an error code.
 *
 */
errno_t udf_get_fid_in_allocator(udf_file_identifier_descriptor_t **fid,
    block_t **block, udf_node_t *node, aoff64_t pos)
{
	void *buf = malloc(node->instance->sector_size);

	// FIXME: Check for NULL return value

	size_t j = 0;
	size_t n = 0;
	size_t len = 0;

	while (j < node->alloc_size) {
		size_t i = 0;
		while (i * node->instance->sector_size < node->allocators[j].length) {
			errno_t rc = block_get(block, node->instance->service_id,
			    node->allocators[j].position + i, BLOCK_FLAGS_NONE);
			if (rc != EOK) {
				// FIXME: Memory leak
				return rc;
			}

			/*
			 * Last item in allocator is a part of sector. We take
			 * this fragment and join it to part of sector in next allocator.
			 */
			if ((node->allocators[j].length / node->instance->sector_size == i) &&
			    (node->allocators[j].length - i * node->instance->sector_size <
			    MIN_FID_LEN)) {
				size_t len = node->allocators[j].length - i * node->instance->sector_size;
				memcpy(buf, (*block)->data, len);
				block_put(*block);
				*block = NULL;
				break;
			}

			rc = udf_get_fid_in_sector(fid, block, node, pos, &n, &buf, &len);
			if (rc == EOK) {
				// FIXME: Memory leak
				return EOK;
			}

			if (rc == EINVAL) {
				// FIXME: Memory leak
				return ENOENT;
			}

			if (rc == ENOENT) {
				if (block) {
					rc = block_put(*block);
					*block = NULL;

					if (rc != EOK)
						return rc;
				}
			}

			i++;
		}

		j++;
	}

	if (buf)
		free(buf);

	return ENOENT;
}

/** Read FIDs in sector inside of current allocator
 *
 * @param fid   Returned value.
 * @param block Returned value
 * @param node  UDF node
 * @param pos   Number FID which we need to find
 * @param n     Count of FIDs which we already have passed
 * @param buf   Part of FID from last sector (current allocator or previous)
 * @param len   Length of buf
 *
 * @return EOK on success or an error code.
 *
 */
errno_t udf_get_fid_in_sector(udf_file_identifier_descriptor_t **fid,
    block_t **block, udf_node_t *node, aoff64_t pos, size_t *n, void **buf,
    size_t *len)
{
	void *fidbuf = malloc(node->instance->sector_size);

	// FIXME: Check for NULL return value

	bool buf_flag;

	if (*len > 0) {
		memcpy(fidbuf, *buf, *len);
		buf_flag = true;
	} else
		buf_flag = false;

	size_t fid_sum = 0;
	while (node->instance->sector_size - fid_sum > 0) {
		if (node->instance->sector_size - fid_sum >= MIN_FID_LEN) {
			void *fid_data;

			if (buf_flag) {
				memcpy((fidbuf + *len), (*block)->data,
				    node->instance->sector_size - *len);
				fid_data = fidbuf;
			} else
				fid_data = (*block)->data + fid_sum;

			udf_descriptor_tag_t *desc =
			    (udf_descriptor_tag_t *) fid_data;

			if (desc->checksum != udf_tag_checksum((uint8_t *) desc)) {
				if (fidbuf)
					free(fidbuf);

				if (*buf) {
					free(*buf);
					*buf = NULL;
					*len = 0;
				}

				return EINVAL;
			}

			*fid = (udf_file_identifier_descriptor_t *) fid_data;

			/* According to ECMA 167 4/14.4.9 */
			size_t padding = 4 * (((*fid)->length_file_id +
			    FLE16((*fid)->length_iu) + 38 + 3) / 4) -
			    ((*fid)->length_file_id + FLE16((*fid)->length_iu) + 38);
			size_t size_fid = (*fid)->length_file_id +
			    FLE16((*fid)->length_iu) + padding + 38;
			if (buf_flag)
				fid_sum += size_fid - *len;
			else
				fid_sum += size_fid;

			/* According to ECMA 167 4/8.6 */
			if (((*fid)->length_file_id != 0) &&
			    (((*fid)->file_characteristics & 4) == 0)) {
				(*n)++;
				if (*n == pos + 1) {
					if (fidbuf)
						free(fidbuf);

					return EOK;
				}
			}

			if (fidbuf) {
				buf_flag = false;
				free(fidbuf);
				fidbuf = NULL;
			}

			if (*buf) {
				free(*buf);
				*buf = NULL;
				*len = 0;
			}
		} else {
			if (*buf)
				free(*buf);

			*len = node->instance->sector_size - fid_sum;
			*buf = malloc(*len);
			buf_flag = false;
			memcpy(*buf, ((*block)->data + fid_sum), *len);

			return ENOENT;
		}
	}

	return ENOENT;
}

/** Read file if it is saved in allocators.
 *
 * @param read_len Returned value. Length file or part file which we could read.
 * @param call     IPC call
 * @param node     UDF node
 * @param pos      Position in file since we have to read.
 * @param len      Length of data for reading
 *
 * @return EOK on success or an error code.
 *
 */
errno_t udf_read_file(size_t *read_len, ipc_call_t *call, udf_node_t *node,
    aoff64_t pos, size_t len)
{
	size_t i = 0;
	size_t l = 0;

	while (i < node->alloc_size) {
		if (pos >= l + node->allocators[i].length) {
			l += node->allocators[i].length;
			i++;
		} else
			break;
	}

	size_t sector_cnt = ALL_UP(l, node->instance->sector_size);
	size_t sector_num = pos / node->instance->sector_size;

	block_t *block = NULL;
	errno_t rc = block_get(&block, node->instance->service_id,
	    node->allocators[i].position + (sector_num - sector_cnt),
	    BLOCK_FLAGS_NONE);
	if (rc != EOK) {
		async_answer_0(call, rc);
		return rc;
	}

	size_t sector_pos = pos % node->instance->sector_size;

	if (sector_pos + len < node->instance->sector_size)
		*read_len = len;
	else
		*read_len = node->instance->sector_size - sector_pos;

	if (ALL_UP(node->allocators[i].length, node->instance->sector_size) ==
	    sector_num - sector_cnt + 1) {
		if (pos + len > node->allocators[i].length + l)
			*read_len = node->allocators[i].length -
			    (sector_num - sector_cnt) * node->instance->sector_size -
			    sector_pos;
		else
			*read_len = len;
	}

	async_data_read_finalize(call, block->data + sector_pos, *read_len);
	return block_put(block);
}

/**
 * @}
 */
