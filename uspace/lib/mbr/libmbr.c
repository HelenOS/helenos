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

#include "libmbr.h"

static br_block_t * alloc_br(void);
static int decode_part(pt_entry_t * src, mbr_part_t * trgt, uint32_t base);
static int decode_logical(mbr_t * mbr, mbr_partitions_t * p, mbr_part_t * ext);
static void encode_part(mbr_part_t * src, pt_entry_t * trgt, uint32_t base, bool ebr);
static int check_overlap(mbr_part_t * p1, mbr_part_t * p2);
static int check_encaps(mbr_part_t * inner, mbr_part_t * outer);
static int check_preceeds(mbr_part_t * preceeder, mbr_part_t * precedee);

/** Allocate memory for mbr_t */
mbr_t * mbr_alloc_mbr()
{
	return alloc_br();
}

/** Read MBR from specific device
 * @param	dev_handle	device to read MBR from
 *
 * @return				mbr record on success, NULL on error
 */
mbr_t * mbr_read_mbr(service_id_t dev_handle)
{
	int rc;

	mbr_t * mbr = malloc(sizeof(mbr_t));
	if (mbr == NULL) {
		return NULL;
	}

	rc = block_init(EXCHANGE_ATOMIC, dev_handle, 512);
	if (rc != EOK) {
		free(mbr);
		return NULL;
	}

	rc = block_read_direct(dev_handle, 0, 1, &(mbr->raw_data));
	if (rc != EOK) {
		free(mbr);
		block_fini(dev_handle);
		return NULL;
	}

	block_fini(dev_handle);

	mbr->device = dev_handle;

	return mbr;
}

/** Write mbr to disk
 * @param mbr			MBR to be written
 * @param dev_handle	device handle to write MBR to (may be different
 * 							from the device in 'mbr')
 *
 * @return				0 on success, otherwise libblock error code
 */
int mbr_write_mbr(mbr_t * mbr, service_id_t dev_handle)
{
	int rc;

	rc = block_init(EXCHANGE_ATOMIC, dev_handle, 512);
	if (rc != EOK) {
		return rc;
	}

	rc = block_write_direct(dev_handle, 0, 1, &(mbr->raw_data));
	block_fini(dev_handle);
	if (rc != EOK) {
		return rc;
	}

	return 0;
}

/** Decide whether this is an actual MBR or a Protective MBR from GPT
 *
 * @param mbr		the actual MBR to decide upon
 *
 * @return			1 if MBR, 0 if GPT
 */
int mbr_is_mbr(mbr_t * mbr)
{
	return (mbr->raw_data.pte[0].ptype != PT_GPT) ? 1 : 0;
}

/** Parse partitions from MBR
 * @param mbr	MBR to be parsed
 *
 * @return 		linked list of partitions or NULL on error
 */
mbr_partitions_t * mbr_read_partitions(mbr_t * mbr)
{
	int rc, i, rc_ext;
	mbr_part_t * p;
	mbr_part_t * ext = NULL;
	mbr_partitions_t * parts;
	
	if (mbr == NULL)
		return NULL;
	
	parts = mbr_alloc_partitions();
	if (parts == NULL) {
		return NULL;
	}

	// Generate the primary partitions
	for (i = 0; i < N_PRIMARY; ++i) {
		if (mbr->raw_data.pte[i].ptype == PT_UNUSED)
			continue;
		
		p = mbr_alloc_partition();
		if (p == NULL) {
			printf(LIBMBR_NAME ": Error on memory allocation.\n");
			mbr_free_partitions(parts);
			return NULL;
		}
		
		rc_ext = decode_part(&(mbr->raw_data.pte[i]), p, 0);
		mbr_set_flag(p, ST_LOGIC, false);
		rc = mbr_add_partition(parts, p);
		if (rc != ERR_OK) {
			printf(LIBMBR_NAME ": Error occured during decoding the MBR. (%d)\n" \
				   LIBMBR_NAME ": Partition list may be incomplete.\n", rc);
			return NULL;
		}
		
		if (rc_ext) {
			ext = p;
			parts->l_extended = list_last(&(parts->list));
		}
	}
	
	// Fill in the primary partitions and generate logical ones, if any
	rc = decode_logical(mbr, parts, ext);
	if (rc != EOK) {
		printf(LIBMBR_NAME ": Error occured during decoding the MBR.\n" \
			   LIBMBR_NAME ": Partition list may be incomplete.\n");
	}
	
	return parts;
}

/** Write MBR and partitions to device
 * @param parts			partition list to be written
 * @param mbr			MBR to be written with 'parts' partitions
 * @param dev_handle	device to write the data to
 *
 * @return				returns EOK on succes, specific error code otherwise
 */
int mbr_write_partitions(mbr_partitions_t * parts, mbr_t * mbr, service_id_t dev_handle)
{
	int i = 0;
	int rc;
	mbr_part_t * p;
	mbr_part_t * ext = (parts->l_extended == NULL) ? NULL
					: list_get_instance(parts->l_extended, mbr_part_t, link);
	
	rc = block_init(EXCHANGE_ATOMIC, dev_handle, 512);
	if (rc != EOK) {
		printf(LIBMBR_NAME ": Error while initializing libblock: %d - %s.\n", rc, str_error(rc));
		return rc;
	}
	
	link_t * l = parts->list.head.next;
	
	// Encoding primary partitions
	for (i = 0; i < parts->n_primary; i++) {
		p = list_get_instance(l, mbr_part_t, link);
		encode_part(p, &(mbr->raw_data.pte[i]), 0, false);
		l = l->next;
	}
	
	// Writing MBR
	rc = block_write_direct(dev_handle, 0, 1, &(mbr->raw_data));
	if (rc != EOK) {
		printf(LIBMBR_NAME ": Error while writing MBR : %d - %s.\n", rc, str_error(rc));
		goto end;
	}
	
	if (ext == NULL)
		goto end;
	
	uint32_t base = ext->start_addr;
	mbr_part_t * prev_p;
	
	/* Note for future changes: Some thought has been put into design
	 * and implementation. If you don't have to change it, don't. Other
	 * designs have been tried, this came out as the least horror with
	 * as much power over it as you can get. Thanks. */
	
	// Encoding and writing first logical partition
	if (l != &(parts->list.head)) {
		p = list_get_instance(l, mbr_part_t, link);
		p->ebr_addr = base;
		encode_part(p, &(p->ebr->pte[0]), base, false);
		l = l->next;
	} else {
		/* If there was an extended but no logical, we should overwrite
		 * the space where the first logical's EBR would have been. There
		 * might be some garbage from the past. */
		br_block_t * tmp = alloc_br();
		rc = block_write_direct(dev_handle, base, 1, tmp);
		if (rc != EOK) {
			printf(LIBMBR_NAME ": Error while writing EBR: %d - %s.\n", rc, str_error(rc));
			goto end;
		}
		free(tmp);
		goto end;
	}
	
	prev_p = p;
	
	// Encoding and writing logical partitions
	while (l != &(parts->list.head)) {
		p = list_get_instance(l, mbr_part_t, link);
		
		/* Checking whether EBR address makes sense. If not, we take a guess.
		 * So far this is simple, we just take the first preceeding sector.
		 * Fdisk always reserves at least 2048 sectors (1MiB), so it can have 
		 * the EBR aligned as well as the partition itself. Parted reserves
		 * minimum one sector, like we do.
		 * 
		 * Note that we know there is at least one sector free from previous checks.
		 * Also note that the user can set ebr_addr to their liking (if it's valid). */		 
		if (p->ebr_addr >= p->start_addr || p->ebr_addr <= (prev_p->start_addr + prev_p->length)) {
			p->ebr_addr = p->start_addr - 1;
			DEBUG_PRINT_0(LIBMBR_NAME ": Warning: invalid EBR address.\n");
		}
		
		encode_part(p, &(p->ebr->pte[0]), p->ebr_addr, false);
		encode_part(p, &(prev_p->ebr->pte[1]), base, true);
		
		rc = block_write_direct(dev_handle, prev_p->ebr_addr, 1, prev_p->ebr);
		if (rc != EOK) {
			printf(LIBMBR_NAME ": Error while writing EBR: %d - %s.\n", rc, str_error(rc));
			goto end;
		}
		
		prev_p = p;
		l = l->next;
	}
	
	// write the last EBR
	encode_part(NULL, &(prev_p->ebr->pte[1]), 0, false);
	rc = block_write_direct(dev_handle, prev_p->ebr_addr, 1, prev_p->ebr);
	if (rc != EOK) {
		printf(LIBMBR_NAME ": Error while writing EBR: %d - %s.\n", rc, str_error(rc));
		goto end;
	}
	
	rc = EOK;
	
end:
	block_fini(dev_handle);
	
	return rc;
}

/** mbr_part_t constructor */
mbr_part_t * mbr_alloc_partition(void)
{
	mbr_part_t * p = malloc(sizeof(mbr_part_t));
	if (p == NULL) {
		return NULL;
	}
	
	link_initialize(&(p->link));
	p->ebr = NULL;
	p->type = 0;
	p->status = 0;
	p->start_addr = 0;
	p->length = 0;
	p->ebr_addr = 0;

	return p;
}

/** mbr_partitions_t constructor */
mbr_partitions_t * mbr_alloc_partitions(void)
{
	mbr_partitions_t * parts = malloc(sizeof(mbr_partitions_t));
	if (parts == NULL) {
		return NULL;
	}

	list_initialize(&(parts->list));
	parts->n_primary = 0;
	parts->n_logical = 0;
	parts->l_extended = NULL;

	return parts;
}

/** Add partition
 * 	Performs checks, sorts the list.
 * 
 * @param parts			partition list to add to
 * @param p				partition to add
 * 
 * @return				ERR_OK (0) on success, other MBR_ERR_VAL otherwise
 */
MBR_ERR_VAL mbr_add_partition(mbr_partitions_t * parts, mbr_part_t * p)
{
	if (mbr_get_flag(p, ST_LOGIC)) { // adding logical part
		// is there any extended partition?
		if (parts->l_extended == NULL)
			return ERR_NO_EXTENDED;
		
		// is the logical partition inside the extended one?
		mbr_part_t * ext = list_get_instance(parts->l_extended, mbr_part_t, link);
		if (!check_encaps(p, ext))
			return ERR_OUT_BOUNDS;
		
		// find a place for the new partition in a sorted linked list
		mbr_part_t * last = list_get_instance(list_last(&(parts->list)), mbr_part_t, link);
		mbr_part_t * iter;
		uint32_t ebr_space = 1;
		mbr_part_foreach(parts, iter) {
			if (mbr_get_flag(iter, ST_LOGIC)) {
				if (check_overlap(p, iter)) 
					return ERR_OVERLAP;
				if (check_preceeds(iter, p)) {
					last = iter;
					ebr_space = p->start_addr - (last->start_addr + last->length);
				} else
					break;
			}
		}
		
		// checking if there's at least one sector of space preceeding
		if (ebr_space < 1)
			return ERR_NO_EBR;
		
		// checking if there's at least one sector of space following (for following partitions's EBR)
		if (last->link.next != &(parts->list.head)) {
			if (list_get_instance(&(last->link.next), mbr_part_t, link)->start_addr <= p->start_addr + p->length + 1)
				return ERR_NO_EBR;
		}
		
		// alloc EBR if it's not already there
		if (p->ebr == NULL) {
			p->ebr = alloc_br();
			if (p->ebr == NULL) {
				return ERR_NOMEM;
			}
		}
		
		// add it
		list_insert_after(&(p->link), &(last->link));
		parts->n_logical += 1;
	} else { // adding primary
		if (parts->n_primary == 4) {
			return ERR_PRIMARY_FULL;
		}
		
		// TODO: should we check if it's inside the drive's upper boundary?
		if (p->start_addr == 0) {
			return ERR_OUT_BOUNDS;
		}
		
		// if it's extended, is there any other one?
		if (p->type == PT_EXTENDED && parts->l_extended != NULL) {
			return ERR_EXTENDED_PRESENT;
		}
		
		// find a place and add it
		if (list_empty(&(parts->list))) {
			list_append(&(p->link), &(parts->list));
		} else {
			mbr_part_t * iter;
			mbr_part_foreach(parts, iter) {
				if (mbr_get_flag(iter, ST_LOGIC)) {
					list_insert_before(&(p->link), &(iter->link));
					break;
				} else if (check_overlap(p, iter))
					return ERR_OVERLAP;
			}
			if (iter == list_get_instance(&(parts->list.head.prev), mbr_part_t, link))
				list_append(&(p->link), &(parts->list));
		}
		parts->n_primary += 1;
	}

	return ERR_OK;
}

/** Remove partition
 * 	Removes partition by index, indexed from zero. When removing extended
 * partition, all logical partitions get removed as well.
 * 
 * @param parts			partition list to remove from
 * @param idx			index of the partition to remove
 * 
 * @return				EOK on success, EINVAL if idx invalid
 */
int mbr_remove_partition(mbr_partitions_t * parts, size_t idx)
{
	link_t * l = list_nth(&(parts->list), idx);
	if (l == NULL)
		return EINVAL;
	
	mbr_part_t * p;
	
	/* TODO: if it is extended partition, should we also remove all logical?
	 * If we don't, we break the consistency of the list. If we do,
	 * the user will have to input them all over again. So yes. */
	if (l == parts->l_extended) {
		parts->l_extended = NULL;
		
		link_t * it = l->next;
		link_t * next_it;
		while (it != &(parts->list.head)) {
			next_it = it->next;
			
			p = list_get_instance(it, mbr_part_t, link);
			if (mbr_get_flag(p, ST_LOGIC)) {
				list_remove(it);
				parts->n_logical -= 1;
				mbr_free_partition(p);
			}
			
			it = next_it;
		}
		
	}
	
	list_remove(l);
	
	p = list_get_instance(l, mbr_part_t, link);
	if (mbr_get_flag(p, ST_LOGIC))
		parts->n_logical -= 1;
	else
		parts->n_primary -= 1;
	
	
	mbr_free_partition(p);
	
	return EOK;
}

/** mbr_part_t destructor */
void mbr_free_partition(mbr_part_t * p)
{
	if (p->ebr != NULL)
		free(p->ebr);
	free(p);
}

/** Get flag bool value */
int mbr_get_flag(mbr_part_t * p, MBR_FLAGS flag)
{
	return (p->status & (1 << flag)) ? 1 : 0;
}

/** Set a specifig status flag to a value */
void mbr_set_flag(mbr_part_t * p, MBR_FLAGS flag, bool value)
{
	uint8_t status = p->status;

	if (value)
		status = status | (1 << flag);
	else
		status = status ^ (status & (1 << flag));

	p->status = status;
}

/** Get next aligned address (in sectors!) */
uint32_t mbr_get_next_aligned(uint32_t addr, unsigned int alignment)
{
	uint32_t div = addr / alignment;
	return (div + 1) * alignment;
}

/** Just a wrapper for free() */
void mbr_free_mbr(mbr_t * mbr)
{
	free(mbr);
}

/** Free partition list
 *
 * @param parts		partition list to be freed
 */
void mbr_free_partitions(mbr_partitions_t * parts)
{
	list_foreach_safe(parts->list, cur_link, next) {
		mbr_part_t * p = list_get_instance(cur_link, mbr_part_t, link);
		list_remove(cur_link);
		mbr_free_partition(p);
	}

	free(parts);
}

// Internal functions follow //

static br_block_t * alloc_br()
{
	br_block_t * br = malloc(sizeof(br_block_t));
	if (br == NULL)
		return NULL;
	
	memset(br, 0, 512);
	br->signature = host2uint16_t_le(BR_SIGNATURE);
	
	return br;
}

/** Parse partition entry to mbr_part_t
 * @return		returns 1, if extended partition, 0 otherwise
 * */
static int decode_part(pt_entry_t * src, mbr_part_t * trgt, uint32_t base)
{
	trgt->type = src->ptype;

	/* Checking only 0x80; otherwise writing will fix to 0x00 */
	//trgt->bootable = (src->status == B_ACTIVE) ? true : false;
	mbr_set_flag(trgt, ST_BOOT, (src->status == B_ACTIVE) ? true : false);

	trgt->start_addr = uint32_t_le2host(src->first_lba) + base;
	trgt->length = uint32_t_le2host(src->length);

	return (src->ptype == PT_EXTENDED) ? 1 : 0;
}

/** Parse MBR contents to mbr_part_t list */
static int decode_logical(mbr_t * mbr, mbr_partitions_t * parts, mbr_part_t * ext)
{
	int rc;
	mbr_part_t * p;

	if (mbr == NULL || parts == NULL)
		return EINVAL;


	if (ext == NULL)
		return EOK;


	uint32_t base = ext->start_addr;
	uint32_t addr = base;
	br_block_t * ebr;
	
	rc = block_init(EXCHANGE_ATOMIC, mbr->device, 512);
	if (rc != EOK)
		return rc;
	
	ebr = alloc_br();
	if (ebr == NULL) {
		rc = ENOMEM;
		goto end;
	}
	
	rc = block_read_direct(mbr->device, addr, 1, ebr);
	if (rc != EOK) {
		goto free_ebr_end;
	}
	
	if (uint16_t_le2host(ebr->signature) != BR_SIGNATURE) {
		rc = EINVAL;
		goto free_ebr_end;
	}
	
	if (ebr->pte[0].ptype == PT_UNUSED) {
		rc = EOK;
		goto free_ebr_end;
	}
	
	p = mbr_alloc_partition();
	if (p == NULL) {
		rc = ENOMEM;
		goto free_ebr_end;
	}
	
	decode_part(&(ebr->pte[0]), p, base);
	mbr_set_flag(p, ST_LOGIC, true);
	p->ebr = ebr;
	p->ebr_addr = addr;
	rc = mbr_add_partition(parts, p);
	if (rc != ERR_OK) 
		return EINVAL;
	
	addr = uint32_t_le2host(ebr->pte[1].first_lba) + base;
	
	while (ebr->pte[1].ptype != PT_UNUSED) {
		ebr = alloc_br();
		if (ebr == NULL) {
			rc = ENOMEM;
			goto end;
		}
		
		rc = block_read_direct(mbr->device, addr, 1, ebr);
		if (rc != EOK) {
			goto free_ebr_end;
		}
		
		if (uint16_t_le2host(ebr->signature) != BR_SIGNATURE) {
			rc = EINVAL;
			goto free_ebr_end;
		}
		
		p = mbr_alloc_partition();
		if (p == NULL) {
			rc = ENOMEM;
			goto free_ebr_end;
		}
		
		
		decode_part(&(ebr->pte[0]), p, addr);
		mbr_set_flag(p, ST_LOGIC, true);
		p->ebr = ebr;
		p->ebr_addr = addr;
		rc = mbr_add_partition(parts, p);
		if (rc != ERR_OK)
			return EINVAL;
		
		addr = uint32_t_le2host(ebr->pte[1].first_lba) + base;
	}
	
	rc = EOK;
	goto end;
	
free_ebr_end:
	free(ebr);
	
end:
	block_fini(mbr->device);
	
	return rc;
}

/** Convert mbr_part_t to pt_entry_t */
static void encode_part(mbr_part_t * src, pt_entry_t * trgt, uint32_t base, bool ebr)
{
	if (src != NULL) {
		trgt->status = mbr_get_flag(src, ST_BOOT) ? B_ACTIVE : B_INACTIVE;
		if (ebr) {	// encoding reference to EBR
			trgt->ptype = PT_EXTENDED_LBA;
			trgt->first_lba = host2uint32_t_le(src->ebr_addr - base);
			trgt->length = host2uint32_t_le(src->length + src->start_addr - src->ebr_addr);
		} else {	// encoding reference to partition
			trgt->ptype = src->type;
			trgt->first_lba = host2uint32_t_le(src->start_addr - base);
			trgt->length = host2uint32_t_le(src->length);
		}
	} else {
		trgt->status = 0;
		trgt->first_chs[0] = 0;
		trgt->first_chs[1] = 0;
		trgt->first_chs[2] = 0;
		trgt->ptype = 0;
		trgt->last_chs[0] = 0;
		trgt->last_chs[1] = 0;
		trgt->last_chs[2] = 0;
		trgt->first_lba = 0;
		trgt->length = 0;
	}
}

/** Check whether two partitions overlap
 * 
 * @return		1 for yes, 0 for no
 */
static int check_overlap(mbr_part_t * p1, mbr_part_t * p2)
{
	if (p1->start_addr < p2->start_addr && p1->start_addr + p1->length < p2->start_addr) {
		return 0;
	} else if (p1->start_addr > p2->start_addr && p2->start_addr + p2->length < p1->start_addr) {
		return 0;
	}

	return 1;
}

/** Check whether one partition encapsulates the other
 * 
 * @return		1 for yes, 0 for no
 */
static int check_encaps(mbr_part_t * inner, mbr_part_t * outer)
{
	if (inner->start_addr <= outer->start_addr || outer->start_addr + outer->length <= inner->start_addr) {
		return 0;
	} else if (outer->start_addr + outer->length < inner->start_addr + inner->length) {
		return 0;
	}

	return 1;
}

/** Check whether one partition preceeds the other
 * 
 * @return		1 for yes, 0 for no
 */
static int check_preceeds(mbr_part_t * preceeder, mbr_part_t * precedee)
{
	return preceeder->start_addr < precedee->start_addr;
}




