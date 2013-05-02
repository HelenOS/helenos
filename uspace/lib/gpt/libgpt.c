/*
 * Copyright (c) 2011, 2012, 2013 Dominik Taborsky
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

/** @addtogroup libgpt
 * @{
 */
/** @file
 */

/* TODO:
 * This implementation only supports fixed size partition entries. Specification
 * requires otherwise, though. Use void * array and casting to achieve that.
 */

#include <ipc/bd.h>
#include <async.h>
#include <stdio.h>
#include <block.h>
#include <errno.h>
#include <stdlib.h>
#include <assert.h>
#include <byteorder.h>
#include <checksum.h>
#include <mem.h>

#include "libgpt.h"

static int load_and_check_header(service_id_t handle, aoff64_t addr, size_t b_size, gpt_header_t * header);
static gpt_partitions_t * alloc_part_array(uint32_t num);
static int extend_part_array(gpt_partitions_t * p);
static int reduce_part_array(gpt_partitions_t * p);
static long long nearest_larger_int(double a);
static int gpt_memcmp(const void * a, const void * b, size_t len);

/** Allocate memory for gpt header */
gpt_t * gpt_alloc_gpt_header()
{
	return malloc(sizeof(gpt_t));
}

/** Read GPT from specific device
 * @param	dev_handle	device to read GPT from
 *
 * @return				GPT record on success, NULL on error
 */
gpt_t * gpt_read_gpt_header(service_id_t dev_handle)
{
	int rc;
	size_t b_size;
	
	rc = block_init(EXCHANGE_ATOMIC, dev_handle, 512);
	if (rc != EOK)
		return NULL;
	
	rc = block_get_bsize(dev_handle, &b_size);
	if (rc != EOK) {
		errno = rc;
		return NULL;
	}
	
	gpt_t * gpt = malloc(sizeof(gpt_t));
	if (gpt == NULL) {
		errno = ENOMEM;
		return NULL;
	}

	gpt->raw_data = malloc(b_size);	// We might need only sizeof(gpt_header_t),
	if (gpt == NULL) {				// but we should follow specs and have
		free(gpt);					// zeroes through all the rest of the block
		errno = ENOMEM;
		return NULL;
	}
	
	
	rc = load_and_check_header(dev_handle, GPT_HDR_BA, b_size, gpt->raw_data);
	if (rc == EBADCHECKSUM || rc == EINVAL) {
		aoff64_t n_blocks;
		rc = block_get_nblocks(dev_handle, &n_blocks);
		if (rc != EOK) {
			errno = rc;
			goto fail;
		}

		rc = load_and_check_header(dev_handle, n_blocks - 1, b_size, gpt->raw_data);
		if (rc == EBADCHECKSUM || rc == EINVAL) {
			errno = rc;
			goto fail;
		}
	}
	
	gpt->device = dev_handle;
	block_fini(dev_handle);
	return gpt;
	
fail:
	block_fini(dev_handle);
	gpt_free_gpt(gpt);
	return NULL;
}

/** Write GPT header to device
 * @param header		GPT header to be written
 * @param dev_handle	device handle to write the data to
 *
 * @return				0 on success, libblock error code otherwise
 *
 * Note: Firstly write partitions (if changed), then gpt header.
 */
int gpt_write_gpt_header(gpt_t * gpt, service_id_t dev_handle)
{
	int rc;
	size_t b_size;

	gpt->raw_data->header_crc32 = 0;
	gpt->raw_data->header_crc32 = compute_crc32((uint8_t *) gpt->raw_data,
					uint32_t_le2host(gpt->raw_data->header_size));

	rc = block_init(EXCHANGE_ATOMIC, dev_handle, b_size);
	if (rc != EOK)
		return rc;

	rc = block_get_bsize(dev_handle, &b_size);
	if (rc != EOK)
		return rc;

	/* Write to main GPT header location */
	rc = block_write_direct(dev_handle, GPT_HDR_BA, GPT_HDR_BS, gpt->raw_data);
	if (rc != EOK)
		block_fini(dev_handle);
		return rc;

	aoff64_t n_blocks;
	rc = block_get_nblocks(dev_handle, &n_blocks);
	if (rc != EOK)
		return rc;

	/* Write to backup GPT header location */
	//FIXME: those idiots thought it would be cool to have these fields in reverse order...
	rc = block_write_direct(dev_handle, n_blocks - 1, GPT_HDR_BS, gpt->raw_data);
	block_fini(dev_handle);
	if (rc != EOK)
		return rc;

	return 0;
}

/** Alloc partition array */
gpt_partitions_t *	gpt_alloc_partitions()
{
	return alloc_part_array(128);
}

/** Parse partitions from GPT
 * @param gpt	GPT to be parsed
 *
 * @return 		partition linked list pointer or NULL on error
 * 				error code is stored in errno
 */
gpt_partitions_t * gpt_read_partitions(gpt_t * gpt)
{
	int rc;
	unsigned int i;
	gpt_partitions_t * res;
	uint32_t fill = uint32_t_le2host(gpt->raw_data->fillries);
	uint32_t ent_size = uint32_t_le2host(gpt->raw_data->entry_size);
	uint64_t ent_lba = uint64_t_le2host(gpt->raw_data->entry_lba);

	res = alloc_part_array(fill);
	if (res == NULL) {
		//errno = ENOMEM; // already set in alloc_part_array()
		return NULL;
	}

	/* We can limit comm_size like this:
	 *  - we don't need more bytes
	 *  - the size of GPT partition entry can be different to 128 bytes */
	rc = block_init(EXCHANGE_SERIALIZE, gpt->device, sizeof(gpt_entry_t));
	if (rc != EOK) {
		gpt_free_partitions(res);
		errno = rc;
		return NULL;
	}

	size_t block_size;
	rc = block_get_bsize(gpt->device, &block_size);
	if (rc != EOK) {
		gpt_free_partitions(res);
		errno = rc;
		return NULL;
	}

	//size_t bufpos = 0;
	//size_t buflen = 0;
	aoff64_t pos = ent_lba * block_size;

	/* Now we read just sizeof(gpt_entry_t) bytes for each entry from the device.
	 * Hopefully, this does not bypass cache (no mention in libblock.c),
	 * and also allows us to have variable partition entry size (but we
	 * will always read just sizeof(gpt_entry_t) bytes - hopefully they
	 * don't break backward compatibility) */
	for (i = 0; i < fill; ++i) {
		//FIXME: this does bypass cache...
		rc = block_read_bytes_direct(gpt->device, pos, sizeof(gpt_entry_t), res->part_array + i);
		//FIXME: but seqread() is just too complex...
		//rc = block_seqread(gpt->device, &bufpos, &buflen, &pos, res->part_array[i], sizeof(gpt_entry_t));
		pos += ent_size;

		if (rc != EOK) {
			gpt_free_partitions(res);
			errno = rc;
			return NULL;
		}
	}

	/* FIXME: so far my boasting about variable partition entry size
	 * will not work. The CRC32 checksums will be different.
	 * This can't be fixed easily - we'd have to run the checksum
	 * on all of the partition entry array.
	 */
	uint32_t crc = compute_crc32((uint8_t *) res->part_array, res->fill * sizeof(gpt_entry_t));

	if(uint32_t_le2host(gpt->raw_data->pe_array_crc32) != crc)
	{
		gpt_free_partitions(res);
		errno = EBADCHECKSUM;
		return NULL;
	}

	return res;
}

/** Write GPT and partitions to device
 * @param parts			partition list to be written
 * @param header		GPT header belonging to the 'parts' partitions
 * @param dev_handle	device to write the data to
 *
 * @return				returns EOK on succes, specific error code otherwise
 */
int gpt_write_partitions(gpt_partitions_t * parts, gpt_t * gpt, service_id_t dev_handle)
{
	int rc;
	size_t b_size;

	gpt->raw_data->pe_array_crc32 = compute_crc32((uint8_t *) parts->part_array, parts->fill * gpt->raw_data->entry_size);

	rc = block_init(EXCHANGE_ATOMIC, dev_handle, b_size);
	if (rc != EOK)
		return rc;

	rc = block_get_bsize(dev_handle, &b_size);
	if (rc != EOK)
		return rc;

	/* Write to main GPT partition array location */
	rc = block_write_direct(dev_handle, uint64_t_le2host(gpt->raw_data->entry_lba),
			nearest_larger_int((uint64_t_le2host(gpt->raw_data->entry_size) * parts->fill) / b_size),
			parts->part_array);
	if (rc != EOK)
		block_fini(dev_handle);
		return rc;

	aoff64_t n_blocks;
	rc = block_get_nblocks(dev_handle, &n_blocks);
	if (rc != EOK)
		return rc;

	/* Write to backup GPT partition array location */
	//rc = block_write_direct(dev_handle, n_blocks - 1, GPT_HDR_BS, header->raw_data);
	block_fini(dev_handle);
	if (rc != EOK)
		return rc;


	return gpt_write_gpt_header(gpt, dev_handle);
}

/** Alloc new partition
 *
 * @param parts		partition table to carry new partition
 *
 * @return			returns pointer to the new partition or NULL on ENOMEM
 *
 * Note: use either gpt_alloc_partition or gpt_add_partition. The first
 * returns a pointer to write your data to, the second copies the data
 * (and does not free the memory).
 */
gpt_part_t * gpt_alloc_partition(gpt_partitions_t * parts)
{
	if (parts->fill == parts->arr_size) {
		if (extend_part_array(parts) == -1)
			return NULL;
	}

	return parts->part_array + parts->fill++;
}

/** Copy partition into partition array
 *
 * @param parts			target partition array
 * @param partition		source partition to copy
 *
 * @return 				-1 on error, 0 otherwise
 *
 * Note: use either gpt_alloc_partition or gpt_add_partition. The first
 * returns a pointer to write your data to, the second copies the data
 * (and does not free the memory).
 */
int gpt_add_partition(gpt_partitions_t * parts, gpt_part_t * partition)
{
	if (parts->fill == parts->arr_size) {
		if (extend_part_array(parts) == -1)
			return ENOMEM;
	}
	extend_part_array(parts);
	return EOK;;
}

/** Remove partition from array
 *
 * @param idx		index of the partition to remove
 *
 * @return			-1 on error, 0 otherwise
 *
 * Note: even if it fails, the partition still gets removed. Only
 * reducing the array failed.
 */
int gpt_remove_partition(gpt_partitions_t * parts, size_t idx)
{
	if (idx != parts->fill - 1) {
		memcpy(parts->part_array + idx, parts->part_array + parts->fill - 1, sizeof(gpt_entry_t));
		parts->fill -= 1;
	}

	if (parts->fill < (parts->arr_size / 2) - GPT_IGNORE_FILL_NUM) {
		if (reduce_part_array(parts) == -1)
			return -1;
	}

	return 0;
}

/** free() GPT header including gpt->header_lba */
void gpt_free_gpt(gpt_t * gpt)
{
	free(gpt->raw_data);
	free(gpt);
}

/** Free partition list
 *
 * @param parts		partition list to be freed
 */
void gpt_free_partitions(gpt_partitions_t * parts)
{
	free(parts->part_array);
	free(parts);
}

/** Get partition type by linear search
 * (hopefully this doesn't get slow)
 */
size_t gpt_get_part_type(gpt_part_t * p)
{
	size_t i;
	for (i = 0; gpt_ptypes[i].guid != NULL; i++) {
		if (gpt_memcmp(p->part_type, gpt_ptypes[i].guid, 16) == 0) {
			break;
		}
	}
	return i;
}

/** Set partition type
 * @param p			partition to be set
 * @param type		partition type to set
 * 					- see our fine selection at gpt_ptypes to choose from
 */
void gpt_set_part_type(gpt_part_t * p, size_t type)
{
	/* Beware: first 3 blocks are byteswapped! */
	p->part_type[3] = gpt_ptypes[type].guid[0];
	p->part_type[2] = gpt_ptypes[type].guid[1];
	p->part_type[1] = gpt_ptypes[type].guid[2];
	p->part_type[0] = gpt_ptypes[type].guid[3];

	p->part_type[5] = gpt_ptypes[type].guid[4];
	p->part_type[4] = gpt_ptypes[type].guid[5];

	p->part_type[7] = gpt_ptypes[type].guid[6];
	p->part_type[6] = gpt_ptypes[type].guid[7];

	p->part_type[8] = gpt_ptypes[type].guid[8];
	p->part_type[9] = gpt_ptypes[type].guid[9];
	p->part_type[10] = gpt_ptypes[type].guid[10];
	p->part_type[11] = gpt_ptypes[type].guid[11];
	p->part_type[12] = gpt_ptypes[type].guid[12];
	p->part_type[13] = gpt_ptypes[type].guid[13];
	p->part_type[14] = gpt_ptypes[type].guid[14];
	p->part_type[15] = gpt_ptypes[type].guid[15];
}

/** Get partition starting LBA */
uint64_t gpt_get_start_lba(gpt_part_t * p)
{
	return uint64_t_le2host(p->start_lba);
}

/** Set partition starting LBA */
void gpt_set_start_lba(gpt_part_t * p, uint64_t start)
{
	p->start_lba = host2uint64_t_le(start);
}

/** Get partition ending LBA */
uint64_t gpt_get_end_lba(gpt_part_t * p)
{
	return uint64_t_le2host(p->end_lba);
}

/** Set partition ending LBA */
void gpt_set_end_lba(gpt_part_t * p, uint64_t end)
{
	p->end_lba = host2uint64_t_le(end);
}


unsigned char * gpt_get_part_name(gpt_part_t * p)
{
	return p->part_name;
}

/** Copy partition name */
void gpt_set_part_name(gpt_part_t * p, char * name[], size_t length)
{
	if (length >= 72)
		length = 71;

	memcpy(p->part_name, name, length);
	p->part_name[length] = '\0';
}

/** Get partition attribute */
bool gpt_get_flag(gpt_part_t * p, GPT_ATTR flag)
{
	return (p->attributes & (((uint64_t) 1) << flag)) ? 1 : 0;
}

/** Set partition attribute */
void gpt_set_flag(gpt_part_t * p, GPT_ATTR flag, bool value)
{
	uint64_t attr = p->attributes;

	if (value)
		attr = attr | (((uint64_t) 1) << flag);
	else
		attr = attr ^ (attr & (((uint64_t) 1) << flag));

	p->attributes = attr;
}

// Internal functions follow //

static int load_and_check_header(service_id_t dev_handle, aoff64_t addr, size_t b_size, gpt_header_t * header)
{
	int rc;

	rc = block_read_direct(dev_handle, addr, GPT_HDR_BS, header);
	if (rc != EOK)
		return rc;

	unsigned int i;
	/* Check the EFI signature */
	for (i = 0; i < 8; ++i) {
		if (header->efi_signature[i] != efi_signature[i])
			return EINVAL;
	}

	/* Check the CRC32 of the header */
	uint32_t crc = header->header_crc32;
	header->header_crc32 = 0;
	if (crc != compute_crc32((uint8_t *) header, header->header_size))
		return EBADCHECKSUM;
	else
		header->header_crc32 = crc;

	/* Check for zeroes in the rest of the block */
	for (i = sizeof(gpt_header_t); i < b_size; ++i) {
		if (((uint8_t *) header)[i] != 0)
			return EINVAL;
	}

	return EOK;
}

static gpt_partitions_t * alloc_part_array(uint32_t num)
{
	gpt_partitions_t * res = malloc(sizeof(gpt_partitions_t));
	if (res == NULL) {
		errno = ENOMEM;
		return NULL;
	}

	uint32_t size = num > GPT_BASE_PART_NUM ? num : GPT_BASE_PART_NUM;
	res->part_array = malloc(size * sizeof(gpt_entry_t));
	if (res->part_array == NULL) {
		free(res);
		errno = ENOMEM;
		return NULL;
	}

	res->fill = num;
	res->arr_size = size;

	return res;
}

static int extend_part_array(gpt_partitions_t * p)
{
	unsigned int nsize = p->arr_size * 2;
	gpt_entry_t * tmp = malloc(nsize * sizeof(gpt_entry_t));
	if(tmp == NULL) {
		errno = ENOMEM;
		return -1;
	}

	memcpy(tmp, p->part_array, p->fill);
	free(p->part_array);
	p->part_array = tmp;
	p->arr_size = nsize;

	return 0;
}

static int reduce_part_array(gpt_partitions_t * p)
{
	if(p->arr_size > GPT_MIN_PART_NUM) {
		unsigned int nsize = p->arr_size / 2;
		gpt_entry_t * tmp = malloc(nsize * sizeof(gpt_entry_t));
		if(tmp == NULL) {
			errno = ENOMEM;
			return -1;
		}

		memcpy(tmp, p->part_array, p->fill < nsize ? p->fill  : nsize);
		free(p->part_array);
		p->part_array = tmp;
		p->arr_size = nsize;
	}

	return 0;
}

//FIXME: replace this with a library call, if it exists
static long long nearest_larger_int(double a)
{
	if ((long long) a == a) {
		return (long long) a;
	}

	return ((long long) a) + 1;
}

static int gpt_memcmp(const void * a, const void * b, size_t len)
{
	size_t i;
	int diff;
	const unsigned char * x = a;
	const unsigned char * y = b;

	for (i = 0; i < len; i++) {
		diff = (int)*(x++) - (int)*(y++);
		if (diff != 0) {
			return diff;
		}
	}
	return 0;
}




