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

#ifndef LIBMBR_LIBMBR_H_
#define LIBMBR_LIBMBR_H_

#include <sys/types.h>

#define LIBMBR_NAME	"libmbr"

#ifdef DEBUG_CONFIG
#include <stdio.h>
#include <str_error.h>
#define DEBUG_PRINT_0(str) \
	printf("%s:%d: " str, __FILE__, __LINE__)
#define DEBUG_PRINT_1(str, arg1) \
	printf("%s:%d: " str, __FILE__, __LINE__, arg1)
#define DEBUG_PRINT_2(str, arg1, arg2) \
	printf("%s:%d: " str, __FILE__, __LINE__, arg1, arg2)
#define DEBUG_PRINT_3(str, arg1, arg2, arg3) \
	printf("%s:%d: " str, __FILE__, __LINE__, arg1, arg2, arg3)
#else
#define DEBUG_PRINT_0(str)
#define DEBUG_PRINT_1(str, arg1)
#define DEBUG_PRINT_2(str, arg1, arg2)
#define DEBUG_PRINT_3(str, arg1, arg2, arg3)
#endif

/** Number of primary partition records */
#define N_PRIMARY		4

/** Boot record signature */
#define BR_SIGNATURE	0xAA55

enum {
	/** Non-bootable */
	B_INACTIVE = 0x00,
	/** Bootable */
	B_ACTIVE = 0x80,
	/** Anything else means invalid */
};

typedef enum {
	/** Bootability */
	ST_BOOT = 0,
	/** Logical partition, 0 = primary, 1 = logical*/
	ST_LOGIC = 1
} MBR_FLAGS;

enum {
	/** Unused partition entry */
	PT_UNUSED	= 0x00,
	/** Extended partition */
	PT_EXTENDED	= 0x05,
	/** GPT Protective partition */
	PT_GPT	= 0xEE,
};

typedef enum {
	/** No error */
	ERR_OK = 0,
	/** All primary partitions already present */
	ERR_PRIMARY_FULL,
	/** Extended partition already present */
	ERR_EXTENDED_PRESENT,
	/** No extended partition present */
	ERR_NO_EXTENDED,
	/** Partition overlapping */
	ERR_OVERLAP,
	/** Logical partition out of bounds */
	ERR_OUT_BOUNDS,
} MBR_ERR_VAL;


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

/** MBR header */
typedef struct {
	/** Raw access to data */
	br_block_t raw_data;
	/** Device where the data are from */
	service_id_t device;
} mbr_t;


/** Partition */
typedef struct mbr_part {
	/** The link in the doubly-linked list */
	link_t link;
	/** Partition type */
	uint8_t type;
	/** Flags */
	uint8_t status;
	/** Address of first block */
	uint32_t start_addr;
	/** Number of blocks */
	uint32_t length;
	/** Points to Extended Boot Record of logical partition */
	br_block_t * ebr;
} mbr_part_t;

/** Partition list structure */
typedef struct mbr_parts {
	/** Number of primary partitions */
	unsigned char n_primary;
	/** Index to the extended partition in the array */
	link_t * l_extended;
	/** Number of logical partitions */
	unsigned int n_logical;
	/** Logical partition linked list */
	list_t list;
} mbr_partitions_t;

/** Both header and partition list */
typedef struct mbr_table {
	mbr_t * mbr;
	mbr_partitions_t * parts;
} mbr_table_t;

/** Read/Write MBR header.
 * WARNING: when changing both header and partitions, write first header,
 * then partitions. The MBR headers' raw_data is NOT updated to follow
 * partition changes. */
extern mbr_t * mbr_read_mbr(service_id_t dev_handle);
extern int mbr_write_mbr(mbr_t * mbr, service_id_t dev_handle);
extern int mbr_is_mbr(mbr_t * mbr);

/** Read/Write/Set MBR partitions.
 * NOTE: Writing partitions writes the complete header as well. */
extern mbr_partitions_t * mbr_read_partitions(mbr_t * mbr);
extern int 			mbr_write_partitions(mbr_partitions_t * parts, mbr_t * mbr, service_id_t dev_handle);
extern mbr_part_t *	mbr_alloc_partition(void);
extern mbr_partitions_t * mbr_alloc_partitions(void);
extern int			mbr_add_partition(mbr_partitions_t * parts, mbr_part_t * partition);
extern int			mbr_remove_partition(mbr_partitions_t * parts, size_t idx);
extern int			mbr_get_flag(mbr_part_t * p, MBR_FLAGS flag);
extern void			mbr_set_flag(mbr_part_t * p, MBR_FLAGS flag, bool value);

#define mbr_part_foreach(parts, iterator)	\
			for (mbr_part_t * iterator = list_get_instance((parts)->list.head.next, mbr_part_t, link); \
				iterator != list_get_instance(&(parts)->list.head, mbr_part_t, link); \
				iterator = list_get_instance(iterator->link.next, mbr_part_t, link))


/** free() wrapper functions. */
extern void mbr_free_mbr(mbr_t * mbr);
extern void mbr_free_partition(mbr_part_t * p);
extern void mbr_free_partitions(mbr_partitions_t * parts);

#endif

