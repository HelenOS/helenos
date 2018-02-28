/*
 * Copyright (c) 2015 Jiri Svoboda
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
/** @file
 */

#ifndef LIBLABEL_STD_MBR_H_
#define LIBLABEL_STD_MBR_H_

#include <stdint.h>

enum {
	/** Block address of Master Boot Record. */
	mbr_ba = 0,

	/** First block allowed for allocation */
	mbr_ablock0 = 18,

	/** Number of primary partition records */
	mbr_nprimary = 4,

	/** Boot record signature */
	mbr_br_signature = 0xAA55,

	/** EBR PTE slot describing partition corresponding to this EBR */
	mbr_ebr_pte_this = 0,
	/** EBR PTE slot describing the next EBR */
	mbr_ebr_pte_next = 1
};

enum mbr_ptype {
	/** Unused partition entry */
	mbr_pt_unused	    = 0x00,
	/** Extended partition */
	mbr_pt_extended	    = 0x05,
	/** Extended partition with LBA */
	mbr_pt_extended_lba = 0x0f,
	/** FAT16 with LBA */
	mbr_pt_fat16_lba    = 0x0e,
	/** FAT32 with LBA */
	mbr_pt_fat32_lba    = 0x0c,
	/** IFS, HPFS, NTFS, exFAT */
	mbr_pt_ms_advanced  = 0x07,
	/** Minix */
	mbr_pt_minix        = 0x81,
	/** Linux */
	mbr_pt_linux        = 0x83,
	/** GPT Protective */
	mbr_pt_gpt_protect  = 0xee
};

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
} __attribute__((packed)) mbr_pte_t;

/** Structure of a boot-record block */
typedef struct {
	/* Area for boot code */
	uint8_t code_area[440];

	/* Optional media ID */
	uint32_t media_id;

	uint16_t pad0;

	/** Partition table entries */
	mbr_pte_t pte[mbr_nprimary];

	/** Boot record block signature (@c BR_SIGNATURE) */
	uint16_t signature;
} __attribute__((packed)) mbr_br_block_t;

#endif

/** @}
 */
