/*
 * Copyright (c) 2011 Dominik Taborsky
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

 /** @addtogroup libmbr
 * @{
 */
/** @file MBR extraxtion library
 */

#include <ipc/bd.h>
#include <async.h>
#include <stdio.h>
#include <block.h>
#include <errno.h>
#include <stdlib.h>
#include <assert.h>
#include <byteorder.h>

#include "libmbr.h"

static br_block_t * alloc_br(void);
static int decode_part(pt_entry_t * src, part_t * trgt, uint32_t base);
static int decode_logical(mbr_t * mbr, mbr_parts_t * p, part_t * ext);
static void encode_part(part_t * src, pt_entry_t * trgt, uint32_t base);

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
		return NULL;
	}

	rc = block_read_direct(dev_handle, 0, 1, &(mbr->raw_data));
	if (rc != EOK) {
		block_fini(dev_handle);
		return NULL;
	}

	block_fini(dev_handle);

	mbr->device = dev_handle;
	//mbr->partitions = NULL;

	return mbr;
}

/** Write mbr to disk
 * @param mbr			MBR to be written
 * @param dev_handle	device handle to write MBR to (may be different
 * 							from the device in 'mbr')
 *
 * @return				0 on success, -1 on block_init error, -2 on write error
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
mbr_parts_t * mbr_read_partitions(mbr_t * mbr)
{
	int rc, i;
	part_t * p;
	part_t * ext = NULL;
	mbr_parts_t * parts;

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
		
		p = malloc(sizeof(part_t));
		if (p == NULL) {
			printf(NAME ": Error on memory allocation.\n");
			free(p);
			mbr_free_partitions(parts);
			return NULL;
		}
		list_append(&(p->link), &(parts->list));
		p->ebr = NULL;
		if (decode_part(&(mbr->raw_data.pte[i]), p, 0))
			ext = p;
	}

	// Fill in the primary partitions and generate logical ones, if any
	rc = decode_logical(mbr, parts, ext);
	if (rc != EOK) {
		printf(NAME ": Error occured during decoding the MBR.\n" \
			   NAME ": Partition list may be incomplete.\n");
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
int mbr_write_partitions(mbr_parts_t * parts, mbr_t * mbr, service_id_t dev_handle)
{
	bool logical = false;
	int i = 0;
	int rc;
	part_t * p;
	part_t * ext = (parts->l_extended == NULL) ? NULL
					: list_get_instance(parts->l_extended, part_t, link);
	
	br_block_t * last_ebr = NULL;


	rc = block_init(EXCHANGE_ATOMIC, dev_handle, 512);
	if (rc != EOK) {
		return rc;
	}

	if (ext == NULL)
		goto no_extended;

	aoff64_t addr = ext->start_addr;
	part_t * prev_part = NULL;

	list_foreach(parts->list, it) {
		p = list_get_instance(it, part_t, link);
		if (mbr_get_flag(p, ST_LOGIC)) {
			// writing logical partition

			if (p->ebr == NULL) {
				p->ebr = alloc_br();
				if (p->ebr == NULL)
				{
					rc = ENOMEM;
					goto end;
				}
			}


			encode_part(p, &(p->ebr->pte[0]), addr);
			if (prev_part != NULL) {
				encode_part(p, &(prev_part->ebr->pte[1]), ext->start_addr);
				rc = block_write_direct(dev_handle, p->start_addr, 1, prev_part->ebr);
				if (rc != EOK)
					goto end;
			}
			
			addr = p->start_addr;
			prev_part = p;
		} else {
			// writing primary partition
			if (i >= 4) {
				rc = EINVAL;
				goto end;
			}

			encode_part(p, &(mbr->raw_data.pte[i]), 0);

			++i;
		}
	}

	/* If there was an extended but no logical, we should overwrite
	 * the space where the first logical's EBR would have been. There
	 * might be some garbage from the past.
	 */
	
	last_ebr = prev_part->ebr;
	
	if (!logical)
	{
		last_ebr = alloc_br();
		if (last_ebr == NULL) {
			rc = ENOMEM;
			goto end;
		}
		
		last_ebr->pte[0].ptype = PT_UNUSED;
	}
	
	
	encode_part(NULL, &(last_ebr->pte[1]), 0);
	rc = block_write_direct(dev_handle, addr, 1, last_ebr);
	
	if (!logical)
	{
		free(last_ebr);
	}
	
	if (rc != EOK)
		goto end;

	goto skip;

no_extended:

	list_foreach(parts->list, it) {
		p = list_get_instance(it, part_t, link);
		if (mbr_get_flag(p, ST_LOGIC)) {
			// extended does not exist, fail
			return EINVAL;
		} else {
			// writing primary partition
			if (i >= 4)
				return EINVAL;

			encode_part(p, &(mbr->raw_data.pte[i]), 0);

			++i;
		}
	}

skip:
	rc = block_write_direct(dev_handle, 0, 1, &(mbr->raw_data));
	if (rc != EOK)
		goto end;

	/*
	for (i = 0; i < N_PRIMARY; ++i) {
		encode_part(&(p->partition), &(mbr->raw_data.pte[i]), 0);
		if (p->type == PT_EXTENDED)
			ext = p;

		//p = list_get_instance(p->link.next, mbr_parts_t, link);
		p = p->next;
	}

	rc = block_write_direct(dev_handle, 0, 1, &(mbr->raw_data));
	if (rc != EOK) {
		block_fini(dev_handle);
		return rc;
	}

	//writing logical partitions

	if (p == NULL && ext != NULL) {
		//we need an empty EBR to rewrite the old EBR on disk, if we need to delete it
		br_block_t * temp_ebr = alloc_br();
		if (temp_ebr == NULL) {
			block_fini(dev_handle);
			return ENOMEM;
		}

		temp_ebr->pte[0].ptype = PT_UNUSED;
		encode_part(NULL, &(temp_ebr->pte[1]), 0);
		rc = block_write_direct(dev_handle, ext->start_addr, 1, temp_ebr);
		free(temp_ebr);
		block_fini(dev_handle);
		return rc;
	}

	if (p != NULL && ext == NULL) {
		block_fini(dev_handle);
		//no extended but one or more logical? EINVAL to the rescue!
		return EINVAL;
	}

	aoff64_t addr = ext->start_addr;

	while (p != NULL) {
		if (p->type == PT_UNUSED) {
			p = p->next;
			continue;
		}
		//encode_part(p, &(p->ebr->pte[0]), p->start_addr - 63 * 512);
		encode_part(p, &(p->ebr->pte[0]), addr);
		encode_part(p->next, &(p->ebr->pte[1]), ext->start_addr);

		rc = block_write_direct(dev_handle, p->start_addr, 1, p->ebr);
		if (rc != EOK) {
			block_fini(dev_handle);
			return rc;
		}
		addr = p->start_addr;
		p = p->next;
	}*/

	rc = EOK;
	
end:
	block_fini(dev_handle);

	return rc;
}

/** part_t constructor */
part_t * mbr_alloc_partition(void)
{
	part_t * p = malloc(sizeof(part_t));
	if (p == NULL) {
		return NULL;
	}
	link_initialize(&(p->link));
	p->ebr = NULL;
	p->type = 0;
	p->status = 0;
	p->start_addr = 0;
	p->length = 0;

	return p;
}

mbr_parts_t * mbr_alloc_partitions(void)
{
	mbr_parts_t * parts = malloc(sizeof(mbr_parts_t));
	if (parts == NULL) {
		return NULL;
	}

	list_initialize(&(parts->list));

	return parts;
}

/** Add partition */
void mbr_add_partition(mbr_parts_t * parts, part_t * partition)
{
	list_append(&(partition->link), &(parts->list));
}

/** Remove partition */
void mbr_remove_partition(mbr_parts_t * parts, int idx)
{
	link_t * l = list_nth(&(parts->list), idx);
	list_remove(l);
	part_t * p = list_get_instance(l, part_t, link);
	mbr_free_partition(p);
}

/** part_t destructor */
void mbr_free_partition(part_t * p)
{
	if (p->ebr != NULL)
		free(p->ebr);
	free(p);
}

/** Get flag bool value */
int mbr_get_flag(part_t * p, MBR_FLAGS flag)
{
	return (p->status & (1 << flag)) ? 1 : 0;
}

/** Set a specifig status flag to a value */
void mbr_set_flag(part_t * p, MBR_FLAGS flag, bool value)
{
	uint8_t status = p->status;

	if (value)
		status = status | (1 << flag);
	else
		status = status ^ (status & (1 << flag));

	p->status = status;
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
void mbr_free_partitions(mbr_parts_t * parts)
{
	list_foreach_safe(parts->list, cur_link, next) {
		part_t * p = list_get_instance(cur_link, part_t, link);
		list_remove(cur_link);
		mbr_free_partition(p);
	}
}

// Internal functions follow //

static br_block_t * alloc_br()
{
	br_block_t * br = malloc(sizeof(br_block_t));
	if (br == NULL)
		return NULL;

	br->media_id = 0;
	br->pad0 = 0;
	br->signature = host2uint16_t_le(BR_SIGNATURE);

	return br;
}

/** Parse partition entry to part_t */
static int decode_part(pt_entry_t * src, part_t * trgt, uint32_t base)
{
	trgt->type = src->ptype;

	/* Checking only 0x80; otherwise writing will fix to 0x00 */
	//trgt->bootable = (src->status == B_ACTIVE) ? true : false;
	mbr_set_flag(trgt, ST_BOOT, (src->status == B_ACTIVE) ? true : false);

	trgt->start_addr = uint32_t_le2host(src->first_lba) + base;
	trgt->length = uint32_t_le2host(src->length);

	return (src->ptype == PT_EXTENDED) ? 1 : 0;
}

/** Parse MBR contents to part_t list
 * parameter 'p' is allocated for only used primary partitions
 */
static int decode_logical(mbr_t * mbr, mbr_parts_t * parts, part_t * ext)
{
	int rc;
	part_t * p;

	if (mbr == NULL || parts == NULL)
		return EINVAL;


	if (ext == NULL)
		return EOK;


	uint32_t addr = ext->start_addr;
	//uint32_t base = ext->start_addr;
	br_block_t * ebr;

	rc = block_init(EXCHANGE_ATOMIC, mbr->device, 512);
	if (rc != EOK)
		return rc;

	do {
		ebr = alloc_br();
		if (ebr == NULL) {
			return ENOMEM;
		}

		rc = block_read_direct(mbr->device, addr, 1, ebr);
		if (rc != EOK) {
			return rc;
		}

		//FIXME: is this the right way?
		if (uint16_t_le2host(ebr->signature) != BR_SIGNATURE) {
			return EINVAL;
		}

		p = mbr_alloc_partition();

		decode_part(&(ebr->pte[0]), p, addr);
		mbr_set_flag(p, ST_LOGIC, true);
		p->ebr = ebr;
		mbr_add_partition(parts, p);

		//TODO: Check this code
		addr = ebr->pte[1].first_lba + ext->start_addr;
	} while (ebr->pte[1].ptype != PT_UNUSED);


	block_fini(mbr->device);

	return EOK;
}

/** Convert part_t to pt_entry_t */
static void encode_part(part_t * src, pt_entry_t * trgt, uint32_t base)
{
	if (src != NULL) {
		trgt->status = mbr_get_flag(src, ST_BOOT) ? B_ACTIVE : B_INACTIVE;
		trgt->ptype = src->type;
		trgt->first_lba = host2uint32_t_le(src->start_addr - base + 63); //63 sectors skipped
		trgt->length = host2uint32_t_le(src->length - 64);	//63 + 1 (EBR)
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

