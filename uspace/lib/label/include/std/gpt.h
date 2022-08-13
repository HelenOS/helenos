/*
 * SPDX-FileCopyrightText: 2015 Jiri Svoboda
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup liblabel
 * @{
 */
/** @file
 */

#ifndef LIBLABEL_STD_GPT_H_
#define LIBLABEL_STD_GPT_H_

#include <stdint.h>

enum {
	/** Block address of primary GPT header. */
	gpt_hdr_ba = 1,

	/** Minimum size of partition table in bytes, required by std. */
	gpt_ptable_min_size = 16384,

	/** GPT revision */
	gpt_revision = 0x00010000
};

/** GPT header */
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

/** GPT partition entry */
typedef struct {
	uint8_t part_type[16];
	uint8_t part_id[16];
	uint64_t start_lba;
	uint64_t end_lba;
	uint64_t attributes;
	uint16_t part_name[36];
} __attribute__((packed)) gpt_entry_t;

/** Microsoft Basic Data Partition */
#define GPT_MS_BASIC_DATA "EBD0A0A2-B9E5-4433-87C0-68B6B72699C7"
/** Linux Filesystem Data */
#define GPT_LINUX_FS_DATA "0FC63DAF-8483-4772-8E79-3D69D8477DE4"
/** I could not find any definition of Minix GUID partition type.
 * This is a randomly generated UUID
 */
#define GPT_MINIX_FAKE "8308e350-4e2d-46c7-8e3b-24b07e8ac674"

#endif

/** @}
 */
