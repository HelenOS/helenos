/*
 * Copyright (c) 2009 Jiri Svoboda
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
 * @brief PC MBR partition driver
 *
 * Handles the PC MBR partitioning scheme. Uses a block device
 * and provides one for each partition.
 *
 * Limitations:
 * 
 * Only works with boot records using LBA. CHS-only records are not
 * supported.
 *
 * Referemces:
 *	
 * The source of MBR structures for this driver have been the following
 * Wikipedia articles:
 *	- http://en.wikipedia.org/wiki/Master_Boot_Record
 *	- http://en.wikipedia.org/wiki/Extended_boot_record
 *
 * The fact that the extended partition has type 0x05 is pure observation.
 * (TODO: can it have any other type number?)
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
#include <errno.h>
#include <bool.h>
#include <byteorder.h>
#include <assert.h>
#include <macros.h>
#include <task.h>

#define NAME "mbr_part"

enum {
	/** Number of primary partition records */
	N_PRIMARY	= 4,

	/** Boot record signature */
	BR_SIGNATURE	= 0xAA55
};

enum ptype {
	/** Unused partition entry */
	PT_UNUSED	= 0x00,
	/** Extended partition */
	PT_EXTENDED	= 0x05,
};

/** Partition */
typedef struct part {
	/** Primary partition entry is in use */
	bool present;
	/** Address of first block */
	aoff64_t start_addr;
	/** Number of blocks */
	aoff64_t length;
	/** Device representing the partition (outbound device) */
	service_id_t dsid;
	/** Points to next partition structure. */
	struct part *next;
} part_t;

/** Structure of a partition table entry */
typedef struct {
	uint8_t status;
	/** CHS of fist block in partition */
	uint8_t first_chs[3];
	/** Partition type */
	uint8_t ptype;
	/** CHS of last block in partition */
	uint8_t last_chs[3];
	/** LBA of first block in partition */
	uint32_t first_lba;
	/** Number of blocks in partition */
	uint32_t length;
} __attribute__((packed)) pt_entry_t;

/** Structure of a boot-record block */
typedef struct {
	/* Area for boot code */
	uint8_t code_area[440];

	/* Optional media ID */
	uint32_t media_id;

	uint16_t pad0;

	/** Partition table entries */
	pt_entry_t pte[N_PRIMARY];

	/** Boot record block signature (@c BR_SIGNATURE) */
	uint16_t signature;
} __attribute__((packed)) br_block_t;


static size_t block_size;

/** Partitioned device (inbound device) */
static service_id_t indef_sid;

/** List of partitions. This structure is an empty head. */
static part_t plist_head;

static int mbr_init(const char *dev_name);
static int mbr_part_read(void);
static part_t *mbr_part_new(void);
static void mbr_pte_to_part(uint32_t base, const pt_entry_t *pte, part_t *part);
static void mbr_connection(ipc_callid_t iid, ipc_call_t *icall, void *arg);
static int mbr_bd_read(part_t *p, uint64_t ba, size_t cnt, void *buf);
static int mbr_bd_write(part_t *p, uint64_t ba, size_t cnt, const void *buf);
static int mbr_bsa_translate(part_t *p, uint64_t ba, size_t cnt, uint64_t *gba);

int main(int argc, char **argv)
{
	printf(NAME ": PC MBR partition driver\n");

	if (argc != 2) {
		printf("Expected one argument (device name).\n");
		return -1;
	}

	if (mbr_init(argv[1]) != EOK)
		return -1;

	printf(NAME ": Accepting connections\n");
	task_retval(0);
	async_manager();

	/* Not reached */
	return 0;
}

static int mbr_init(const char *dev_name)
{
	int rc;
	int i;
	char *name;
	service_id_t dsid;
	uint64_t size_mb;
	part_t *part;

	rc = loc_service_get_id(dev_name, &indef_sid, 0);
	if (rc != EOK) {
		printf(NAME ": could not resolve device `%s'.\n", dev_name);
		return rc;
	}

	rc = block_init(EXCHANGE_SERIALIZE, indef_sid, 2048);
	if (rc != EOK)  {
		printf(NAME ": could not init libblock.\n");
		return rc;
	}

	/* Determine and verify block size. */

	rc = block_get_bsize(indef_sid, &block_size);
	if (rc != EOK) {
		printf(NAME ": error getting block size.\n");
		return rc;
	}

	if (block_size < 512 || (block_size % 512) != 0) {
		printf(NAME ": invalid block size %zu.\n", block_size);
		return ENOTSUP;
	}

	/* Read in partition records. */
	rc = mbr_part_read();
	if (rc != EOK)
		return rc;

	/* Register server with location service. */
	async_set_client_connection(mbr_connection);
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
		printf(NAME ": Registered device %s: %" PRIuOFF64 " blocks "
		    "%" PRIu64 " MB.\n", name, part->length, size_mb);

		part->dsid = dsid;
		free(name);

		part = part->next;
		++i;
	}

	return EOK;
}

/** Read in partition records. */
static int mbr_part_read(void)
{
	int i, rc;
	br_block_t *brb;
	uint16_t sgn;
	uint32_t ba;
	part_t *ext_part, cp;
	uint32_t base;
	part_t *prev, *p;

	brb = malloc(sizeof(br_block_t));
	if (brb == NULL) {
		printf(NAME ": Failed allocating memory.\n");
		return ENOMEM;
	}

	/*
	 * Read primary partition entries.
	 */

	rc = block_read_direct(indef_sid, 0, 1, brb);
	if (rc != EOK) {
		printf(NAME ": Failed reading MBR block.\n");
		return rc;
	}

	sgn = uint16_t_le2host(brb->signature);
	if (sgn != BR_SIGNATURE) {
		printf(NAME ": Invalid boot record signature 0x%04" PRIX16
		    ".\n", sgn);
		return EINVAL;
	}

	ext_part = NULL;
	plist_head.next = NULL;
	prev = &plist_head;

	for (i = 0; i < N_PRIMARY; ++i) {
		p = mbr_part_new();
		if (p == NULL)
			return ENOMEM;

		mbr_pte_to_part(0, &brb->pte[i], p);
		prev->next = p;
		prev = p;

		if (brb->pte[i].ptype == PT_EXTENDED) {
			p->present = false;
			ext_part = p;
		}
	}

	if (ext_part == NULL)
		return EOK;

	printf("Extended partition found.\n");

	/*
	 * Read extended partition entries.
	 */

	cp.start_addr = ext_part->start_addr;
	cp.length = ext_part->length;
	base = ext_part->start_addr;

	do {
		/*
		 * Addressing in the EBR chain is relative to the beginning
		 * of the extended partition.
		 */
		ba = cp.start_addr;
		rc = block_read_direct(indef_sid, ba, 1, brb);
		if (rc != EOK) {
			printf(NAME ": Failed reading EBR block at %"
			    PRIu32 ".\n", ba);
			return rc;
		}

		sgn = uint16_t_le2host(brb->signature);
		if (sgn != BR_SIGNATURE) {
			printf(NAME ": Invalid boot record signature 0x%04"
			    PRIX16 " in EBR at %" PRIu32 ".\n", sgn, ba);
			return EINVAL;
		}

		p = mbr_part_new();
		if (p == NULL)
			return ENOMEM;

		/* First PTE is the logical partition itself. */
		mbr_pte_to_part(base, &brb->pte[0], p);
		prev->next = p;
		prev = p;

		/* Second PTE describes next chain element. */
		mbr_pte_to_part(base, &brb->pte[1], &cp);
	} while (cp.present);

	return EOK;
}

/** Allocate a new @c part_t structure. */
static part_t *mbr_part_new(void)
{
	return malloc(sizeof(part_t));
}

/** Parse partition table entry. */
static void mbr_pte_to_part(uint32_t base, const pt_entry_t *pte, part_t *part)
{
	uint32_t sa, len;

	sa = uint32_t_le2host(pte->first_lba);
	len = uint32_t_le2host(pte->length);

	part->start_addr = base + sa;
	part->length     = len;

	part->present = (pte->ptype != PT_UNUSED) ? true : false;

	part->dsid = 0;
	part->next = NULL;
}

static void mbr_connection(ipc_callid_t iid, ipc_call_t *icall, void *arg)
{
	size_t comm_size;
	void *fs_va = NULL;
	ipc_callid_t callid;
	ipc_call_t call;
	sysarg_t method;
	service_id_t dh;
	unsigned int flags;
	int retval;
	uint64_t ba;
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

	while (1) {
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
			retval = mbr_bd_read(part, ba, cnt, fs_va);
			break;
		case BD_WRITE_BLOCKS:
			ba = MERGE_LOUP32(IPC_GET_ARG1(call),
			    IPC_GET_ARG2(call));
			cnt = IPC_GET_ARG3(call);
			if (cnt * block_size > comm_size) {
				retval = ELIMIT;
				break;
			}
			retval = mbr_bd_write(part, ba, cnt, fs_va);
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
static int mbr_bd_read(part_t *p, uint64_t ba, size_t cnt, void *buf)
{
	uint64_t gba;

	if (mbr_bsa_translate(p, ba, cnt, &gba) != EOK)
		return ELIMIT;

	return block_read_direct(indef_sid, gba, cnt, buf);
}

/** Write blocks to partition. */
static int mbr_bd_write(part_t *p, uint64_t ba, size_t cnt, const void *buf)
{
	uint64_t gba;

	if (mbr_bsa_translate(p, ba, cnt, &gba) != EOK)
		return ELIMIT;

	return block_write_direct(indef_sid, gba, cnt, buf);
}

/** Translate block segment address with range checking. */
static int mbr_bsa_translate(part_t *p, uint64_t ba, size_t cnt, uint64_t *gba)
{
	if (ba + cnt > p->length)
		return ELIMIT;

	*gba = p->start_addr + ba;
	return EOK;
}

/**
 * @}
 */
