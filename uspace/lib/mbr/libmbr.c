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
 * - The LIBMBR_NAME of the author may not be used to endorse or promote products
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

/** @addtogroup libmbr
 * @{
 */
/** @file MBR extraxtion library
 */

#include <async.h>
#include <assert.h>
#include <block.h>
#include <byteorder.h>
#include <errno.h>
#include <ipc/bd.h>
#include <mem.h>
#include <stdio.h>
#include <stdlib.h>
#include <str_error.h>
#include <align.h>
#include "libmbr.h"

static br_block_t *alloc_br(void);
static int decode_part(pt_entry_t *, mbr_part_t *, uint32_t);
static int decode_logical(mbr_label_t *, mbr_part_t *);
static void encode_part(mbr_part_t *, pt_entry_t *, uint32_t, bool);
static bool check_overlap(mbr_part_t *, mbr_part_t *);
static bool check_encaps(mbr_part_t *, mbr_part_t *);
static bool check_preceeds(mbr_part_t *, mbr_part_t *);
static mbr_err_val mbr_add_primary(mbr_label_t *, mbr_part_t *);
static mbr_err_val mbr_add_logical(mbr_label_t *, mbr_part_t *);

/** Allocate and initialize mbr_label_t structure */
mbr_label_t *mbr_alloc_label(void)
{
	mbr_label_t *label = malloc(sizeof(mbr_label_t));
	if (label == NULL)
		return NULL;
	
	label->mbr = NULL;
	label->parts = NULL;
	label->device = 0;
	
	return label;
}

void mbr_set_device(mbr_label_t *label, service_id_t dev_handle)
{
	label->device = dev_handle;
}

/** Free mbr_label_t structure */
void mbr_free_label(mbr_label_t *label)
{
	if (label->mbr != NULL)
		mbr_free_mbr(label->mbr);
	
	if (label->parts != NULL)
		mbr_free_partitions(label->parts);
	
	free(label);
}

/** Allocate memory for mbr_t */
mbr_t *mbr_alloc_mbr(void)
{
	return (mbr_t *)alloc_br();
}

/** Read MBR from specific device
 *
 * @param label      Label to be read.
 * @param dev_handle Device to read MBR from.
 *
 * @return EOK on success, error code on error.
 *
 */
int mbr_read_mbr(mbr_label_t *label, service_id_t dev_handle)
{	
	if (label->mbr == NULL) {
		label->mbr = mbr_alloc_mbr();
		if (label->mbr == NULL)
			return ENOMEM;
	}
	
	int rc = block_init(EXCHANGE_ATOMIC, dev_handle, 512);
	if (rc != EOK)
		return rc;
	
	rc = block_read_direct(dev_handle, 0, 1, &label->mbr->raw_data);
	block_fini(dev_handle);
	if (rc != EOK)
		return rc;
	
	label->device = dev_handle;
	
	return EOK;
}

/** Write MBR to specific device
 *
 * @param label      Label to be written.
 * @param dev_handle Device to write MBR to.
 *
 * @return EOK on success, error code on error.
 *
 */
int mbr_write_mbr(mbr_label_t *label, service_id_t dev_handle)
{
	int rc = block_init(EXCHANGE_ATOMIC, dev_handle, 512);
	if (rc != EOK)
		return rc;
	
	rc = block_write_direct(dev_handle, 0, 1, &label->mbr->raw_data);
	block_fini(dev_handle);
	
	return rc;
}

/** Decide whether this is an actual MBR or a Protective MBR for GPT
 *
 * @param label Label to decide upon.
 *
 * @return True if MBR.
 * @return False if Protective MBR for GPT.
 *
 */
int mbr_is_mbr(mbr_label_t *label)
{
	return (label->mbr->raw_data.pte[0].ptype != PT_GPT);
}

/** Parse partitions from MBR (freeing previous partitions if any)
 *
 * It is assumed that mbr_read_mbr() was called before.
 *
 * @param label Label to be parsed.
 *
 * @return EOK on success, error code on error.
 *
 */
int mbr_read_partitions(mbr_label_t *label)
{
	if ((label == NULL) || (label->mbr == NULL))
		return EINVAL;
	
	if (label->parts != NULL)
		mbr_free_partitions(label->parts);
	
	label->parts = mbr_alloc_partitions();
	if (label->parts == NULL)
		return ENOMEM;
	
	mbr_part_t *extended = NULL;
	
	/* Generate the primary partitions */
	for (unsigned int i = 0; i < N_PRIMARY; i++) {
		if (label->mbr->raw_data.pte[i].ptype == PT_UNUSED)
			continue;
		
		mbr_part_t *partition = mbr_alloc_partition();
		if (partition == NULL) {
			mbr_free_partitions(label->parts);
			return ENOMEM;
		}
		
		int is_extended =
		    decode_part(&label->mbr->raw_data.pte[i], partition, 0);
		
		mbr_set_flag(partition, ST_LOGIC, false);
		
		int rc = mbr_add_partition(label, partition);
		if (rc != ERR_OK) {
			mbr_free_partitions(label->parts);
			return EINVAL;
		}
		
		if (is_extended) {
			extended = partition;
			label->parts->l_extended = &partition->link;
		}
	}
	
	/* Fill in the primary partitions and generate logical ones (if any) */
	return decode_logical(label, extended);
}

/** Write MBR and partitions to device
 *
 * @param label      Label to write.
 * @param dev_handle Device to write the data to.
 *
 * @return EOK on success, specific error code otherwise.
 *
 */
int mbr_write_partitions(mbr_label_t *label, service_id_t dev_handle)
{
	if (label->parts == NULL)
		return EOK;
	
	if (label->mbr == NULL)
		label->mbr = mbr_alloc_mbr();
	
	int rc = block_init(EXCHANGE_ATOMIC, dev_handle, 512);
	if (rc != EOK)
		return rc;
	
	mbr_part_t *partition = NULL;
	mbr_part_t *extended = NULL;
	
	if (label->parts->l_extended != NULL)
		extended = list_get_instance(label->parts->l_extended,
		    mbr_part_t, link);
	
	link_t *link = label->parts->list.head.next;
	
	/* Encode primary partitions */
	for (unsigned int i = 0; i < N_PRIMARY; i++) {
		partition = list_get_instance(link, mbr_part_t, link);
		
		encode_part(partition, &label->mbr->raw_data.pte[i], 0, false);
		link = link->next;
	}
	
	/* Write MBR */
	rc = block_write_direct(dev_handle, 0, 1, &label->mbr->raw_data);
	if ((rc != EOK) || (extended == NULL))
		goto end;
	
	uint32_t base = extended->start_addr;
	mbr_part_t *prev_partition;
	
	/* Encode and write first logical partition */
	if (link != &label->parts->list.head) {
		partition = list_get_instance(link, mbr_part_t, link);
		
		partition->ebr_addr = base;
		encode_part(partition, &partition->ebr->pte[0], base, false);
		link = link->next;
	} else {
		/*
		 * If there was an extended partition but no logical partitions,
		 * we should overwrite the space where the first logical
		 * partitions's EBR would have been. There might be some
		 * garbage from the past.
		 */
		
		br_block_t *br = alloc_br();
		rc = block_write_direct(dev_handle, base, 1, br);
		if (rc != EOK)
			goto end;
		
		free(br);
		goto end;
	}
	
	prev_partition = partition;
	
	/*
	 * Check EBR addresses: The code saves previous EBR
	 * placements from other software. But if our user
	 * modifies the logical partition chain, we have to
	 * fix those placements if needed.
	 */
	
	link_t *link_ebr = link;
	link_t *link_iter;
	
	mbr_part_t tmp_partition;
	tmp_partition.length = 1;
	
	while (link_ebr != &label->parts->list.head) {
		partition = list_get_instance(link_ebr, mbr_part_t, link);
		
		tmp_partition.start_addr = partition->ebr_addr;
		
		link_iter = link;
		while (link_iter != &label->parts->list.head) {
			/*
			 * Check whether EBR address makes sense. If not, we take
			 * a guess.  So far this is simple, we just take the first
			 * preceeding sector. FDisk always reserves at least 2048
			 * sectors (1 MiB), so it can have the EBR aligned as well
			 * as the partition itself. Parted reserves minimum one
			 * sector, like we do.
			 *
			 * Note that we know there is at least one sector free from
			 * previous checks. Also note that the user can set ebr_addr
			 * to their liking (if it is valid).
			 */
			
			if ((partition->ebr_addr < base) ||
			    (partition->ebr_addr >= base + extended->length) ||
			    (check_overlap(&tmp_partition,
			    list_get_instance(link_iter, mbr_part_t, link)))) {
				partition->ebr_addr = partition->start_addr - 1;
				break;
			}
			
			link_iter = link_iter->next;
		}
		
		link_ebr = link_ebr->next;
	}
	
	/* Encode and write logical partitions */
	while (link != &label->parts->list.head) {
		partition = list_get_instance(link, mbr_part_t, link);
		
		encode_part(partition, &partition->ebr->pte[0],
		    partition->ebr_addr, false);
		encode_part(partition, &prev_partition->ebr->pte[1],
		    base, true);
		
		rc = block_write_direct(dev_handle, prev_partition->ebr_addr, 1,
		    prev_partition->ebr);
		if (rc != EOK)
			goto end;
		
		prev_partition = partition;
		link = link->next;
	}
	
	/* Write the last EBR */
	encode_part(NULL, &prev_partition->ebr->pte[1], 0, false);
	rc = block_write_direct(dev_handle, prev_partition->ebr_addr,
	    1, prev_partition->ebr);
	
end:
	block_fini(dev_handle);
	return rc;
}

/** Partition constructor */
mbr_part_t *mbr_alloc_partition(void)
{
	mbr_part_t *partition = malloc(sizeof(mbr_part_t));
	if (partition == NULL)
		return NULL;
	
	link_initialize(&partition->link);
	partition->ebr = NULL;
	partition->type = PT_UNUSED;
	partition->status = 0;
	partition->start_addr = 0;
	partition->length = 0;
	partition->ebr_addr = 0;
	
	return partition;
}

/** Partitions constructor */
mbr_partitions_t *mbr_alloc_partitions(void)
{
	mbr_partitions_t *parts = malloc(sizeof(mbr_partitions_t));
	if (parts == NULL)
		return NULL;
	
	list_initialize(&parts->list);
	parts->n_primary = 0;
	parts->n_logical = 0;
	parts->l_extended = NULL;
	
	/* Add blank primary partitions */
	for (unsigned int i = 0; i < N_PRIMARY; ++i) {
		mbr_part_t *part = mbr_alloc_partition();
		if (part == NULL) {
			mbr_free_partitions(parts);
			return NULL;
		}
		
		list_append(&part->link, &parts->list);
	}
	
	return parts;
}

/** Add partition
 *
 * Perform checks, sort the list.
 *
 * @param label Label to add to.
 * @param part  Partition to add.
 *
 * @return ERR_OK on success, other MBR_ERR_VAL otherwise
 *
 */
mbr_err_val mbr_add_partition(mbr_label_t *label, mbr_part_t *part)
{
	int rc = block_init(EXCHANGE_ATOMIC, label->device, 512);
	if ((rc != EOK) && (rc != EEXIST))
		return ERR_LIBBLOCK;
	
	aoff64_t nblocks;
	int ret = block_get_nblocks(label->device, &nblocks);
	
	if (rc != EEXIST)
		block_fini(label->device);
	
	if (ret != EOK)
		return ERR_LIBBLOCK;
	
	if ((aoff64_t) part->start_addr + part->length > nblocks)
		return ERR_OUT_BOUNDS;
	
	if (label->parts == NULL) {
		label->parts = mbr_alloc_partitions();
		if (label->parts == NULL)
			// FIXME! merge mbr_err_val into errno.h
			return ENOMEM;
	}
	
	if (mbr_get_flag(part, ST_LOGIC))
		return mbr_add_logical(label, part);
	else
		return mbr_add_primary(label, part);
}

/** Remove partition
 *
 * Remove partition (indexed from zero). When removing the extended
 * partition, all logical partitions get removed as well.
 *
 * @param label Label to remove from.
 * @param idx   Index of the partition to remove.
 *
 * @return EOK on success.
 * @return EINVAL if the index is invalid.
 *
 */
int mbr_remove_partition(mbr_label_t *label, size_t idx)
{
	link_t *link = list_nth(&label->parts->list, idx);
	if (link == NULL)
		return EINVAL;
	
	/*
	 * If removing the extended partition, remove all
	 * logical partitions as well.
	 */
	if (link == label->parts->l_extended) {
		label->parts->l_extended = NULL;
		
		link_t *iterator = link->next;
		link_t *next;
		
		while (iterator != &label->parts->list.head) {
			next = iterator->next;
			mbr_part_t *partition =
			    list_get_instance(iterator, mbr_part_t, link);
			
			if (mbr_get_flag(partition, ST_LOGIC)) {
				list_remove(iterator);
				label->parts->n_logical--;
				mbr_free_partition(partition);
			}
			
			iterator = next;
		}
	}
	
	/* Remove the partition itself */
	mbr_part_t *partition =
	    list_get_instance(link, mbr_part_t, link);
	
	if (mbr_get_flag(partition, ST_LOGIC)) {
		label->parts->n_logical--;
		list_remove(link);
		mbr_free_partition(partition);
	} else {
		/*
		 * Cannot remove a primary partition without
		 * breaking the ordering. Just zero it.
		 */
		label->parts->n_primary--;
		partition->type = 0;
		partition->status = 0;
		partition->start_addr = 0;
		partition->length = 0;
		partition->ebr_addr = 0;
	}
	
	return EOK;
}

/** Partition destructor */
void mbr_free_partition(mbr_part_t *partition)
{
	if (partition->ebr != NULL)
		free(partition->ebr);
	
	free(partition);
}

/** Check for flag */
int mbr_get_flag(mbr_part_t *partition, mbr_flags_t flag)
{
	return (partition->status & (1 << flag));
}

/** Set a specific status flag */
void mbr_set_flag(mbr_part_t *partition, mbr_flags_t flag, bool set)
{
	if (set)
		partition->status |= 1 << flag;
	else
		partition->status &= ~((uint16_t) (1 << flag));
}

/** Get next aligned address */
uint32_t mbr_get_next_aligned(uint32_t addr, unsigned int alignment)
{
	return ALIGN_UP(addr + 1, alignment);
}

list_t *mbr_get_list(mbr_label_t *label)
{
	if (label->parts != NULL)
		return &label->parts->list;
	else
		return NULL;
}

mbr_part_t *mbr_get_first_partition(mbr_label_t *label)
{
	list_t *list = mbr_get_list(label);
	if ((list != NULL) && (!list_empty(list)))
		return list_get_instance(list->head.next, mbr_part_t, link);
	else
		return NULL;
}

mbr_part_t *mbr_get_next_partition(mbr_label_t *label, mbr_part_t *partition)
{
	list_t *list = mbr_get_list(label);
	if ((list != NULL) && (&partition->link != list_last(list)))
		return list_get_instance(partition->link.next, mbr_part_t, link);
	else
		return NULL;
}

void mbr_free_mbr(mbr_t *mbr)
{
	free(mbr);
}

/** Free partition list
 *
 * @param parts Partition list to be freed
 *
 */
void mbr_free_partitions(mbr_partitions_t *parts)
{
	list_foreach_safe(parts->list, cur_link, next) {
		mbr_part_t *partition = list_get_instance(cur_link, mbr_part_t, link);
		list_remove(cur_link);
		mbr_free_partition(partition);
	}
	
	free(parts);
}

static br_block_t *alloc_br(void)
{
	br_block_t *br = malloc(sizeof(br_block_t));
	if (br == NULL)
		return NULL;
	
	memset(br, 0, 512);
	br->signature = host2uint16_t_le(BR_SIGNATURE);
	
	return br;
}

/** Decode partition entry */
static int decode_part(pt_entry_t *src, mbr_part_t *partition, uint32_t base)
{
	partition->type = src->ptype;
	partition->status = (partition->status & 0xff00) | (uint16_t) src->status;
	partition->start_addr = uint32_t_le2host(src->first_lba) + base;
	partition->length = uint32_t_le2host(src->length);
	
	return (src->ptype == PT_EXTENDED);
}

/** Parse logical partitions */
static int decode_logical(mbr_label_t *label, mbr_part_t *extended)
{
	if (extended == NULL)
		return EOK;
	
	br_block_t *ebr = alloc_br();
	if (ebr == NULL)
		return ENOMEM;
	
	uint32_t base = extended->start_addr;
	uint32_t addr = base;
	
	int rc = block_init(EXCHANGE_ATOMIC, label->device, 512);
	if (rc != EOK)
		goto end;
	
	rc = block_read_direct(label->device, addr, 1, ebr);
	if (rc != EOK)
		goto end;
	
	if (uint16_t_le2host(ebr->signature) != BR_SIGNATURE) {
		rc = EINVAL;
		goto end;
	}
	
	if (ebr->pte[0].ptype == PT_UNUSED) {
		rc = EOK;
		goto end;
	}
	
	mbr_part_t *partition = mbr_alloc_partition();
	if (partition == NULL) {
		rc = ENOMEM;
		goto end;
	}
	
	decode_part(&ebr->pte[0], partition, base);
	mbr_set_flag(partition, ST_LOGIC, true);
	partition->ebr = ebr;
	partition->ebr_addr = addr;
	
	rc = mbr_add_partition(label, partition);
	if (rc != ERR_OK)
		goto end;
	
	addr = uint32_t_le2host(ebr->pte[1].first_lba) + base;
	
	while (ebr->pte[1].ptype != PT_UNUSED) {
		rc = block_read_direct(label->device, addr, 1, ebr);
		if (rc != EOK)
			goto end;
		
		if (uint16_t_le2host(ebr->signature) != BR_SIGNATURE) {
			rc = EINVAL;
			goto end;
		}
		
		mbr_part_t *partition = mbr_alloc_partition();
		if (partition == NULL) {
			rc = ENOMEM;
			goto end;
		}
		
		decode_part(&ebr->pte[0], partition, addr);
		mbr_set_flag(partition, ST_LOGIC, true);
		partition->ebr = ebr;
		partition->ebr_addr = addr;
		
		rc = mbr_add_partition(label, partition);
		if (rc != ERR_OK)
			goto end;
		
		addr = uint32_t_le2host(ebr->pte[1].first_lba) + base;
	}
	
	rc = EOK;
	
end:
	// FIXME possible memory leaks
	block_fini(label->device);
	
	return rc;
}

/** Encode partition entry */
static void encode_part(mbr_part_t *src, pt_entry_t *entry, uint32_t base,
    bool ebr)
{
	if (src != NULL) {
		entry->status = (uint8_t) src->status & 0xff;
		
		/* Ignore CHS */
		entry->first_chs[0] = 0xfe;
		entry->first_chs[1] = 0xff;
		entry->first_chs[2] = 0xff;
		entry->last_chs[0] = 0xfe;
		entry->last_chs[1] = 0xff;
		entry->last_chs[2] = 0xff;
		
		if (ebr) {
			/* Encode reference to EBR */
			entry->ptype = PT_EXTENDED_LBA;
			entry->first_lba = host2uint32_t_le(src->ebr_addr - base);
			entry->length = host2uint32_t_le(src->length + src->start_addr -
			    src->ebr_addr);
		} else {
			/* Encode reference to partition */
			entry->ptype = src->type;
			entry->first_lba = host2uint32_t_le(src->start_addr - base);
			entry->length = host2uint32_t_le(src->length);
		}
		
		if (entry->ptype == PT_UNUSED)
			memset(entry, 0, sizeof(pt_entry_t));
	} else
		memset(entry, 0, sizeof(pt_entry_t));
}

/** Check whether two partitions overlap */
static bool check_overlap(mbr_part_t *part1, mbr_part_t *part2)
{
	if ((part1->start_addr < part2->start_addr) &&
	    (part1->start_addr + part1->length <= part2->start_addr))
		return false;
	
	if ((part1->start_addr > part2->start_addr) &&
	    (part2->start_addr + part2->length <= part1->start_addr))
		return false;
	
	return true;
}

/** Check whether one partition encapsulates the other */
static bool check_encaps(mbr_part_t *inner, mbr_part_t *outer)
{
	if ((inner->start_addr <= outer->start_addr) ||
	    (outer->start_addr + outer->length <= inner->start_addr))
		return false;
	
	if (outer->start_addr + outer->length < inner->start_addr + inner->length)
		return false;
	
	return true;
}

/** Check whether one partition preceeds the other */
static bool check_preceeds(mbr_part_t *preceeder, mbr_part_t *precedee)
{
	return preceeder->start_addr < precedee->start_addr;
}

mbr_err_val mbr_add_primary(mbr_label_t *label, mbr_part_t *part)
{
	if (label->parts->n_primary == 4)
		return ERR_PRIMARY_FULL;
	
	/* Check if partition makes space for MBR itself */
	if (part->start_addr == 0)
		return ERR_OUT_BOUNDS;
	
	/* If it is an extended partition, is there any other one? */
	if (((part->type == PT_EXTENDED) || (part->type == PT_EXTENDED_LBA)) &&
	    (label->parts->l_extended != NULL))
		return ERR_EXTENDED_PRESENT;
	
	/* Find a place and add it */
	mbr_part_t *iter;
	mbr_part_t *empty = NULL;
	mbr_part_foreach(label, iter) {
		if (iter->type == PT_UNUSED) {
			if (empty == NULL)
				empty = iter;
		} else if (check_overlap(part, iter))
			return ERR_OVERLAP;
	}
	
	list_insert_after(&part->link, &empty->link);
	list_remove(&empty->link);
	free(empty);
	
	label->parts->n_primary++;
	
	if ((part->type == PT_EXTENDED) || (part->type == PT_EXTENDED_LBA))
		label->parts->l_extended = &part->link;
	
	return EOK;
}

mbr_err_val mbr_add_logical(mbr_label_t *label, mbr_part_t *part)
{
	/* Is there any extended partition? */
	if (label->parts->l_extended == NULL)
		return ERR_NO_EXTENDED;
	
	/* Is the logical partition inside the extended partition? */
	mbr_part_t *extended = list_get_instance(label->parts->l_extended, mbr_part_t, link);
	if (!check_encaps(part, extended))
		return ERR_OUT_BOUNDS;
	
	/* Find a place for the new partition in a sorted linked list */
	bool first_logical = true;
	mbr_part_t *iter;
	mbr_part_foreach (label, iter) {
		if (mbr_get_flag(iter, ST_LOGIC)) {
			if (check_overlap(part, iter))
				return ERR_OVERLAP;
			
			if (check_preceeds(iter, part)) {
				/* Check if there is at least one sector of space preceeding */
				if ((iter->start_addr + iter->length) >= part->start_addr - 1)
					return ERR_NO_EBR;
			} else if (first_logical) {
				/*
				 * First logical partition's EBR is before every other
				 * logical partition. Thus we do not check if this partition
				 * leaves enough space for it.
				 */
				first_logical = false;
			} else {
				/*
				 * Check if there is at least one sector of space following
				 * (for following partitions's EBR).
				 */
				if ((part->start_addr + part->length) >= iter->start_addr - 1)
					return ERR_NO_EBR;
			}
		}
	}
	
	/* Allocate EBR if it is not already there */
	if (part->ebr == NULL) {
		part->ebr = alloc_br();
		if (part->ebr == NULL)
			return ERR_NOMEM;
	}
	
	list_append(&part->link, &label->parts->list);
	label->parts->n_logical++;
	
	return EOK;
}
