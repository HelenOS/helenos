/*
 * Copyright (c) 2011-2013 Dominik Taborsky
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
 * The implementation currently supports fixed size partition entries only.
 * The specification requires otherwise, though.
 */

#include <ipc/bd.h>
#include <async.h>
#include <stdio.h>
#include <block.h>
#include <errno.h>
#include <stdlib.h>
#include <assert.h>
#include <byteorder.h>
#include <adt/checksum.h>
#include <mem.h>
#include <sys/typefmt.h>
#include <mbr.h>
#include <align.h>
#include "libgpt.h"

static int load_and_check_header(service_id_t, aoff64_t, size_t, gpt_header_t *);
static gpt_partitions_t *alloc_part_array(uint32_t);
static int extend_part_array(gpt_partitions_t *);
static int reduce_part_array(gpt_partitions_t *);
static uint8_t get_byte(const char *);
static bool check_overlap(gpt_part_t *, gpt_part_t *);
static bool check_encaps(gpt_part_t *, uint64_t, uint64_t);

/** Allocate a GPT label */
gpt_label_t *gpt_alloc_label(void)
{
	gpt_label_t *label = malloc(sizeof(gpt_label_t));
	if (label == NULL)
		return NULL;
	
	label->parts = gpt_alloc_partitions();
	if (label->parts == NULL) {
		free(label);
		return NULL;
	}
	
	label->gpt = NULL;
	label->device = 0;
	
	return label;
}

/** Free a GPT label */
void gpt_free_label(gpt_label_t *label)
{
	if (label->gpt != NULL)
		gpt_free_gpt(label->gpt);
	
	if (label->parts != NULL)
		gpt_free_partitions(label->parts);
	
	free(label);
}

/** Allocate a GPT header */
gpt_t *gpt_alloc_header(size_t size)
{
	gpt_t *gpt = malloc(sizeof(gpt_t));
	if (gpt == NULL)
		return NULL;
	
	/*
	 * We might need only sizeof(gpt_header_t), but we should follow
	 * specs and have zeroes through all the rest of the block
	 */
	size_t final_size = max(size, sizeof(gpt_header_t));
	gpt->header = malloc(final_size);
	if (gpt->header == NULL) {
		free(gpt);
		return NULL;
	}
	
	memset(gpt->header, 0, final_size);
	memcpy(gpt->header->efi_signature, efi_signature, 8);
	memcpy(gpt->header->revision, revision, 4);
	gpt->header->header_size = host2uint32_t_le(final_size);
	gpt->header->entry_lba = host2uint64_t_le((uint64_t) 2);
	gpt->header->entry_size = host2uint32_t_le(sizeof(gpt_entry_t));
	
	return gpt;
}

/** Free a GPT header */
void gpt_free_gpt(gpt_t *gpt)
{
	free(gpt->header);
	free(gpt);
}

/** Read GPT from a device
 *
 * @param label      Label to read.
 * @param dev_handle Device to read GPT from.
 *
 * @return EOK on success, error code on error.
 *
 */
int gpt_read_header(gpt_label_t *label, service_id_t dev_handle)
{
	int rc = block_init(EXCHANGE_ATOMIC, dev_handle, 512);
	if (rc != EOK)
		return rc;
	
	size_t block_size;
	rc = block_get_bsize(dev_handle, &block_size);
	if (rc != EOK)
		goto end;
	
	if (label->gpt == NULL) {
		label->gpt = gpt_alloc_header(block_size);
		if (label->gpt == NULL) {
			rc = ENOMEM;
			goto end;
		}
	}
	
	rc = load_and_check_header(dev_handle, GPT_HDR_BA, block_size,
	    label->gpt->header);
	if ((rc == EBADCHECKSUM) || (rc == EINVAL)) {
		aoff64_t blocks;
		rc = block_get_nblocks(dev_handle, &blocks);
		if (rc != EOK) {
			gpt_free_gpt(label->gpt);
			goto end;
		}
		
		rc = load_and_check_header(dev_handle, blocks - 1, block_size,
		    label->gpt->header);
		if ((rc == EBADCHECKSUM) || (rc == EINVAL)) {
			gpt_free_gpt(label->gpt);
			goto end;
		}
	}
	
	label->device = dev_handle;
	rc = EOK;
	
end:
	block_fini(dev_handle);
	return rc;
}

/** Write GPT header to device
 *
 * @param label        Label to be written.
 * @param dev_handle   Device to write the GPT to.
 *
 * @return EOK on success, libblock error code otherwise.
 *
 */
int gpt_write_header(gpt_label_t *label, service_id_t dev_handle)
{
	/* The comm_size argument (the last one) is ignored */
	int rc = block_init(EXCHANGE_ATOMIC, dev_handle, 4096);
	if ((rc != EOK) && (rc != EEXIST))
		return rc;
	
	size_t block_size;
	rc = block_get_bsize(dev_handle, &block_size);
	if (rc != EOK)
		goto end;
	
	aoff64_t blocks;
	rc = block_get_nblocks(dev_handle, &blocks);
	if (rc != EOK)
		goto end;
	
	gpt_set_random_uuid(label->gpt->header->disk_guid);
	
	/* Prepare the backup header */
	label->gpt->header->alternate_lba = label->gpt->header->current_lba;
	label->gpt->header->current_lba = host2uint64_t_le(blocks - 1);
	
	uint64_t lba = label->gpt->header->entry_lba;
	label->gpt->header->entry_lba = host2uint64_t_le(blocks -
	    (uint32_t_le2host(label->gpt->header->fillries) *
	    sizeof(gpt_entry_t)) / block_size - 1);
	
	label->gpt->header->header_crc32 = 0;
	label->gpt->header->header_crc32 =
	    host2uint32_t_le(compute_crc32((uint8_t *) label->gpt->header,
	    uint32_t_le2host(label->gpt->header->header_size)));
	
	/* Write to backup GPT header location */
	rc = block_write_direct(dev_handle, blocks - 1, GPT_HDR_BS,
	    label->gpt->header);
	if (rc != EOK)
		goto end;
	
	/* Prepare the main header */
	label->gpt->header->entry_lba = lba;
	
	lba = label->gpt->header->alternate_lba;
	label->gpt->header->alternate_lba = label->gpt->header->current_lba;
	label->gpt->header->current_lba = lba;
	
	label->gpt->header->header_crc32 = 0;
	label->gpt->header->header_crc32 =
	    host2uint32_t_le(compute_crc32((uint8_t *) label->gpt->header,
	    uint32_t_le2host(label->gpt->header->header_size)));
	
	/* Write to main GPT header location */
	rc = block_write_direct(dev_handle, GPT_HDR_BA, GPT_HDR_BS,
	    label->gpt->header);
	if (rc != EOK)
		goto end;
	
	/* Write Protective MBR */
	br_block_t mbr;
	memset(&mbr, 0, 512);
	
	memset(mbr.pte[0].first_chs, 1, 3);
	mbr.pte[0].ptype = 0xEE;
	memset(mbr.pte[0].last_chs, 0xff, 3);
	mbr.pte[0].first_lba = host2uint32_t_le(1);
	mbr.pte[0].length = 0xffffffff;
	mbr.signature = host2uint16_t_le(BR_SIGNATURE);
	
	rc = block_write_direct(dev_handle, 0, 1, &mbr);
	
end:
	block_fini(dev_handle);
	return rc;
}

/** Alloc partition array */
gpt_partitions_t *gpt_alloc_partitions(void)
{
	return alloc_part_array(GPT_MIN_PART_NUM);
}

/** Parse partitions from GPT
 *
 * @param label GPT label to be parsed.
 *
 * @return EOK on success, error code otherwise.
 *
 */
int gpt_read_partitions(gpt_label_t *label)
{
	uint32_t fillries = uint32_t_le2host(label->gpt->header->fillries);
	uint32_t ent_size = uint32_t_le2host(label->gpt->header->entry_size);
	uint64_t ent_lba = uint64_t_le2host(label->gpt->header->entry_lba);
	
	if (label->parts == NULL) {
		label->parts = alloc_part_array(fillries);
		if (label->parts == NULL)
			return ENOMEM;
	}
	
	int rc = block_init(EXCHANGE_SERIALIZE, label->device,
	    sizeof(gpt_entry_t));
	if (rc != EOK) {
		gpt_free_partitions(label->parts);
		label->parts = NULL;
		goto end;
	}
	
	size_t block_size;
	rc = block_get_bsize(label->device, &block_size);
	if (rc != EOK) {
		gpt_free_partitions(label->parts);
		label->parts = NULL;
		goto end;
	}
	
	aoff64_t pos = ent_lba * block_size;
	
	for (uint32_t i = 0; i < fillries; i++) {
		rc = block_read_bytes_direct(label->device, pos, sizeof(gpt_entry_t),
		    label->parts->part_array + i);
		pos += ent_size;
		
		if (rc != EOK) {
			gpt_free_partitions(label->parts);
			label->parts = NULL;
			goto end;
		}
	}
	
	uint32_t crc = compute_crc32((uint8_t *) label->parts->part_array,
	    fillries * ent_size);
	
	if (uint32_t_le2host(label->gpt->header->pe_array_crc32) != crc) {
		rc = EBADCHECKSUM;
		gpt_free_partitions(label->parts);
		label->parts = NULL;
		goto end;
	}
	
	rc = EOK;
	
end:
	block_fini(label->device);
	return rc;
}

/** Write GPT and partitions to device
 *
 * Note: Also writes the header.
 *
 * @param label      Label to write.
 * @param dev_handle Device to write the data to.
 *
 * @return EOK on succes, error code otherwise
 *
 */
int gpt_write_partitions(gpt_label_t *label, service_id_t dev_handle)
{
	/* comm_size of 4096 is ignored */
	int rc = block_init(EXCHANGE_ATOMIC, dev_handle, 4096);
	if ((rc != EOK) && (rc != EEXIST))
		return rc;
	
	size_t block_size;
	rc = block_get_bsize(dev_handle, &block_size);
	if (rc != EOK)
		goto fail;
	
	aoff64_t blocks;
	rc = block_get_nblocks(dev_handle, &blocks);
	if (rc != EOK)
		goto fail;
	
	if (label->gpt == NULL)
		label->gpt = gpt_alloc_header(block_size);
	
	uint32_t entry_size =
	    uint32_t_le2host(label->gpt->header->entry_size);
	size_t fillries = (label->parts->fill > GPT_MIN_PART_NUM) ?
	    label->parts->fill : GPT_MIN_PART_NUM;
	
	if (entry_size != sizeof(gpt_entry_t))
		return ENOTSUP;
	
	label->gpt->header->fillries = host2uint32_t_le(fillries);
	
	uint64_t arr_blocks = (fillries * sizeof(gpt_entry_t)) / block_size;
	
	/* Include Protective MBR */
	uint64_t gpt_space = arr_blocks + GPT_HDR_BS + 1;
	
	label->gpt->header->first_usable_lba = host2uint64_t_le(gpt_space);
	label->gpt->header->last_usable_lba =
	    host2uint64_t_le(blocks - gpt_space - 1);
	
	/* Perform checks */
	gpt_part_foreach (label, p) {
		if (gpt_get_part_type(p) == GPT_PTE_UNUSED)
			continue;
		
		if (!check_encaps(p, blocks, gpt_space)) {
			rc = ERANGE;
			goto fail;
		}
		
		gpt_part_foreach (label, q) {
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
	
	label->gpt->header->pe_array_crc32 =
	    host2uint32_t_le(compute_crc32((uint8_t *) label->parts->part_array,
	    fillries * entry_size));
	
	/* Write to backup GPT partition array location */
	rc = block_write_direct(dev_handle, blocks - arr_blocks - 1,
	    arr_blocks, label->parts->part_array);
	if (rc != EOK)
		goto fail;
	
	/* Write to main GPT partition array location */
	rc = block_write_direct(dev_handle,
	    uint64_t_le2host(label->gpt->header->entry_lba),
	    arr_blocks, label->parts->part_array);
	if (rc != EOK)
		goto fail;
	
	return gpt_write_header(label, dev_handle);
	
fail:
	block_fini(dev_handle);
	return rc;
}

/** Allocate a new partition
 *
 * Note: Use either gpt_alloc_partition() or gpt_get_partition().
 * This returns a memory block (zero-filled) and needs gpt_add_partition()
 * to be called to insert it into a partition array.
 * Requires you to call gpt_free_partition afterwards.
 *
 * @return Pointer to the new partition or NULL.
 *
 */
gpt_part_t *gpt_alloc_partition(void)
{
	gpt_part_t *partition = malloc(sizeof(gpt_part_t));
	if (partition == NULL)
		return NULL;
	
	memset(partition, 0, sizeof(gpt_part_t));
	
	return partition;
}

/** Allocate a new partition already inside the label
 *
 * Note: Use either gpt_alloc_partition() or gpt_get_partition().
 * This one returns a pointer to the first empty structure already
 * inside the array, so don't call gpt_add_partition() afterwards.
 * This is the one you will usually want.
 *
 * @param label Label to carry new partition.
 *
 * @return Pointer to the new partition or NULL.
 *
 */
gpt_part_t *gpt_get_partition(gpt_label_t *label)
{
	gpt_part_t *partition;
	
	/* Find the first empty entry */
	do {
		if (label->parts->fill == label->parts->arr_size) {
			if (extend_part_array(label->parts) == -1)
				return NULL;
		}
		
		partition = label->parts->part_array + label->parts->fill++;
	} while (gpt_get_part_type(partition) != GPT_PTE_UNUSED);
	
	return partition;
}

/** Get partition already inside the label
 *
 * Note: For new partitions use either gpt_alloc_partition() or
 * gpt_get_partition() unless you want a partition at a specific place.
 * This returns a pointer to a structure already inside the array,
 * so don't call gpt_add_partition() afterwards.
 * This function is handy when you want to change already existing
 * partition or to simply write somewhere in the middle. This works only
 * for indexes smaller than either 128 or the actual number of filled
 * entries.
 *
 * @param label Label to carrying the partition.
 * @param idx   Index of the partition.
 *
 * @return Pointer to the partition or NULL when out of range.
 *
 */
gpt_part_t *gpt_get_partition_at(gpt_label_t *label, size_t idx)
{
	if ((idx >= GPT_MIN_PART_NUM) && (idx >= label->parts->fill))
		return NULL;
	
	return label->parts->part_array + idx;
}

/** Copy partition into partition array
 *
 * Note: For use with gpt_alloc_partition() only. You will get
 * duplicates with gpt_get_partition().
 * Note: Does not call gpt_free_partition()!
 *
 * @param parts     Target label
 * @param partition Source partition to copy
 *
 * @return EOK on succes, error code otherwise
 *
 */
int gpt_add_partition(gpt_label_t *label, gpt_part_t *partition)
{
	/* Find the first empty entry */
	
	gpt_part_t *part;
	
	do {
		if (label->parts->fill == label->parts->arr_size) {
			if (extend_part_array(label->parts) == -1)
				return ENOMEM;
		}
		
		part = label->parts->part_array + label->parts->fill++;
	} while (gpt_get_part_type(part) != GPT_PTE_UNUSED);
	
	memcpy(part, partition, sizeof(gpt_entry_t));
	return EOK;
}

/** Remove partition from array
 *
 * Note: Even if it fails, the partition still gets removed. Only
 * reducing the array failed.
 *
 * @param label Label to remove from
 * @param idx   Index of the partition to remove
 *
 * @return EOK on success, ENOMEM on array reduction failure
 *
 */
int gpt_remove_partition(gpt_label_t *label, size_t idx)
{
	if (idx >= label->parts->arr_size)
		return EINVAL;
	
	/*
	 * FIXME:
	 * If we allow blank spots, we break the array. If we have more than
	 * 128 partitions in the array and then remove something from
	 * the first 128 partitions, we would forget to write the last one.
	 */
	
	memset(label->parts->part_array + idx, 0, sizeof(gpt_entry_t));
	
	if (label->parts->fill > idx)
		label->parts->fill = idx;
	
	gpt_part_t *partition;
	
	if ((label->parts->fill > GPT_MIN_PART_NUM) &&
	    (label->parts->fill < (label->parts->arr_size / 2) -
	    GPT_IGNORE_FILL_NUM)) {
		for (partition = gpt_get_partition_at(label, label->parts->arr_size / 2);
		     partition < label->parts->part_array + label->parts->arr_size;
		     partition++) {
			if (gpt_get_part_type(partition) != GPT_PTE_UNUSED)
				return EOK;
		}
		
		if (reduce_part_array(label->parts) == ENOMEM)
			return ENOMEM;
	}
	
	return EOK;
}

/** Free partition list
 *
 * @param parts Partition list to be freed
 *
 */
void gpt_free_partitions(gpt_partitions_t *parts)
{
	free(parts->part_array);
	free(parts);
}

/** Get partition type */
size_t gpt_get_part_type(gpt_part_t *partition)
{
	size_t i;
	
	for (i = 0; gpt_ptypes[i].guid != NULL; i++) {
		if ((partition->part_type[3] == get_byte(gpt_ptypes[i].guid + 0)) &&
		    (partition->part_type[2] == get_byte(gpt_ptypes[i].guid + 2)) &&
		    (partition->part_type[1] == get_byte(gpt_ptypes[i].guid + 4)) &&
		    (partition->part_type[0] == get_byte(gpt_ptypes[i].guid + 6)) &&
		    (partition->part_type[5] == get_byte(gpt_ptypes[i].guid + 8)) &&
		    (partition->part_type[4] == get_byte(gpt_ptypes[i].guid + 10)) &&
		    (partition->part_type[7] == get_byte(gpt_ptypes[i].guid + 12)) &&
		    (partition->part_type[6] == get_byte(gpt_ptypes[i].guid + 14)) &&
		    (partition->part_type[8] == get_byte(gpt_ptypes[i].guid + 16)) &&
		    (partition->part_type[9] == get_byte(gpt_ptypes[i].guid + 18)) &&
		    (partition->part_type[10] == get_byte(gpt_ptypes[i].guid + 20)) &&
		    (partition->part_type[11] == get_byte(gpt_ptypes[i].guid + 22)) &&
		    (partition->part_type[12] == get_byte(gpt_ptypes[i].guid + 24)) &&
		    (partition->part_type[13] == get_byte(gpt_ptypes[i].guid + 26)) &&
		    (partition->part_type[14] == get_byte(gpt_ptypes[i].guid + 28)) &&
		    (partition->part_type[15] == get_byte(gpt_ptypes[i].guid + 30)))
			return i;
	}
	
	return i;
}

/** Set partition type */
void gpt_set_part_type(gpt_part_t *partition, size_t type)
{
	/* Beware: first 3 blocks are byteswapped! */
	partition->part_type[3] = get_byte(gpt_ptypes[type].guid + 0);
	partition->part_type[2] = get_byte(gpt_ptypes[type].guid + 2);
	partition->part_type[1] = get_byte(gpt_ptypes[type].guid + 4);
	partition->part_type[0] = get_byte(gpt_ptypes[type].guid + 6);
	
	partition->part_type[5] = get_byte(gpt_ptypes[type].guid + 8);
	partition->part_type[4] = get_byte(gpt_ptypes[type].guid + 10);
	
	partition->part_type[7] = get_byte(gpt_ptypes[type].guid + 12);
	partition->part_type[6] = get_byte(gpt_ptypes[type].guid + 14);
	
	partition->part_type[8] = get_byte(gpt_ptypes[type].guid + 16);
	partition->part_type[9] = get_byte(gpt_ptypes[type].guid + 18);
	partition->part_type[10] = get_byte(gpt_ptypes[type].guid + 20);
	partition->part_type[11] = get_byte(gpt_ptypes[type].guid + 22);
	partition->part_type[12] = get_byte(gpt_ptypes[type].guid + 24);
	partition->part_type[13] = get_byte(gpt_ptypes[type].guid + 26);
	partition->part_type[14] = get_byte(gpt_ptypes[type].guid + 28);
	partition->part_type[15] = get_byte(gpt_ptypes[type].guid + 30);
}

/** Get partition starting LBA */
uint64_t gpt_get_start_lba(gpt_part_t *partition)
{
	return uint64_t_le2host(partition->start_lba);
}

/** Set partition starting LBA */
void gpt_set_start_lba(gpt_part_t *partition, uint64_t start)
{
	partition->start_lba = host2uint64_t_le(start);
}

/** Get partition ending LBA */
uint64_t gpt_get_end_lba(gpt_part_t *partition)
{
	return uint64_t_le2host(partition->end_lba);
}

/** Set partition ending LBA */
void gpt_set_end_lba(gpt_part_t *partition, uint64_t end)
{
	partition->end_lba = host2uint64_t_le(end);
}

/** Get partition name */
unsigned char * gpt_get_part_name(gpt_part_t *partition)
{
	return partition->part_name;
}

/** Copy partition name */
void gpt_set_part_name(gpt_part_t *partition, char *name, size_t length)
{
	if (length >= 72)
		length = 71;
	
	memcpy(partition->part_name, name, length);
	partition->part_name[length] = '\0';
}

/** Get partition attribute */
bool gpt_get_flag(gpt_part_t *partition, gpt_attr_t flag)
{
	return (partition->attributes & (((uint64_t) 1) << flag)) ? 1 : 0;
}

/** Set partition attribute */
void gpt_set_flag(gpt_part_t *partition, gpt_attr_t flag, bool value)
{
	uint64_t attr = partition->attributes;
	
	if (value)
		attr = attr | (((uint64_t) 1) << flag);
	else
		attr = attr ^ (attr & (((uint64_t) 1) << flag));
	
	partition->attributes = attr;
}

/** Generate a new pseudo-random UUID compliant with RFC 4122 */
void gpt_set_random_uuid(uint8_t *uuid)
{
	srandom((unsigned int) (size_t) uuid);
	
	for (size_t i = 0; i < 16; i++)
		uuid[i] = random();

	/* 
	 * Set version (stored in bits 4-7 of seventh byte) to 4 (random
	 * UUID) and bits 6 and 7 of ninth byte to 0 and 1 respectively -
	 * according to RFC 4122, section 4.4.
	 */
	uuid[6] &= 0x0f;
	uuid[6] |= (0x4 << 4);
	uuid[8] &= 0x3f;
	uuid[8] |= (1 << 7);
}

/** Get next aligned address */
uint64_t gpt_get_next_aligned(uint64_t addr, unsigned int alignment)
{
	return ALIGN_UP(addr + 1, alignment);
}

static int load_and_check_header(service_id_t dev_handle, aoff64_t addr,
    size_t block_size, gpt_header_t *header)
{
	int rc = block_read_direct(dev_handle, addr, GPT_HDR_BS, header);
	if (rc != EOK)
		return rc;
	
	/* Check the EFI signature */
	for (unsigned int i = 0; i < 8; i++) {
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
	for (size_t i = sizeof(gpt_header_t); i < block_size; i++) {
		if (((uint8_t *) header)[i] != 0)
			return EINVAL;
	}
	
	return EOK;
}

static gpt_partitions_t *alloc_part_array(uint32_t num)
{
	gpt_partitions_t *res = malloc(sizeof(gpt_partitions_t));
	if (res == NULL)
		return NULL;
	
	uint32_t size = num > GPT_BASE_PART_NUM ? num : GPT_BASE_PART_NUM;
	res->part_array = malloc(size * sizeof(gpt_entry_t));
	if (res->part_array == NULL) {
		free(res);
		return NULL;
	}
	
	memset(res->part_array, 0, size * sizeof(gpt_entry_t));
	
	res->fill = 0;
	res->arr_size = num;
	
	return res;
}

static int extend_part_array(gpt_partitions_t *partition)
{
	size_t nsize = partition->arr_size * 2;
	gpt_entry_t *entry = malloc(nsize * sizeof(gpt_entry_t));
	if (entry == NULL)
		return ENOMEM;
	
	memcpy(entry, partition->part_array, partition->fill *
	    sizeof(gpt_entry_t));
	free(partition->part_array);
	
	partition->part_array = entry;
	partition->arr_size = nsize;
	
	return EOK;
}

static int reduce_part_array(gpt_partitions_t *partition)
{
	if (partition->arr_size > GPT_MIN_PART_NUM) {
		unsigned int nsize = partition->arr_size / 2;
		nsize = nsize > GPT_MIN_PART_NUM ? nsize : GPT_MIN_PART_NUM;
		
		gpt_entry_t *entry = malloc(nsize * sizeof(gpt_entry_t));
		if (entry == NULL)
			return ENOMEM;
		
		memcpy(entry, partition->part_array,
		    partition->fill < nsize ? partition->fill : nsize);
		free(partition->part_array);
		
		partition->part_array = entry;
		partition->arr_size = nsize;
	}
	
	return EOK;
}

/* Parse a byte from a string in hexadecimal */
static uint8_t get_byte(const char *c)
{
	uint8_t val = 0;
	char hex[3] = {*c, *(c + 1), 0};
	
	str_uint8_t(hex, NULL, 16, false, &val);
	return val;
}

static bool check_overlap(gpt_part_t *part1, gpt_part_t *part2)
{
	if ((gpt_get_start_lba(part1) < gpt_get_start_lba(part2)) &&
	    (gpt_get_end_lba(part1) < gpt_get_start_lba(part2)))
		return false;
	
	if ((gpt_get_start_lba(part1) > gpt_get_start_lba(part2)) &&
	    (gpt_get_end_lba(part2) < gpt_get_start_lba(part1)))
		return false;
	
	return true;
}

static bool check_encaps(gpt_part_t *part, uint64_t blocks,
    uint64_t first_lba)
{
	/*
	 * We allow "<=" in the second expression because it lacks
	 * MBR so it is smaller by 1 block.
	 */
	if ((gpt_get_start_lba(part) >= first_lba) &&
	    (gpt_get_end_lba(part) <= blocks - first_lba))
		return true;
	
	return false;
}
