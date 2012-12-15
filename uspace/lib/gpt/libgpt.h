/*
 * Copyright (c) 2009 Jiri Svoboda, 2011 Dominik Taborsky
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
const uint8_t efi_signature[8] = {
	0x45, 0x46, 0x49, 0x20, 0x50, 0x41, 0x52, 0x54
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

typedef struct {
	/** Raw header. Has more bytes alloced than sizeof(gpt_header_t)!
	 * See gpt_read_gpt_header() to know why. */
	gpt_header_t * raw_data;
	/** Device where the data are from */
	service_id_t device;
	/** Linked list of partitions (initially NULL) */
	//g_part_t * partitions;	//shall we keep this? same problem as in libmbr
	//NOTE: if we have partition list here, do we free() it or not? 
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
}g_part_t;

typedef struct gpt_parts {
	/** Number of entries */
	unsigned int num_ent;
	/** Size of the array */
	unsigned int arr_size;
	/** Resizable partition array */
	gpt_entry_t * part_array;
	
} gpt_parts_t;

struct partition_type {
	const char * desc;
	const char * guid;
}

struct partition_type gpt_ptypes[] {
	{ "Unused entry",					"00000000-0000-0000-0000-000000000000" },
	{ "MBR partition scheme",			"024DEE41-33E7-11D3-9D69-0008C781F39F" },
	{ "EFI System",						"C12A7328-F81F-11D2-BA4B-00A0C93EC93B" },
	{ "BIOS Boot",						"21686148-6449-6E6F-744E-656564454649" },
	{ "Windows Reserved",				"E3C9E316-0B5C-4DB8-817D-F92DF00215AE" },
	{ "Windows Basic data",				"EBD0A0A2-B9E5-4433-87C0-68B6B72699C7" },
	{ "Windows LDM metadata", 			"5808C8AA-7E8F-42E0-85D2-E1E90434CFB3" },
	{ "Windows LDM data", 				"AF9B60A0-1431-4F62-BC68-3311714A69AD" },
	{ "Windows Recovery Environment",	"DE94BBA4-06D1-4D40-A16A-BFD50179D6AC" },
	{ "Windows IBM GPFS",				"37AFFC90-EF7D-4E96-91C3-2D7AE055B174" },
	{ "Windows Cluster metadata",		"DB97DBA9-0840-4BAE-97F0-FFB9A327C7E1" },
	{ "HP-UX Data",						"75894C1E-3AEB-11D3-B7C1-7B03A0000000" },
	{ "HP-UX Service",					"E2A1E728-32E3-11D6-A682-7B03A0000000" },
	{ "Linux filesystem data",			"EBD0A0A2-B9E5-4433-87C0-68B6B72699C7" },
	{ "Linux filesystem data",			"0FC63DAF-8483-4772-8E79-3D69D8477DE4" },
	{ "Linux RAID",						"A19D880F-05FC-4D3B-A006-743F0F84911E" },
	{ "Linux Swap",						"0657FD6D-A4AB-43C4-84E5-0933C84B4F4F" },
	{ "Linux LVM",						"E6D6D379-F507-44C2-A23C-238F2A3DF928" },
	{ "Linux Reserved",					"8DA63339-0007-60C0-C436-083AC8230908" },
	{ "FreeBSD Boot",					"83BD6B9D-7F41-11DC-BE0B-001560B84F0F" },
	{ "FreeBSD Data",					"516E7CB4-6ECF-11D6-8FF8-00022D09712B" },
	{ "FreeBSD Swap",					"516E7CB5-6ECF-11D6-8FF8-00022D09712B" },
	{ "FreeBSD UFS", 					"516E7CB6-6ECF-11D6-8FF8-00022D09712B" },
	{ "FreeBSD Vinum VM",				"516E7CB8-6ECF-11D6-8FF8-00022D09712B" },
	{ "FreeBSD ZFS",					"516E7CBA-6ECF-11D6-8FF8-00022D09712B" },
	{ "Mac OS X HFS+",					"48465300-0000-11AA-AA11-00306543ECAC" },
	{ "Mac OS X UFS",					"55465300-0000-11AA-AA11-00306543ECAC" },
	{ "Mac OS X ZFS",					"6A898CC3-1DD2-11B2-99A6-080020736631" },
	{ "Mac OS X RAID",					"52414944-0000-11AA-AA11-00306543ECAC" },
	{ "Mac OS X RAID, offline",			"52414944-5F4F-11AA-AA11-00306543ECAC" },
	{ "Mac OS X Boot",					"426F6F74-0000-11AA-AA11-00306543ECAC" },
	{ "Mac OS X Label",					"4C616265-6C00-11AA-AA11-00306543ECAC" },
	{ "Mac OS X TV Recovery",			"5265636F-7665-11AA-AA11-00306543ECAC" },
	{ "Mac OS X Core Storage",			"53746F72-6167-11AA-AA11-00306543ECAC" },
	{ "Solaris Boot",					"6A82CB45-1DD2-11B2-99A6-080020736631" },
	{ "Solaris Root",					"6A85CF4D-1DD2-11B2-99A6-080020736631" },
	{ "Solaris Swap",					"6A87C46F-1DD2-11B2-99A6-080020736631" },
	{ "Solaris Backup",					"6A8B642B-1DD2-11B2-99A6-080020736631" },
	{ "Solaris /usr",					"6A898CC3-1DD2-11B2-99A6-080020736631" },
	{ "Solaris /var",					"6A8EF2E9-1DD2-11B2-99A6-080020736631" },
	{ "Solaris /home",					"6A90BA39-1DD2-11B2-99A6-080020736631" },
	{ "Solaris Alternate sector",		"6A9283A5-1DD2-11B2-99A6-080020736631" },
	{ "Solaris Reserved",				"6A945A3B-1DD2-11B2-99A6-080020736631" },
	{ "Solaris Reserved",				"6A9630D1-1DD2-11B2-99A6-080020736631" },
	{ "Solaris Reserved",				"6A980767-1DD2-11B2-99A6-080020736631" },
	{ "Solaris Reserved",				"6A96237F-1DD2-11B2-99A6-080020736631" },
	{ "Solaris Reserved",				"6A8D2AC7-1DD2-11B2-99A6-080020736631" },
	{ "NetBSD Swap",					"49F48D32-B10E-11DC-B99B-0019D1879648" },
	{ "NetBSD FFS",						"49F48D5A-B10E-11DC-B99B-0019D1879648" },
	{ "NetBSD LFS",						"49F48D82-B10E-11DC-B99B-0019D1879648" },
	{ "NetBSD RAID",					"49F48DAA-B10E-11DC-B99B-0019D1879648" },
	{ "NetBSD Concatenated",			"2DB519C4-B10F-11DC-B99B-0019D1879648" },
	{ "NetBSD Encrypted",				"2DB519EC-B10F-11DC-B99B-0019D1879648" },
	{ "ChromeOS ChromeOS kernel",		"FE3A2A5D-4F32-41A7-B725-ACCC3285A309" },
	{ "ChromeOS rootfs",				"3CB8E202-3B7E-47DD-8A3C-7FF2A13CFCEC" },
	{ "ChromeOS future use",			"2E0A753D-9E48-43B0-8337-B15192CB1B5E" },
	{ "MidnightBSD Boot",				"85D5E45E-237C-11E1-B4B3-E89A8F7FC3A7" },
	{ "MidnightBSD Data",				"85D5E45A-237C-11E1-B4B3-E89A8F7FC3A7" },
	{ "MidnightBSD Swap",				"85D5E45B-237C-11E1-B4B3-E89A8F7FC3A7" },
	{ "MidnightBSD UFS",				"0394Ef8B-237E-11E1-B4B3-E89A8F7FC3A7" },
	{ "MidnightBSD Vinum VM",			"85D5E45C-237C-11E1-B4B3-E89A8F7FC3A7" },
	{ "MidnightBSD ZFS",				"85D5E45D-237C-11E1-B4B3-E89A8F7FC3A7" }
};


extern gpt_t * gpt_read_gpt_header(service_id_t dev_handle);
extern int gpt_write_gpt_header(gpt_header_t header, service_id_t dev_handle);

extern gpt_parts_t * gpt_read_partitions(gpt_t * gpt);
extern int gpt_write_partitions(gpt_parts_t * parts, gpt_t * header);
extern gpt_parts_t * gpt_add_partition(gpt_parts_t * parts, g_part_t partition);
extern gpt_parts_t * gpt_remove_partition(gpt_parts_t * parts, int idx);

extern void gpt_free_gpt(gpt_t * gpt);
extern void gpt_free_partitions(gpt_parts_t * parts);

#endif
