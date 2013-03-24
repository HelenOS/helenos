/*
 * Copyright (c) 2009 Jiri Svoboda, 2011, 2012, 2013 Dominik Taborsky
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

/** @addtogroup libgpt
 * @{
 */
/** @file
 */

#ifndef __GPT_H__
#define __GPT_H__

#define LIBGPT_NAME	"libgpt"

#include <loc.h>
#include <sys/types.h>

/** Block address of GPT header. */
#define GPT_HDR_BA	1
/** Block size of GPT header. */
#define GPT_HDR_BS	1
/** Minimum number of GPT partition entries */
#define GPT_MIN_PART_NUM 128
/** Basic number of GPT partition entries */
#define GPT_BASE_PART_NUM (GPT_MIN_PART_NUM)

/** GPT header signature ("EFI PART" in ASCII) */
extern const uint8_t efi_signature[8];

/** GPT header
 * - all in little endian.
 */
typedef struct {
	uint8_t efi_signature[8];
	uint32_t revision;
	uint32_t header_size;
	uint32_t header_crc32;
	uint32_t reserved;
	uint64_t my_lba;
	uint64_t alternate_lba;
	uint64_t first_usable_lba;
	uint64_t last_usable_lba;
	uint8_t disk_guid[16];
	uint64_t entry_lba;
	uint32_t num_entries;
	uint32_t entry_size;
	uint32_t pe_array_crc32;
} __attribute__((packed)) gpt_header_t;

typedef struct {
	/** Raw header. Has more bytes alloced than sizeof(gpt_header_t)!
	 * See gpt_read_gpt_header() to know why. */
	gpt_header_t * raw_data;
	/** Device where the data are from */
	service_id_t device;
	/** Linked list of partitions (initially NULL) */
} gpt_t;

/** GPT partition entry */
typedef struct {
	uint8_t part_type[16];
	uint8_t part_id[16];
	uint64_t start_lba;
	uint64_t end_lba;
	uint64_t attributes;
	uint8_t part_name[72];
} __attribute__((packed)) gpt_entry_t;

typedef struct g_part {
	/** Partition entry is in use */
	bool present;
	/** Address of first block */
	aoff64_t start_addr;
	/** Number of blocks */
	aoff64_t length;
	/** Raw data access */
	gpt_entry_t raw_data;	//TODO: a pointer or just a member?
}gpt_part_t;

typedef struct gpt_parts {
	/** Number of entries */
	unsigned int num_ent;
	/** Size of the array */
	unsigned int arr_size;
	/** Resizable partition array */
	gpt_entry_t * part_array;
} gpt_partitions_t;


typedef struct gpt_table {
	gpt_t * gpt;
	gpt_partitions_t * parts;
} gpt_table_t;

struct partition_type {
	const char * desc;
	const char * guid;
};

extern const struct partition_type gpt_ptypes[];



extern gpt_t * gpt_read_gpt_header(service_id_t dev_handle);
extern int gpt_write_gpt_header(gpt_t * header, service_id_t dev_handle);

extern gpt_partitions_t * gpt_read_partitions(gpt_t * gpt);
extern int 				  gpt_write_partitions(gpt_partitions_t * parts, gpt_t * header, service_id_t dev_handle);
extern gpt_partitions_t * gpt_add_partition(gpt_partitions_t * parts, gpt_part_t * partition);
extern gpt_partitions_t * gpt_remove_partition(gpt_partitions_t * parts, size_t idx);
extern void 			  gpt_set_part_type(gpt_part_t * p, int type);
extern void 			  gpt_set_part_name(gpt_entry_t * p, char * name[], size_t length);

extern void gpt_free_gpt(gpt_t * gpt);
extern void gpt_free_partitions(gpt_partitions_t * parts);

#endif

