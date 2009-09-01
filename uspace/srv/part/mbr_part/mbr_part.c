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
 * supported. Maximum number of partitions is fixed.
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <ipc/ipc.h>
#include <ipc/bd.h>
#include <async.h>
#include <as.h>
#include <fibril_sync.h>
#include <devmap.h>
#include <sys/types.h>
#include <libblock.h>
#include <devmap.h>
#include <errno.h>
#include <bool.h>
#include <byteorder.h>
#include <macros.h>
#include <task.h>

#define NAME "mbr_part"

enum {
	/** Number of primary partition records */
	N_PRIMARY	= 4,

	/** Boot record signature */
	BR_SIGNATURE	= 0xAA55,

	/** Maximum number of partitions */
	MAX_PART	= 64
};

enum ptype {
	/** Extended partition */
	PT_EXTENDED	= 0x05
};

/** Partition */
typedef struct {
	/** Primary partition entry is in use */
	bool present;
	/** Address of first block */
	bn_t start_addr;
	/** Number of blocks */
	bn_t length;
	/** Device representing the partition (outbound device) */
	dev_handle_t dev;
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
static dev_handle_t indev_handle;

static part_t primary[MAX_PART];

static int mbr_init(const char *dev_name);
static int mbr_part_read(void);
static void mbr_pte_to_part(uint32_t base, const pt_entry_t *pte, part_t *part);
static void mbr_connection(ipc_callid_t iid, ipc_call_t *icall);
static int mbr_bd_read(part_t *p, uint64_t ba, size_t cnt, void *buf);
static int mbr_bd_write(part_t *p, uint64_t ba, size_t cnt, const void *buf);
static int mbr_bsa_translate(part_t *p, uint64_t ba, size_t cnt, uint64_t *gba);

/** Total number of partitions entries in @c primary (including non-present). */
static int total_part;

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
	dev_handle_t dev;
	uint64_t size_mb;

	rc = devmap_device_get_handle(dev_name, &indev_handle, 0);
	if (rc != EOK) {
		printf(NAME ": could not resolve device `%s'.\n", dev_name);
		return rc;
	}

	rc = block_init(indev_handle, 2048);
	if (rc != EOK)  {
		printf(NAME ": could not init libblock.\n");
		return rc;
	}

	/* Determine and verify block size. */

	rc = block_get_bsize(indev_handle, &block_size);
	if (rc != EOK) {
		printf(NAME ": error getting block size.\n");
		return rc;
	}

	if (block_size < 512 || (block_size % 512) != 0) {
		printf(NAME ": invalid block size %d.\n");
		return ENOTSUP;
	}

	/* Read in partition records. */
	rc = mbr_part_read();
	if (rc != EOK)
		return rc;

	/* Register the driver with device mapper. */
	rc = devmap_driver_register(NAME, mbr_connection);
	if (rc != EOK) {
		printf(NAME ": Unable to register driver.\n");
		return rc;
	}

	/* Create partition devices. */
	for (i = 0; i < total_part; ++i) {
		/* Skip absent partitions. */
		if (!primary[i].present)
			continue;

		asprintf(&name, "%sp%d", dev_name, i);
		if (name == NULL)
			return ENOMEM;

		rc = devmap_device_register(name, &dev);
		if (rc != EOK) {
			devmap_hangup_phone(DEVMAP_DRIVER);
			printf(NAME ": Unable to register device %s.\n", name);
			return rc;
		}

		size_mb = (primary[i].length * block_size + 1024 * 1024 - 1)
		    / (1024 * 1024);
		printf(NAME ": Registered device %s: %llu blocks %llu MB.\n",
		    name, primary[i].length, size_mb);

		primary[i].dev = dev;
		free(name);
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

	brb = malloc(sizeof(br_block_t));
	if (brb == NULL) {
		printf(NAME ": Failed allocating memory.\n");
		return ENOMEM;	
	}

	/*
	 * Read primary partition entries.
	 */

	rc = block_read_direct(indev_handle, 0, 1, brb);
	if (rc != EOK) {
		printf(NAME ": Failed reading MBR block.\n");
		return rc;
	}

	sgn = uint16_t_le2host(brb->signature);
	if (sgn != BR_SIGNATURE) {
		printf(NAME ": Invalid boot record signature 0x%04X.\n", sgn);
		return EINVAL;
	}

	ext_part = NULL;

	for (i = 0; i < N_PRIMARY; ++i) {
		mbr_pte_to_part(0, &brb->pte[i], &primary[i]);

		if (brb->pte[i].ptype == PT_EXTENDED) {
			primary[i].present = false;
			ext_part = &primary[i];
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
		rc = block_read_direct(indev_handle, ba, 1, brb);
		if (rc != EOK) {
			printf(NAME ": Failed reading EBR block at %u.\n", ba);
			return rc;
		}

		sgn = uint16_t_le2host(brb->signature);
		if (sgn != BR_SIGNATURE) {
			printf(NAME ": Invalid boot record signature 0x%04X "
			    " in EBR at %u.\n", sgn, ba);
			return EINVAL;
		}

		/* First PTE is the logical partition itself. */
		mbr_pte_to_part(base, &brb->pte[0], &primary[i]);
		++i;

		/* Second PTE describes next chain element. */
		mbr_pte_to_part(base, &brb->pte[1], &cp);
	} while (cp.present && i < MAX_PART);

	total_part = i;

	return EOK;
}

/** Parse partition table entry. */
static void mbr_pte_to_part(uint32_t base, const pt_entry_t *pte, part_t *part)
{
	uint32_t sa, len;

	sa = uint32_t_le2host(pte->first_lba);
	len = uint32_t_le2host(pte->length);

	part->start_addr = base + sa;
	part->length     = len;

	part->present = (sa != 0 || len != 0) ? true : false;
	part->dev = 0;
}

static void mbr_connection(ipc_callid_t iid, ipc_call_t *icall)
{
	size_t comm_size;
	void *fs_va = NULL;
	ipc_callid_t callid;
	ipc_call_t call;
	ipcarg_t method;
	dev_handle_t dh;
	int flags;
	int retval;
	uint64_t ba;
	size_t cnt;
	int pidx, i;
	part_t *part;

	/* Get the device handle. */
	dh = IPC_GET_ARG1(*icall);

	/* Determine which partition device is the client connecting to. */
	pidx = -1;
	for (i = 0; i < total_part; i++)
		if (primary[i].dev == dh)
			pidx = i;

	if (pidx < 0/* || disk[disk_id].present == false*/) {
		ipc_answer_0(iid, EINVAL);
		return;
	}

	part = &primary[pidx];

	/* Answer the IPC_M_CONNECT_ME_TO call. */
	ipc_answer_0(iid, EOK);

	if (!ipc_share_out_receive(&callid, &comm_size, &flags)) {
		ipc_answer_0(callid, EHANGUP);
		return;
	}

	fs_va = as_get_mappable_page(comm_size);
	if (fs_va == NULL) {
		ipc_answer_0(callid, EHANGUP);
		return;
	}

	(void) ipc_share_out_finalize(callid, fs_va);

	while (1) {
		callid = async_get_call(&call);
		method = IPC_GET_METHOD(call);
		switch (method) {
		case IPC_M_PHONE_HUNGUP:
			/* The other side has hung up. */
			ipc_answer_0(callid, EOK);
			return;
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
			ipc_answer_1(callid, EOK, block_size);
			continue;

		default:
			retval = EINVAL;
			break;
		}
		ipc_answer_0(callid, retval);
	}
}

/** Read blocks from partition. */
static int mbr_bd_read(part_t *p, uint64_t ba, size_t cnt, void *buf)
{
	uint64_t gba;

	if (mbr_bsa_translate(p, ba, cnt, &gba) != EOK)
		return ELIMIT;

	return block_read_direct(indev_handle, gba, cnt, buf);
}

/** Write blocks to partition. */
static int mbr_bd_write(part_t *p, uint64_t ba, size_t cnt, const void *buf)
{
	uint64_t gba;

	if (mbr_bsa_translate(p, ba, cnt, &gba) != EOK)
		return ELIMIT;

	return block_write_direct(indev_handle, gba, cnt, buf);
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
