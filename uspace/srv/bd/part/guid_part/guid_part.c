/*
 * Copyright (c) 2010 Jiri Svoboda
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

/** @addtogroup bd
 * @{
 */

/**
 * @file
 * @brief GUID partition table driver
 *
 * Handles the GUID partitioning scheme. Uses a block device
 * and provides one for each partition.
 *
 * References:
 *	UEFI Specification Version 2.3, Chapter 5 GUID Partition Table (GPT)
 *	Format, http://www.uefi.org/specs/
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <ipc/bd.h>
#include <async.h>
#include <as.h>
#include <fibril_synch.h>
#include <loc.h>
#include <sys/types.h>
#include <sys/typefmt.h>
#include <inttypes.h>
#include <libblock.h>
#include <loc.h>
#include <errno.h>
#include <bool.h>
#include <byteorder.h>
#include <assert.h>
#include <macros.h>
#include <task.h>

#include "gpt.h"

#define NAME "guid_part"

const uint8_t efi_signature[8] = {
	/* "EFI PART" in ASCII */
	0x45, 0x46, 0x49, 0x20, 0x50, 0x41, 0x52, 0x54
};

/** Partition */
typedef struct part {
	/** Partition entry is in use */
	bool present;
	/** Address of first block */
	aoff64_t start_addr;
	/** Number of blocks */
	aoff64_t length;
	/** Service representing the partition (outbound device) */
	service_id_t dsid;
	/** Points to next partition structure. */
	struct part *next;
} part_t;

static size_t block_size;

/** Partitioned device (inbound device) */
static service_id_t indev_sid;

/** List of partitions. This structure is an empty head. */
static part_t plist_head;

static int gpt_init(const char *dev_name);
static int gpt_read(void);
static part_t *gpt_part_new(void);
static void gpt_pte_to_part(const gpt_entry_t *pte, part_t *part);
static void gpt_connection(ipc_callid_t iid, ipc_call_t *icall, void *arg);
static int gpt_bd_read(part_t *p, aoff64_t ba, size_t cnt, void *buf);
static int gpt_bd_write(part_t *p, aoff64_t ba, size_t cnt, const void *buf);
static int gpt_bsa_translate(part_t *p, aoff64_t ba, size_t cnt, aoff64_t *gba);

int main(int argc, char **argv)
{
	printf(NAME ": GUID partition table driver\n");

	if (argc != 2) {
		printf("Expected one argument (device name).\n");
		return -1;
	}

	if (gpt_init(argv[1]) != EOK)
		return -1;

	printf(NAME ": Accepting connections\n");
	task_retval(0);
	async_manager();

	/* Not reached */
	return 0;
}

static int gpt_init(const char *dev_name)
{
	int rc;
	int i;
	char *name;
	service_id_t dsid;
	uint64_t size_mb;
	part_t *part;

	rc = loc_service_get_id(dev_name, &indev_sid, 0);
	if (rc != EOK) {
		printf(NAME ": could not resolve device `%s'.\n", dev_name);
		return rc;
	}

	rc = block_init(EXCHANGE_SERIALIZE, indev_sid, 2048);
	if (rc != EOK)  {
		printf(NAME ": could not init libblock.\n");
		return rc;
	}

	/* Determine and verify block size. */

	rc = block_get_bsize(indev_sid, &block_size);
	if (rc != EOK) {
		printf(NAME ": error getting block size.\n");
		return rc;
	}

	if (block_size < 512 || (block_size % 512) != 0) {
		printf(NAME ": invalid block size %zu.\n", block_size);
		return ENOTSUP;
	}

	/* Read in partition records. */
	rc = gpt_read();
	if (rc != EOK)
		return rc;

	/* Register server with location service. */
	async_set_client_connection(gpt_connection);
	rc = loc_server_register(NAME);
	if (rc != EOK) {
		printf(NAME ": Unable to register server.\n");
		return rc;
	}
	
	/*
	 * Create partition devices.
	 */
	i = 0;
	part = plist_head.next;

	while (part != NULL) {
		/* Skip absent partitions. */
		if (!part->present) {
			part = part->next;
			++i;
			continue;
		}

		asprintf(&name, "%sp%d", dev_name, i);
		if (name == NULL)
			return ENOMEM;

		rc = loc_service_register(name, &dsid);
		if (rc != EOK) {
			printf(NAME ": Unable to register service %s.\n", name);
			return rc;
		}

		size_mb = (part->length * block_size + 1024 * 1024 - 1)
		    / (1024 * 1024);
		printf(NAME ": Registered device %s: %" PRIu64 " blocks "
		    "%" PRIuOFF64 " MB.\n", name, part->length, size_mb);

		part->dsid = dsid;
		free(name);

		part = part->next;
		++i;
	}

	return EOK;
}

/** Read in partition records. */
static int gpt_read(void)
{
	int i, rc;
	gpt_header_t *gpt_hdr;
	gpt_entry_t *etable;
	uint64_t ba;
	uint32_t bcnt;
	uint32_t esize;
	uint32_t num_entries;
	uint32_t entry;
	part_t *prev, *p;

	gpt_hdr = malloc(block_size);
	if (gpt_hdr == NULL) {
		printf(NAME ": Failed allocating memory.\n");
		return ENOMEM;
	}

	rc = block_read_direct(indev_sid, GPT_HDR_BA, 1, gpt_hdr);
	if (rc != EOK) {
		printf(NAME ": Failed reading GPT header block.\n");
		return rc;
	}

	for (i = 0; i < 8; ++i) {
		if (gpt_hdr->efi_signature[i] != efi_signature[i]) {
			printf(NAME ": Invalid GPT signature.\n");
			return EINVAL;
		}
	}

	plist_head.next = NULL;
	prev = &plist_head;

	num_entries = uint32_t_le2host(gpt_hdr->num_entries);
	ba = uint64_t_le2host(gpt_hdr->entry_lba);
	esize = uint32_t_le2host(gpt_hdr->entry_size);
	bcnt = (num_entries / esize) + ((num_entries % esize != 0) ? 1 : 0);

	etable = malloc(num_entries * esize);
	if (etable == NULL) {
		free(gpt_hdr);
		printf(NAME ": Failed allocating memory.\n");
		return ENOMEM;
	}

	rc = block_read_direct(indev_sid, ba, bcnt, etable);
	if (rc != EOK) {
		printf(NAME ": Failed reading GPT entries.\n");
		return rc;
	}

	for (entry = 0; entry < num_entries; ++entry) {
		p = gpt_part_new();
		if (p == NULL)
			return ENOMEM;

		gpt_pte_to_part(&etable[entry], p);
		prev->next = p;
		prev = p;
	}

	free(etable);

	return EOK;
}

/** Allocate a new @c part_t structure. */
static part_t *gpt_part_new(void)
{
	return malloc(sizeof(part_t));
}

/** Parse partition table entry. */
static void gpt_pte_to_part(const gpt_entry_t *pte, part_t *part)
{
	uint64_t sa, len;
	int i;

	/* Partition spans addresses [start_lba, end_lba] (inclusive). */
	sa = uint64_t_le2host(pte->start_lba);
	len = uint64_t_le2host(pte->end_lba) + 1 - sa;

	part->start_addr = sa;
	part->length     = len;

	part->present = false;

	for (i = 0; i < 8; ++i) {
		if (pte->part_type[i] != 0x00)
			part->present = true;
	}

	part->dsid = 0;
	part->next = NULL;
}

static void gpt_connection(ipc_callid_t iid, ipc_call_t *icall, void *arg)
{
	size_t comm_size;
	void *fs_va = NULL;
	ipc_callid_t callid;
	ipc_call_t call;
	sysarg_t method;
	service_id_t dh;
	unsigned int flags;
	int retval;
	aoff64_t ba;
	size_t cnt;
	part_t *part;

	/* Get the device handle. */
	dh = IPC_GET_ARG1(*icall);

	/* 
	 * Determine which partition device is the client connecting to.
	 * A linear search is not terribly fast, but we only do this
	 * once for each connection.
	 */
	part = plist_head.next;
	while (part != NULL && part->dsid != dh)
		part = part->next;

	if (part == NULL) {
		async_answer_0(iid, EINVAL);
		return;
	}

	assert(part->present == true);

	/* Answer the IPC_M_CONNECT_ME_TO call. */
	async_answer_0(iid, EOK);

	if (!async_share_out_receive(&callid, &comm_size, &flags)) {
		async_answer_0(callid, EHANGUP);
		return;
	}

	(void) async_share_out_finalize(callid, &fs_va);
	if (fs_va == AS_MAP_FAILED) {
		async_answer_0(callid, EHANGUP);
		return;
	}

	while (true) {
		callid = async_get_call(&call);
		method = IPC_GET_IMETHOD(call);
		
		if (!method) {
			/* The other side has hung up. */
			async_answer_0(callid, EOK);
			return;
		}
		
		switch (method) {
		case BD_READ_BLOCKS:
			ba = MERGE_LOUP32(IPC_GET_ARG1(call),
			    IPC_GET_ARG2(call));
			cnt = IPC_GET_ARG3(call);
			if (cnt * block_size > comm_size) {
				retval = ELIMIT;
				break;
			}
			retval = gpt_bd_read(part, ba, cnt, fs_va);
			break;
		case BD_WRITE_BLOCKS:
			ba = MERGE_LOUP32(IPC_GET_ARG1(call),
			    IPC_GET_ARG2(call));
			cnt = IPC_GET_ARG3(call);
			if (cnt * block_size > comm_size) {
				retval = ELIMIT;
				break;
			}
			retval = gpt_bd_write(part, ba, cnt, fs_va);
			break;
		case BD_GET_BLOCK_SIZE:
			async_answer_1(callid, EOK, block_size);
			continue;
		case BD_GET_NUM_BLOCKS:
			async_answer_2(callid, EOK, LOWER32(part->length),
			    UPPER32(part->length));
			continue;
		default:
			retval = EINVAL;
			break;
		}
		async_answer_0(callid, retval);
	}
}

/** Read blocks from partition. */
static int gpt_bd_read(part_t *p, aoff64_t ba, size_t cnt, void *buf)
{
	aoff64_t gba;

	if (gpt_bsa_translate(p, ba, cnt, &gba) != EOK)
		return ELIMIT;

	return block_read_direct(indev_sid, gba, cnt, buf);
}

/** Write blocks to partition. */
static int gpt_bd_write(part_t *p, aoff64_t ba, size_t cnt, const void *buf)
{
	aoff64_t gba;

	if (gpt_bsa_translate(p, ba, cnt, &gba) != EOK)
		return ELIMIT;

	return block_write_direct(indev_sid, gba, cnt, buf);
}

/** Translate block segment address with range checking. */
static int gpt_bsa_translate(part_t *p, aoff64_t ba, size_t cnt, aoff64_t *gba)
{
	if (ba + cnt > p->length)
		return ELIMIT;

	*gba = p->start_addr + ba;
	return EOK;
}

/**
 * @}
 */
