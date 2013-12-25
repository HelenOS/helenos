/*
 * Copyright (c) 2009 Jiri Svoboda
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

 /** @addtogroup libmbr
 * @{
 */
/** @file
 */

#ifndef LIBMBR_MBR_H_
#define LIBMBR_MBR_H_

#include <sys/types.h>

enum {
	/** Number of primary partition records */
	N_PRIMARY = 4,
	
	/** Boot record signature */
	BR_SIGNATURE = 0xAA55
};

enum {
	/** Non-bootable */
	B_INACTIVE = 0x00,
	/** Bootable */
	B_ACTIVE = 0x80,
	/** Anything else means invalid */
};

enum {
	/** Unused partition entry */
	PT_UNUSED = 0x00,
	/** Extended partition */
	PT_EXTENDED = 0x05,
	/** Extended partition with LBA */
	PT_EXTENDED_LBA = 0x0F,
	/** GPT Protective partition */
	PT_GPT = 0xEE,
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
} __attribute__((packed)) pt_entry_t;

/** Structure of a boot-record block */
typedef struct {
	/** Area for boot code */
	uint8_t code_area[440];
	/** Optional media ID */
	uint32_t media_id;
	/** Padding */
	uint16_t pad0;
	/** Partition table entries */
	pt_entry_t pte[N_PRIMARY];
	/** Boot record block signature (@c BR_SIGNATURE) */
	uint16_t signature;
} __attribute__((packed)) br_block_t;

#endif

