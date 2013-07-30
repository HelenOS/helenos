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

static int load_and_check_header(service_id_t, aoff64_t, size_t, gpt_header_t *);
static gpt_partitions_t * alloc_part_array(uint32_t);
static int extend_part_array(gpt_partitions_t *);
static int reduce_part_array(gpt_partitions_t *);
//static long long nearest_larger_int(double);
static uint8_t get_byte(const char *);
static bool check_overlap(gpt_part_t *, gpt_part_t *);
static bool check_encaps(gpt_part_t *, uint64_t, uint64_t);

/** Allocate memory for gpt label */
gpt_label_t * gpt_alloc_label(void)
{
	gpt_label_t *label = malloc(sizeof(gpt_label_t));
	if (label == NULL)
		return NULL;
	
	/* This is necessary so that gpt_part_foreach does not segfault */
	label->parts = gpt_alloc_partitions();
	if (label == NULL) {
		free(label);
		return NULL;
	}
	
	label->gpt = NULL;
	
	label->device = 0;
	
	return label;
}

/** Free gpt_label_t structure */
void gpt_free_label(gpt_label_t *label)
{
	if (label->gpt != NULL)
		gpt_free_gpt(label->gpt);
	
	if (label->parts != NULL)
		gpt_free_partitions(label->parts);
	
	free(label);
}

/** Allocate memory for gpt header */
gpt_t * gpt_alloc_header(size_t size)
{
	gpt_t *gpt = malloc(sizeof(gpt_t));
	if (gpt == NULL)
		return NULL;
	
	/* 
	 * We might need only sizeof(gpt_header_t), but we should follow 
	 * specs and have zeroes through all the rest of the block
	 */
	size_t final_size = size > sizeof(gpt_header_t) ? size : sizeof(gpt_header_t);
	gpt->header = malloc(final_size);
	if (gpt->header == NULL) {
		free(gpt);
		return NULL;
	}
	
	memset(gpt->header, 0, final_size);
	
	return gpt;
}

/** free() GPT header including gpt->header_lba */
void gpt_free_gpt(gpt_t *gpt)
{
	free(gpt->header);
	free(gpt);
}

/** Read GPT from specific device
 * @param label        label structure to fill
 * @param dev_handle   device to read GPT from
 *
 * @return             EOK on success, errorcode on error
 */
int gpt_read_header(gpt_label_t *label, service_id_t dev_handle)
{
	int rc;
	size_t b_size;
	
	rc = block_init(EXCHANGE_ATOMIC, dev_handle, 512);
	if (rc != EOK)
		goto fail;
	
	rc = block_get_bsize(dev_handle, &b_size);
	if (rc != EOK)
		goto fini_fail;
	
	if (label->gpt == NULL) {
		label->gpt = gpt_alloc_header(b_size);
		if (label->gpt == NULL) {
			rc = ENOMEM;
			goto fini_fail;
		}
	}
	
	rc = load_and_check_header(dev_handle, GPT_HDR_BA, b_size, label->gpt->header);
	if (rc == EBADCHECKSUM || rc == EINVAL) {
		aoff64_t n_blocks;
		rc = block_get_nblocks(dev_handle, &n_blocks);
		if (rc != EOK)
			goto free_fail;

		rc = load_and_check_header(dev_handle, n_blocks - 1, b_size, label->gpt->header);
		if (rc == EBADCHECKSUM || rc == EINVAL)
			goto free_fail;
	}
	
	label->device = dev_handle;
	block_fini(dev_handle);
	return EOK;
	
free_fail:
	gpt_free_gpt(label->gpt);
	label->gpt = NULL;
fini_fail:
	block_fini(dev_handle);
fail:
	return rc;
}

/** Write GPT header to device
 * @param label        GPT label header to be written
 * @param dev_handle   device handle to write the data to
 *
 * @return             EOK on success, libblock error code otherwise
 *
 * Note: Firstly write partitions (if modified), then gpt header.
 */
int gpt_write_header(gpt_label_t *label, service_id_t dev_handle)
{
	int rc;
	size_t b_size;
	
	/* The comm_size argument (the last one) is ignored */
	rc = block_init(EXCHANGE_ATOMIC, dev_handle, 4096);
	if (rc != EOK && rc != EEXIST)
		return rc;
	
	rc = block_get_bsize(dev_handle, &b_size);
	if (rc != EOK)
		return rc;
	
	aoff64_t n_blocks;
	rc = block_get_nblocks(dev_handle, &n_blocks);
	if (rc != EOK) {
		block_fini(dev_handle);
		return rc;
	}
	
	uint64_t tmp;
	
	/* Prepare the backup header */
	label->gpt->header->alternate_lba = label->gpt->header->my_lba;
	label->gpt->header->my_lba = host2uint64_t_le(n_blocks - 1);
	
	tmp = label->gpt->header->entry_lba;
	label->gpt->header->entry_lba = host2uint64_t_le(n_blocks - 
	    (uint32_t_le2host(label->gpt->header->fillries) * sizeof(gpt_entry_t)) 
	    / b_size - 1);
	
	label->gpt->header->header_crc32 = 0;
	label->gpt->header->header_crc32 = host2uint32_t_le( 
	    compute_crc32((uint8_t *) label->gpt->header,
	        uint32_t_le2host(label->gpt->header->header_size)));
	
	/* Write to backup GPT header location */
	rc = block_write_direct(dev_handle, n_blocks - 1, GPT_HDR_BS, label->gpt->header);
	if (rc != EOK) {
		block_fini(dev_handle);
		return rc;
	}
	
	
	/* Prepare the main header */
	label->gpt->header->entry_lba = tmp;
	
	tmp = label->gpt->header->alternate_lba;
	label->gpt->header->alternate_lba = label->gpt->header->my_lba;
	label->gpt->header->my_lba = tmp;
	
	label->gpt->header->header_crc32 = 0;
	label->gpt->header->header_crc32 = host2uint32_t_le( 
	    compute_crc32((uint8_t *) label->gpt->header,
	        uint32_t_le2host(label->gpt->header->header_size)));
	
	/* Write to main GPT header location */
	rc = block_write_direct(dev_handle, GPT_HDR_BA, GPT_HDR_BS, label->gpt->header);
	block_fini(dev_handle);
	if (rc != EOK)
		return rc;
	
	
	return 0;
}

/** Alloc partition array */
gpt_partitions_t * gpt_alloc_partitions()
{
	return alloc_part_array(GPT_MIN_PART_NUM);
}

/** Parse partitions from GPT
 * @param label   GPT label to be parsed
 *
 * @return        EOK on success, errorcode otherwise
 */
int gpt_read_partitions(gpt_label_t *label)
{
	int rc;
	unsigned int i;
	uint32_t fillries = uint32_t_le2host(label->gpt->header->fillries);
	uint32_t ent_size = uint32_t_le2host(label->gpt->header->entry_size);
	uint64_t ent_lba = uint64_t_le2host(label->gpt->header->entry_lba);
	
	if (label->parts == NULL) {
		label->parts = alloc_part_array(fillries);
		if (label->parts == NULL) {
			return ENOMEM;
		}
	}

	/* comm_size is ignored */
	rc = block_init(EXCHANGE_SERIALIZE, label->device, sizeof(gpt_entry_t));
	if (rc != EOK)
		goto fail;

	size_t block_size;
	rc = block_get_bsize(label->device, &block_size);
	if (rc != EOK)
		goto fini_fail;

	//size_t bufpos = 0;
	//size_t buflen = 0;
	aoff64_t pos = ent_lba * block_size;

	/* 
	 * Now we read just sizeof(gpt_entry_t) bytes for each entry from the device.
	 * Hopefully, this does not bypass cache (no mention in libblock.c),
	 * and also allows us to have variable partition entry size (but we
	 * will always read just sizeof(gpt_entry_t) bytes - hopefully they
	 * don't break backward compatibility) 
	 */
	for (i = 0; i < fillries; ++i) {
		/*FIXME: this does bypass cache... */
		rc = block_read_bytes_direct(label->device, pos, sizeof(gpt_entry_t), label->parts->part_array + i);
		/* 
		 * FIXME: but seqread() is just too complex...
		 * rc = block_seqread(gpt->device, &bufpos, &buflen, &pos, res->part_array[i], sizeof(gpt_entry_t));
		 */
		pos += ent_size;

		if (rc != EOK)
			goto fini_fail;
	}
	
	uint32_t crc = compute_crc32((uint8_t *) label->parts->part_array, 
	                   fillries * ent_size);

	if(uint32_t_le2host(label->gpt->header->pe_array_crc32) != crc)
	{
		rc = EBADCHECKSUM;
		goto fini_fail;
	}
	
	block_fini(label->device);
	return EOK;
	
fini_fail:
	block_fini(label->device);
	
fail:
	gpt_free_partitions(label->parts);
	label->parts = NULL;
	return rc;
}

/** Write GPT and partitions to device
 * Note: also writes the header.
 * @param label        label to write
 * @param dev_handle   device to write the data to
 *
 * @return             returns EOK on succes, errorcode otherwise
 */
int gpt_write_partitions(gpt_label_t *label, service_id_t dev_handle)
{
	int rc;
	size_t b_size;
	uint32_t e_size = uint32_t_le2host(label->gpt->header->entry_size);
	size_t fillries = label->parts->fill > GPT_MIN_PART_NUM ? label->parts->fill : GPT_MIN_PART_NUM;
	
	if (e_size != sizeof(gpt_entry_t))
		return ENOTSUP;
	
	/* comm_size of 4096 is ignored */
	rc = block_init(EXCHANGE_ATOMIC, dev_handle, 4096);
	if (rc != EOK && rc != EEXIST)
		return rc;
	
	rc = block_get_bsize(dev_handle, &b_size);
	if (rc != EOK)
		goto fail;
	
	aoff64_t n_blocks;
	rc = block_get_nblocks(dev_handle, &n_blocks);
	if (rc != EOK)
		goto fail;
	
	label->gpt->header->fillries = host2uint32_t_le(fillries);
	uint64_t arr_blocks = (fillries * sizeof(gpt_entry_t)) / b_size;
	label->gpt->header->first_usable_lba = host2uint64_t_le(arr_blocks + 1);
	uint64_t first_lba = n_blocks - arr_blocks - 2;
	label->gpt->header->last_usable_lba = host2uint64_t_le(first_lba);
	
	/* Perform checks */
	gpt_part_foreach(label, p) {
		if (gpt_get_part_type(p) == GPT_PTE_UNUSED)
			continue;
		
		if (!check_encaps(p, n_blocks, first_lba)) {
			rc = ERANGE;
			goto fail;
		}
		
		gpt_part_foreach(label, q) {
			if (p == q)
				continue;
			
			if (gpt_get_part_type(p) != GPT_PTE_UNUSED) {
				if (check_overlap(p, q)) {
					rc = ERANGE;
					goto fail;
				}
			}
		}
	}
	
	label->gpt->header->pe_array_crc32 = host2uint32_t_le(compute_crc32(
	                               (uint8_t *) label->parts->part_array,
	                               fillries * e_size));
	
	
	/* Write to backup GPT partition array location */
	rc = block_write_direct(dev_handle, n_blocks - arr_blocks - 1, 
	         arr_blocks, label->parts->part_array);
	if (rc != EOK)
		goto fail;
	
	/* Write to main GPT partition array location */
	rc = block_write_direct(dev_handle, uint64_t_le2host(label->gpt->header->entry_lba),
	         arr_blocks, label->parts->part_array);
	if (rc != EOK)
		goto fail;
	
	return gpt_write_header(label, dev_handle);
	
fail:
	block_fini(dev_handle);
	return rc;
}

/** Alloc new partition
 *
 * @return        returns pointer to the new partition or NULL
 *
 * Note: use either gpt_alloc_partition or gpt_get_partition.
 * This returns a memory block (zero-filled) and needs gpt_add_partition()
 * to be called to insert it into a partition array.
 * Requires you to call gpt_free_partition afterwards.
 */
gpt_part_t * gpt_alloc_partition(void)
{
	gpt_part_t *p = malloc(sizeof(gpt_part_t));
	if (p == NULL)
		return NULL;
	
	memset(p, 0, sizeof(gpt_part_t));
	
	return p;
}

/** Alloc new partition already inside the label
 *
 * @param label   label to carry new partition
 *
 * @return        returns pointer to the new partition or NULL on ENOMEM
 *
 * Note: use either gpt_alloc_partition or gpt_get_partition.
 * This one returns a pointer to the first empty structure already
 * inside the array, so don't call gpt_add_partition() afterwards.
 * This is the one you will usually want.
 */
gpt_part_t * gpt_get_partition(gpt_label_t *label)
{
	gpt_part_t *p;
	
	
	/* Find the first empty entry */
	do {
		if (label->parts->fill == label->parts->arr_size) {
			if (extend_part_array(label->parts) == -1)
				return NULL;
		}
		
		p = label->parts->part_array + label->parts->fill++;
		
	} while (gpt_get_part_type(p) != GPT_PTE_UNUSED);
	
	return p;
}

/** Get partition already inside the label
 *
 * @param label   label to carrying the partition
 * @param idx     index of the partition
 *
 * @return        returns pointer to the partition
 *                or NULL when out of range
 *
 * Note: For new partitions use either gpt_alloc_partition or
 * gpt_get_partition unless you want a partition at a specific place.
 * This returns a pointer to a structure already inside the array,
 * so don't call gpt_add_partition() afterwards.
 * This function is handy when you want to change already existing
 * partition or to simply write somewhere in the middle. This works only
 * for indexes smaller than either 128 or the actual number of filled
 * entries.
 */
gpt_part_t * gpt_get_partition_at(gpt_label_t *label, size_t idx)
{
	return NULL;
	
	if (idx >= GPT_MIN_PART_NUM && idx >= label->parts->fill)
		return NULL;
	
	return label->parts->part_array + idx;
}

/** Copy partition into partition array
 *
 * @param parts			target label
 * @param partition		source partition to copy
 *
 * @return 				-1 on error, 0 otherwise
 *
 * Note: for use with gpt_alloc_partition() only. You will get
 * duplicates with gpt_get_partition().
 * Note: does not call gpt_free_partition()!
 */
int gpt_add_partition(gpt_label_t *label, gpt_part_t *partition)
{
	gpt_part_t *p;
	/* Find the first empty entry */
	do {
		if (label->parts->fill == label->parts->arr_size) {
			if (extend_part_array(label->parts) == -1)
				return ENOMEM;
		}
		
		p = label->parts->part_array + label->parts->fill++;
		
	} while (gpt_get_part_type(p) != GPT_PTE_UNUSED);
	
	
	memcpy(p, partition, sizeof(gpt_entry_t));
	
	
	return EOK;
}

/** Remove partition from array
 * @param label   label to remove from
 * @param idx     index of the partition to remove
 *
 * @return        EOK on success, ENOMEM on array reduction failure
 *
 * Note: even if it fails, the partition still gets removed. Only
 * reducing the array failed.
 */
int gpt_remove_partition(gpt_label_t *label, size_t idx)
{
	if (idx >= label->parts->arr_size)
		return EINVAL;
	
	/* 
	 * FIXME! 
	 * If we allow blank spots, we break the array. If we have more than
	 * 128 partitions in the array and then remove something from
	 * the first 128 partitions, we would forget to write the last one.
	 */
	memset(label->parts->part_array + idx, 0, sizeof(gpt_entry_t));
	
	if (label->parts->fill > idx)
		label->parts->fill = idx;
	
	/* 
	 * FIXME! HOPEFULLY FIXED.
	 * We cannot reduce the array so simply. We may have some partitions
	 * there since we allow blank spots. 
	 */
	gpt_part_t * p;
	
	if (label->parts->fill > GPT_MIN_PART_NUM &&
	    label->parts->fill < (label->parts->arr_size / 2) - GPT_IGNORE_FILL_NUM) {
		for (p = gpt_get_partition_at(label, label->parts->arr_size / 2); 
		     p < label->parts->part_array + label->parts->arr_size; ++p) {
				if (gpt_get_part_type(p) != GPT_PTE_UNUSED)
					return EOK;
		}
		
		if (reduce_part_array(label->parts) == ENOMEM)
			return ENOMEM;
	}

	return EOK;
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
		if (p->part_type[3] == get_byte(gpt_ptypes[i].guid +0) &&
			p->part_type[2] == get_byte(gpt_ptypes[i].guid +2) &&
			p->part_type[1] == get_byte(gpt_ptypes[i].guid +4) &&
			p->part_type[0] == get_byte(gpt_ptypes[i].guid +6) &&
			
			p->part_type[5] == get_byte(gpt_ptypes[i].guid +8) &&
			p->part_type[4] == get_byte(gpt_ptypes[i].guid +10) &&
			
			p->part_type[7] == get_byte(gpt_ptypes[i].guid +12) &&
			p->part_type[6] == get_byte(gpt_ptypes[i].guid +14) &&
			
			p->part_type[8] == get_byte(gpt_ptypes[i].guid +16) &&
			p->part_type[9] == get_byte(gpt_ptypes[i].guid +18) &&
			p->part_type[10] == get_byte(gpt_ptypes[i].guid +20) &&
			p->part_type[11] == get_byte(gpt_ptypes[i].guid +22) &&
			p->part_type[12] == get_byte(gpt_ptypes[i].guid +24) &&
			p->part_type[13] == get_byte(gpt_ptypes[i].guid +26) &&
			p->part_type[14] == get_byte(gpt_ptypes[i].guid +28) &&
			p->part_type[15] == get_byte(gpt_ptypes[i].guid +30))
				break;
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

/** Get partition name */
unsigned char * gpt_get_part_name(gpt_part_t * p)
{
	return p->part_name;
}

/** Copy partition name */
void gpt_set_part_name(gpt_part_t *p, char *name, size_t length)
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

/** Generate a new pseudo-random UUID
 * @param uuid Pointer to the UUID to overwrite.
 */
void gpt_set_random_uuid(uint8_t * uuid)
{
	srandom((unsigned int) uuid);
	
	unsigned int i;
	for (i = 0; i < 16/sizeof(long int); ++i)
		((long int *)uuid)[i] = random();
	
}

/** Get next aligned address */
uint64_t gpt_get_next_aligned(uint64_t addr, unsigned int alignment)
{
	uint64_t div = addr / alignment;
	return (div + 1) * alignment;
}

/* Internal functions follow */

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
	
	memset(res->part_array, 0, size * sizeof(gpt_entry_t));
	
	res->fill = 0;
	res->arr_size = num;

	return res;
}

static int extend_part_array(gpt_partitions_t * p)
{
	size_t nsize = p->arr_size * 2;
	gpt_entry_t * tmp = malloc(nsize * sizeof(gpt_entry_t));
	if(tmp == NULL) {
		errno = ENOMEM;
		return -1;
	}
	
	memcpy(tmp, p->part_array, p->fill * sizeof(gpt_entry_t));
	free(p->part_array);
	p->part_array = tmp;
	p->arr_size = nsize;

	return 0;
}

static int reduce_part_array(gpt_partitions_t * p)
{
	if(p->arr_size > GPT_MIN_PART_NUM) {
		unsigned int nsize = p->arr_size / 2;
		nsize = nsize > GPT_MIN_PART_NUM ? nsize : GPT_MIN_PART_NUM;
		gpt_entry_t * tmp = malloc(nsize * sizeof(gpt_entry_t));
		if(tmp == NULL)
			return ENOMEM;

		memcpy(tmp, p->part_array, p->fill < nsize ? p->fill : nsize);
		free(p->part_array);
		p->part_array = tmp;
		p->arr_size = nsize;
	}

	return 0;
}

/*static long long nearest_larger_int(double a)
{
	if ((long long) a == a) {
		return (long long) a;
	}

	return ((long long) a) + 1;
}*/

/* Parse a byte from a string in hexadecimal 
 * i.e., "FF" => 255 
 */
static uint8_t get_byte(const char * c)
{
	uint8_t val = 0;
	char hex[3] = {*c, *(c+1), 0};
	
	errno = str_uint8_t(hex, NULL, 16, false, &val);
	return val;
}

static bool check_overlap(gpt_part_t * p1, gpt_part_t * p2)
{
	if (gpt_get_start_lba(p1) < gpt_get_start_lba(p2) && gpt_get_end_lba(p1) <= gpt_get_start_lba(p2)) {
		return false;
	} else if (gpt_get_start_lba(p1) > gpt_get_start_lba(p2) && gpt_get_end_lba(p2) <= gpt_get_start_lba(p1)) {
		return false;
	}

	return true;
}

static bool check_encaps(gpt_part_t *p, uint64_t n_blocks, uint64_t first_lba)
{
	uint64_t start = uint64_t_le2host(p->start_lba);
	uint64_t end = uint64_t_le2host(p->end_lba);
	
	if (start >= first_lba && end < n_blocks - first_lba)
		return true;
	
	return false;
}
