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
#include <str_error.h>

#include "libmbr.h"

static br_block_t * alloc_br(void);
static int decode_part(pt_entry_t *, mbr_part_t *, uint32_t);
static int decode_logical(mbr_label_t *, mbr_part_t *);
static void encode_part(mbr_part_t *, pt_entry_t *, uint32_t, bool);
static bool check_overlap(mbr_part_t *, mbr_part_t *);
static bool check_encaps(mbr_part_t *, mbr_part_t *);
static bool check_preceeds(mbr_part_t *, mbr_part_t *);
static mbr_err_val mbr_add_primary(mbr_label_t *, mbr_part_t *);
static mbr_err_val mbr_add_logical(mbr_label_t *, mbr_part_t *);

/** Allocate and initialize mbr_label_t structure */
mbr_label_t * mbr_alloc_label(void)
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
mbr_t * mbr_alloc_mbr(void)
{
	return malloc(sizeof(mbr_t));
}

/** Read MBR from specific device
 * @param   label       label to write data to
 * @param   dev_handle  device to read MBR from
 *
 * @return				EOK on success, error code on error
 */
int mbr_read_mbr(mbr_label_t *label, service_id_t dev_handle)
{	
	int rc;
	
	if (label->mbr == NULL) {
		label->mbr = mbr_alloc_mbr();
		if (label->mbr == NULL) {
			return ENOMEM;
		}
	}

	rc = block_init(EXCHANGE_ATOMIC, dev_handle, 512);
	if (rc != EOK)
		return rc;

	rc = block_read_direct(dev_handle, 0, 1, &(label->mbr->raw_data));
	block_fini(dev_handle);
	if (rc != EOK)
		return rc;

	label->device = dev_handle;

	return EOK;
}

/** Write mbr to disk
 * @param label			MBR to be written
 * @param dev_handle	device handle to write MBR to (may be different
 * 							from the device in 'mbr')
 *
 * @return				0 on success, otherwise libblock error code
 */
int mbr_write_mbr(mbr_label_t *label, service_id_t dev_handle)
{
	int rc;

	rc = block_init(EXCHANGE_ATOMIC, dev_handle, 512);
	if (rc != EOK) {
		return rc;
	}

	rc = block_write_direct(dev_handle, 0, 1, &(label->mbr->raw_data));
	block_fini(dev_handle);
	if (rc != EOK) {
		return rc;
	}

	return EOK;
}

/** Decide whether this is an actual MBR or a Protective MBR from GPT
 *
 * @param mbr		the actual MBR to decide upon
 *
 * @return			1 if MBR, 0 if GPT
 */
int mbr_is_mbr(mbr_label_t *label)
{
	return (label->mbr->raw_data.pte[0].ptype != PT_GPT) ? 1 : 0;
}

/** Parse partitions from MBR, freeing previous partitions if any
 * NOTE: it is assumed mbr_read_mbr(label) was called before.
 * @param label  MBR to be parsed
 *
 * @return       linked list of partitions or NULL on error
 */
int mbr_read_partitions(mbr_label_t *label)
{
	if (label == NULL || label->mbr == NULL)
		return EINVAL;
	
	int rc, rc_ext;
	unsigned int i;
	mbr_part_t *p;
	mbr_part_t *ext = NULL;
	
	if (label->parts != NULL)
		mbr_free_partitions(label->parts);
	
	label->parts = mbr_alloc_partitions();
	if (label->parts == NULL) {
		return ENOMEM;
	}
	
	/* Generate the primary partitions */
	for (i = 0; i < N_PRIMARY; ++i) {
		if (label->mbr->raw_data.pte[i].ptype == PT_UNUSED)
			continue;
		
		p = mbr_alloc_partition();
		if (p == NULL) {
			printf(LIBMBR_NAME ": Error on memory allocation.\n");
			mbr_free_partitions(label->parts);
			return ENOMEM;
		}
		
		rc_ext = decode_part(&(label->mbr->raw_data.pte[i]), p, 0);
		mbr_set_flag(p, ST_LOGIC, false);
		rc = mbr_add_partition(label, p);
		if (rc != ERR_OK) {
			printf(LIBMBR_NAME ": Error occured during decoding the MBR. (%d)\n" \
			       LIBMBR_NAME ": MBR is invalid.\n", rc);
			mbr_free_partitions(label->parts);
			return EINVAL;
		}
		
		if (rc_ext) {
			ext = p;
			label->parts->l_extended = &p->link;
		}
	}
	
	/* Fill in the primary partitions and generate logical ones, if any */
	rc = decode_logical(label, ext);
	if (rc != EOK) {
		printf(LIBMBR_NAME ": Error during decoding logical partitions: %d - %s.\n" \
			   LIBMBR_NAME ": Partition list may be incomplete.\n", rc, str_error(rc));
		return rc;
	}
	
	return EOK;
}

/** Write MBR and partitions to device
 * @param label        label to write
 * @param dev_handle   device to write the data to
 *
 * @return             returns EOK on succes, specific error code otherwise
 */
int mbr_write_partitions(mbr_label_t *label, service_id_t dev_handle)
{
	if (label->parts == NULL)
		return EOK;
	
	if (label->mbr == NULL)
		label->mbr = mbr_alloc_mbr();
	
	int i = 0;
	int rc;
	mbr_part_t *p;
	mbr_part_t *ext = (label->parts->l_extended == NULL) ? NULL
					: list_get_instance(label->parts->l_extended, mbr_part_t, link);
	
	rc = block_init(EXCHANGE_ATOMIC, dev_handle, 512);
	if (rc != EOK) {
		printf(LIBMBR_NAME ": Error while initializing libblock: %d - %s.\n", rc, str_error(rc));
		return rc;
	}
	
	link_t *l = label->parts->list.head.next;
	
	/* Encoding primary partitions */
	for (i = 0; i < N_PRIMARY; i++) {
		p = list_get_instance(l, mbr_part_t, link);	
		encode_part(p, &(label->mbr->raw_data.pte[i]), 0, false);
		l = l->next;
	}
	
	label->mbr->raw_data.signature = host2uint16_t_le(BR_SIGNATURE);
	
	/* Writing MBR */
	rc = block_write_direct(dev_handle, 0, 1, &(label->mbr->raw_data));
	if (rc != EOK) {
		printf(LIBMBR_NAME ": Error while writing MBR : %d - %s.\n", rc, str_error(rc));
		goto end;
	}
	
	if (ext == NULL) {
		rc = EOK;
		goto end;
	}
	
	uint32_t base = ext->start_addr;
	mbr_part_t *prev_p;
	
	/* Note for future changes: Some thought has been put into design
	 * and implementation. If you don't have to change it, don't. Other
	 * designs have been tried, this came out as the least horror with
	 * as much power over it as you can get. */
	
	/* Encoding and writing first logical partition */
	if (l != &(label->parts->list.head)) {
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
		rc = EOK;
		goto end;
	}
	
	prev_p = p;
	
	/* Check EBR addresses
	 * This piece of code saves previous EBR placements from other
	 * software. But if our user modifies the logical partition chain,
	 * we have to fix those placements if needed.*/
	link_t *l_ebr = l;
	link_t *l_iter;
	mbr_part_t *tmp = mbr_alloc_partition();
	tmp->length = 1;
	while (l_ebr != &(label->parts->list.head)) {
		p = list_get_instance(l_ebr, mbr_part_t, link);
		tmp->start_addr = p->ebr_addr;
		
		l_iter = l;
		while (l_iter != &(label->parts->list.head)) {
			/* Checking whether EBR address makes sense. If not, we take a guess.
			 * So far this is simple, we just take the first preceeding sector.
			 * Fdisk always reserves at least 2048 sectors (1MiB), so it can have 
			 * the EBR aligned as well as the partition itself. Parted reserves
			 * minimum one sector, like we do.
			 * 
			 * Note that we know there is at least one sector free from previous checks.
			 * Also note that the user can set ebr_addr to their liking (if it's valid). */		 
			if (p->ebr_addr < base || p->ebr_addr >= base + ext->length ||
			  check_overlap(tmp, list_get_instance(l_iter, mbr_part_t, link))) {
				p->ebr_addr = p->start_addr - 1;
				break;
			}
			
			l_iter = l_iter->next;
		}
		
		l_ebr = l_ebr->next;
	}
	mbr_free_partition(tmp);
	
	/* Encoding and writing logical partitions */
	while (l != &(label->parts->list.head)) {
		p = list_get_instance(l, mbr_part_t, link);
		
		
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
	
	/* write the last EBR */
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
	mbr_part_t *p = malloc(sizeof(mbr_part_t));
	if (p == NULL) {
		return NULL;
	}
	
	link_initialize(&(p->link));
	p->ebr = NULL;
	p->type = PT_UNUSED;
	p->status = 0;
	p->start_addr = 0;
	p->length = 0;
	p->ebr_addr = 0;
	
	return p;
}

/** mbr_partitions_t constructor */
mbr_partitions_t * mbr_alloc_partitions(void)
{
	mbr_partitions_t *parts = malloc(sizeof(mbr_partitions_t));
	if (parts == NULL) {
		return NULL;
	}
	
	list_initialize(&(parts->list));
	parts->n_primary = 0;
	parts->n_logical = 0;
	parts->l_extended = NULL;
	
	/* add blank primary partitions */
	int i;
	mbr_part_t *p;
	for (i = 0; i < N_PRIMARY; ++i) {
		p = mbr_alloc_partition();
		if (p == NULL) {
			mbr_free_partitions(parts);
			return NULL;
		}
		list_append(&(p->link), &(parts->list));
	}
	

	return parts;
}

/** Add partition
 * 	Performs checks, sorts the list.
 * 
 * @param label			label to add to
 * @param p				partition to add
 * 
 * @return				ERR_OK (0) on success, other MBR_ERR_VAL otherwise
 */
mbr_err_val mbr_add_partition(mbr_label_t *label, mbr_part_t *p)
{
	int rc1, rc2;
	aoff64_t nblocks;
	
	rc1 = block_init(EXCHANGE_ATOMIC, label->device, 512);
	if (rc1 != EOK && rc1 != EEXIST) {
		printf(LIBMBR_NAME ": Error during libblock init: %d - %s.\n", rc1, str_error(rc1));
		return ERR_LIBBLOCK;
	}
	
	rc2 = block_get_nblocks(label->device, &nblocks);
	
	if (rc1 != EEXIST)
		block_fini(label->device);
	
	if (rc2 != EOK) {
		printf(LIBMBR_NAME ": Error while getting number of blocks: %d - %s.\n", rc2, str_error(rc2));
		return ERR_LIBBLOCK;
	}
	
	if ((aoff64_t) p->start_addr + p->length > nblocks)
		return ERR_OUT_BOUNDS;
	
	if (label->parts == NULL) {
		label->parts = mbr_alloc_partitions();
		if (label->parts == NULL)
			return ENOMEM; //FIXME! merge mbr_err_val into errno.h
	}
	
	if (mbr_get_flag(p, ST_LOGIC))
		/* adding logical partition */
		return mbr_add_logical(label, p);
	else
		/* adding primary */
		return mbr_add_primary(label, p);
}

/** Remove partition
 * 	Removes partition by index, indexed from zero. When removing extended
 *  partition, all logical partitions get removed as well.
 * 
 * @param label			label to remove from
 * @param idx			index of the partition to remove
 * 
 * @return				EOK on success, EINVAL if idx invalid
 */
int mbr_remove_partition(mbr_label_t *label, size_t idx)
{
	link_t *l = list_nth(&(label->parts->list), idx);
	if (l == NULL)
		return EINVAL;
	
	mbr_part_t *p;
	
	/* If we're removing an extended partition, remove all logical as well */
	if (l == label->parts->l_extended) {
		label->parts->l_extended = NULL;
		
		link_t *it = l->next;
		link_t *next_it;
		while (it != &(label->parts->list.head)) {
			next_it = it->next;
			
			p = list_get_instance(it, mbr_part_t, link);
			if (mbr_get_flag(p, ST_LOGIC)) {
				list_remove(it);
				label->parts->n_logical -= 1;
				mbr_free_partition(p);
			}
			
			it = next_it;
		}
		
	}
	
	/* Remove the partition itself */
	p = list_get_instance(l, mbr_part_t, link);
	if (mbr_get_flag(p, ST_LOGIC)) {
		label->parts->n_logical -= 1;
		list_remove(l);
		mbr_free_partition(p);
	} else {
		/* Cannot remove primary - it would break ordering, just zero it */
		label->parts->n_primary -= 1;
		p->type = 0;
		p->status = 0;
		p->start_addr = 0;
		p->length = 0;
		p->ebr_addr = 0;
	}
	
	return EOK;
}

/** mbr_part_t destructor */
void mbr_free_partition(mbr_part_t *p)
{
	if (p->ebr != NULL)
		free(p->ebr);
	free(p);
}

/** Get flag bool value */
int mbr_get_flag(mbr_part_t *p, MBR_FLAGS flag)
{
	return (p->status & (1 << flag)) ? 1 : 0;
}

/** Set a specifig status flag to a value */
void mbr_set_flag(mbr_part_t *p, MBR_FLAGS flag, bool value)
{
	uint16_t status = p->status;

	if (value)
		status = status | (1 << flag);
	else
		status = status ^ (status & (1 << flag));

	p->status = status;
}

/** Get next aligned address */
uint32_t mbr_get_next_aligned(uint32_t addr, unsigned int alignment)
{
	uint32_t div = addr / alignment;
	return (div + 1) * alignment;
}

list_t * mbr_get_list(mbr_label_t *label)
{
	if (label->parts != NULL)
		return &(label->parts->list);
	else
		return NULL;
}

mbr_part_t * mbr_get_first_partition(mbr_label_t *label)
{
	list_t *list = mbr_get_list(label);
	if (list != NULL && !list_empty(list))
		return list_get_instance(list->head.next, mbr_part_t, link);
	else
		return NULL;
}

mbr_part_t * mbr_get_next_partition(mbr_label_t *label, mbr_part_t *p)
{
	list_t *list = mbr_get_list(label);
	if (list != NULL && &(p->link) != list_last(list))
		return list_get_instance(p->link.next, mbr_part_t, link);
	else
		return NULL;
}

/** Just a wrapper for free() */
void mbr_free_mbr(mbr_t *mbr)
{
	free(mbr);
}

/** Free partition list
 *
 * @param parts		partition list to be freed
 */
void mbr_free_partitions(mbr_partitions_t *parts)
{
	list_foreach_safe(parts->list, cur_link, next) {
		mbr_part_t *p = list_get_instance(cur_link, mbr_part_t, link);
		list_remove(cur_link);
		mbr_free_partition(p);
	}

	free(parts);
}

/* Internal functions follow */

static br_block_t *alloc_br()
{
	br_block_t *br = malloc(sizeof(br_block_t));
	if (br == NULL)
		return NULL;
	
	memset(br, 0, 512);
	br->signature = host2uint16_t_le(BR_SIGNATURE);
	
	return br;
}

/** Parse partition entry to mbr_part_t
 * @return		returns 1, if extended partition, 0 otherwise
 * */
static int decode_part(pt_entry_t *src, mbr_part_t *trgt, uint32_t base)
{
	trgt->type = src->ptype;

	trgt->status = (trgt->status & 0xFF00) | (uint16_t) src->status;

	trgt->start_addr = uint32_t_le2host(src->first_lba) + base;
	trgt->length = uint32_t_le2host(src->length);

	return (src->ptype == PT_EXTENDED) ? 1 : 0;
}

/** Parse MBR contents to mbr_part_t list */
static int decode_logical(mbr_label_t *label, mbr_part_t * ext)
{
	int rc;
	mbr_part_t *p;

	if (ext == NULL)
		return EOK;

	uint32_t base = ext->start_addr;
	uint32_t addr = base;
	br_block_t *ebr;
	
	rc = block_init(EXCHANGE_ATOMIC, label->device, 512);
	if (rc != EOK)
		return rc;
	
	ebr = alloc_br();
	if (ebr == NULL) {
		rc = ENOMEM;
		goto end;
	}
	
	rc = block_read_direct(label->device, addr, 1, ebr);
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
	rc = mbr_add_partition(label, p);
	if (rc != ERR_OK) 
		return EINVAL;
	
	addr = uint32_t_le2host(ebr->pte[1].first_lba) + base;
	
	while (ebr->pte[1].ptype != PT_UNUSED) {
		
		ebr = alloc_br();
		if (ebr == NULL) {
			rc = ENOMEM;
			goto end;
		}
		
		rc = block_read_direct(label->device, addr, 1, ebr);
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
		rc = mbr_add_partition(label, p);
		if (rc != ERR_OK)
			return EINVAL;
		
		addr = uint32_t_le2host(ebr->pte[1].first_lba) + base;
	}
	
	rc = EOK;
	goto end;
	
free_ebr_end:
	free(ebr);
	
end:
	block_fini(label->device);
	
	return rc;
}

/** Convert mbr_part_t to pt_entry_t */
static void encode_part(mbr_part_t * src, pt_entry_t * trgt, uint32_t base, bool ebr)
{
	if (src != NULL) {
		trgt->status = (uint8_t) (src->status & 0xFF);
		/* ingoring CHS */
		trgt->first_chs[0] = 0xFE;
		trgt->first_chs[1] = 0xFF;
		trgt->first_chs[2] = 0xFF;
		trgt->last_chs[0] = 0xFE;
		trgt->last_chs[1] = 0xFF;
		trgt->last_chs[2] = 0xFF;
		if (ebr) {	/* encoding reference to EBR */
			trgt->ptype = PT_EXTENDED_LBA;
			trgt->first_lba = host2uint32_t_le(src->ebr_addr - base);
			trgt->length = host2uint32_t_le(src->length + src->start_addr - src->ebr_addr);
		} else {	/* encoding reference to partition */
			trgt->ptype = src->type;
			trgt->first_lba = host2uint32_t_le(src->start_addr - base);
			trgt->length = host2uint32_t_le(src->length);
		}
		
		if (trgt->ptype == PT_UNUSED)
			memset(trgt, 0, sizeof(pt_entry_t));
	} else {
		memset(trgt, 0, sizeof(pt_entry_t));
	}
}

/** Check whether two partitions overlap
 * 
 * @return		true/false
 */
static bool check_overlap(mbr_part_t * p1, mbr_part_t * p2)
{
	if (p1->start_addr < p2->start_addr && p1->start_addr + p1->length <= p2->start_addr) {
		return false;
	} else if (p1->start_addr > p2->start_addr && p2->start_addr + p2->length <= p1->start_addr) {
		return false;
	}

	return true;
}

/** Check whether one partition encapsulates the other
 * 
 * @return		true/false
 */
static bool check_encaps(mbr_part_t * inner, mbr_part_t * outer)
{
	if (inner->start_addr <= outer->start_addr || outer->start_addr + outer->length <= inner->start_addr) {
		return false;
	} else if (outer->start_addr + outer->length < inner->start_addr + inner->length) {
		return false;
	}

	return true;
}

/** Check whether one partition preceeds the other
 * 
 * @return		true/false
 */
static bool check_preceeds(mbr_part_t * preceeder, mbr_part_t * precedee)
{
	return preceeder->start_addr < precedee->start_addr;
}

mbr_err_val mbr_add_primary(mbr_label_t *label, mbr_part_t *p)
{
	if (label->parts->n_primary == 4) {
		return ERR_PRIMARY_FULL;
	}
	
	/* Check if partition makes space for MBR itself. */
	if (p->start_addr == 0) {
		return ERR_OUT_BOUNDS;
	}
	
	/* if it's extended, is there any other one? */
	if ((p->type == PT_EXTENDED || p->type == PT_EXTENDED_LBA) && label->parts->l_extended != NULL) {
		return ERR_EXTENDED_PRESENT;
	}
	
	/* find a place and add it */
	mbr_part_t *iter;
	mbr_part_t *empty = NULL;
	mbr_part_foreach(label, iter) {
		if (iter->type == PT_UNUSED) {
			if (empty == NULL)
				empty = iter;
		} else if (check_overlap(p, iter))
			return ERR_OVERLAP;
	}
	
	list_insert_after(&(p->link), &(empty->link));
	list_remove(&(empty->link));
	free(empty);
	
	label->parts->n_primary += 1;
	
	if (p->type == PT_EXTENDED || p->type == PT_EXTENDED_LBA)
		label->parts->l_extended = &(p->link);
	
	return EOK;
}

mbr_err_val mbr_add_logical(mbr_label_t *label, mbr_part_t *p)
{
	/* is there any extended partition? */
	if (label->parts->l_extended == NULL)
		return ERR_NO_EXTENDED;
	
	/* is the logical partition inside the extended one? */
	mbr_part_t *ext = list_get_instance(label->parts->l_extended, mbr_part_t, link);
	if (!check_encaps(p, ext))
		return ERR_OUT_BOUNDS;
	
	/* find a place for the new partition in a sorted linked list */
	bool first_logical = true;
	mbr_part_t *iter;
	mbr_part_foreach (label, iter) {
		if (mbr_get_flag(iter, ST_LOGIC)) {
			if (check_overlap(p, iter)) 
				return ERR_OVERLAP;
			if (check_preceeds(iter, p)) {
				/* checking if there's at least one sector of space preceeding */
				if ((iter->start_addr + iter->length) >= p->start_addr - 1)
					return ERR_NO_EBR;
			} else if (first_logical){
				/* First logical partition's EBR is before every other
				 * logical partition. Thus we don't check if this partition
				 * leaves enough space for it. */
				first_logical = false;
			} else {
				/* checking if there's at least one sector of space following (for following partitions's EBR) */
				if ((p->start_addr + p->length) >= iter->start_addr - 1)
					return ERR_NO_EBR;
			}
		}
	}
	
	/* alloc EBR if it's not already there */
	if (p->ebr == NULL) {
		p->ebr = alloc_br();
		if (p->ebr == NULL) {
			return ERR_NOMEM;
		}
	}
	
	/* add it */
	list_append(&(p->link), &(label->parts->list));
	label->parts->n_logical += 1;
	
	return EOK;
}



