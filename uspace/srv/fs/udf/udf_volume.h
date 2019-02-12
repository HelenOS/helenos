/*
 * Copyright (c) 2012 Julia Medvedeva
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

/** @addtogroup udf
 * @{
 */

#ifndef UDF_UDF_VOLUME_H_
#define UDF_UDF_VOLUME_H_

#include <stdint.h>
#include <ipc/loc.h>
#include "udf_types.h"

/* Descriptor Tag Identifier (ECMA 167 3/7.2.1) */
#define UDF_TAG_PVD   0x0001  /* Primary Volume Descriptor */
#define UDF_TAG_AVDP  0x0002  /* Anchor Volume Descriptor */
#define UDF_TAG_VDP   0x0003  /* Volume Descriptor Pointer */
#define UDF_TAG_IUVD  0x0004  /* Implementation Use Volume Descriptor */
#define UDF_TAG_PD    0x0005  /* Partition Descriptor */
#define UDF_TAG_LVD   0x0006  /* Logical Volume Descriptor */
#define UDF_TAG_USD   0x0007  /* Unallocated Space Descriptor */
#define UDF_TAG_TD    0x0008  /* Terminating Descriptor */
#define UDF_TAG_LVID  0x0009  /* Logical Volume Integrity Descriptor */

/* Start address of Anchor Volume Descriptor */
#define UDF_AVDP_SECTOR  256

/* Volume Recognition Sequence params */
#define VRS_ADDR     32768
#define VRS_TYPE     0
#define VRS_VERSION  1
#define VRS_BEGIN    "BEA01"
#define VRS_END      "TEA01"
#define VRS_NSR2     "NSR02"
#define VRS_NSR3     "NSR03"
#define VRS_DEPTH    10
#define VRS_ID_LEN   5

/* Volume Structure Descriptor (ECMA 167 2/9.1) */
typedef struct udf_vrs_descriptor {
	uint8_t type;           /* Structure type */
	uint8_t identifier[5];  /* Standart identifier */
	uint8_t version;        /* Structure version */
	uint8_t data[2041];     /* Structure data */
} __attribute__((packed)) udf_vrs_descriptor_t;

/* Anchor volume descriptor (ECMA 167 3/10.2) */
typedef struct udf_anchor_volume_descriptor {
	udf_descriptor_tag_t tag;     /* Descriptor tag */
	udf_extent_t main_extent;     /* Main Volume Descriptor Sequence Extent */
	udf_extent_t reserve_extent;  /* Reserve Volume Descriptor Sequence Extent */
	uint8_t reserved[480];        /* Structure data */
} __attribute__((packed)) udf_anchor_volume_descriptor_t;

/* Common Volume Descriptor */
typedef struct udf_common_descriptor {
	udf_descriptor_tag_t tag;
	uint8_t reserved[496];
} __attribute__((packed)) udf_common_descriptor_t;

/* Volume Descriptor Pointer (ECMA 167 3/10.3) */
typedef struct udf_volume_pointer_descriptor {
	udf_descriptor_tag_t tag;
	uint32_t sequence_number;
	udf_extent_t next_sequence;
	uint8_t reserved[484];
} __attribute__((packed)) udf_volume_pointer_descriptor_t;

/* Primary Volume Descriptor (ECMA 167 3/10.1) */
typedef struct udf_primary_volume_descriptor {
	udf_descriptor_tag_t tag;
	uint32_t sequence_number;
	uint32_t primary_volume_descriptor_num;
	udf_dstring volume_id[32];
	uint16_t max_sequence_number;
	uint16_t interchange_level;
	uint16_t max_interchange_level;
	uint32_t charset_list;
	uint32_t max_charset_list;
	udf_dstring volume_set_id[128];
	udf_charspec_t descriptor_charset;
	udf_charspec_t explanatory_charset;
	udf_extent_t volume_abstract;
	udf_extent_t volume_copyright_notice;
	udf_regid_t application_id;
	udf_timestamp_t recording_data_and_time;
	udf_regid_t implementation_id;
	uint8_t implementation_use[64];
	uint32_t predecessor_vds_location;
	uint16_t flags;
	uint8_t reserved[22];
} __attribute__((packed)) udf_primary_volume_descriptor_t;

/* Partition Descriptor (ECMA 167 3/10.5) */
typedef struct udf_partition_descriptor {
	udf_descriptor_tag_t tag;
	uint32_t sequence_number;
	uint16_t flags;
	uint16_t number;
	udf_regid_t contents;
	uint8_t contents_use[128];
	uint32_t access_type;
	uint32_t starting_location;
	uint32_t length;
	udf_regid_t implementation_id;
	uint8_t implementation_use[128];
	uint8_t reserved[156];
} __attribute__((packed)) udf_partition_descriptor_t;

/* Logical Volume Descriptor (ECMA 167 3/10.6) */
typedef struct udf_logical_volume_descriptor {
	udf_descriptor_tag_t tag;
	uint32_t sequence_number;
	udf_charspec_t charset;
	udf_dstring logical_volume_id[128];
	uint32_t logical_block_size;
	udf_regid_t domain_id;
	uint8_t logical_volume_conents_use[16];
	uint32_t map_table_length;
	uint32_t number_of_partitions_maps;
	udf_regid_t implementation_id;
	uint8_t implementation_use[128];
	udf_extent_t integrity_sequence_extent;
	uint8_t partition_map[0];
} __attribute__((packed)) udf_logical_volume_descriptor_t;

typedef struct udf_volume_descriptor {
	union {
		udf_common_descriptor_t common;
		udf_terminating_descriptor_t terminating;
		udf_volume_pointer_descriptor_t pointer;
		udf_partition_descriptor_t partition;
		udf_logical_volume_descriptor_t logical;
		udf_unallocated_space_descriptor_t unallocated;
		udf_primary_volume_descriptor_t volume;
	};
} __attribute__((packed)) udf_volume_descriptor_t;

typedef struct udf_general_type {
	uint8_t partition_map_type;
	uint8_t partition_map_length;
} __attribute__((packed)) udf_general_type_t;

typedef struct udf_type1_partition_map {
	uint8_t partition_map_type;
	uint8_t partition_map_length;
	uint16_t volume_sequence_number;
	uint16_t partition_number;
} __attribute__((packed)) udf_type1_partition_map_t;

typedef struct udf_type2_partition_map {
	uint8_t partition_map_type;
	uint8_t partition_map_length;
	uint8_t reserved1[2];
	udf_regid_t partition_ident;
	uint16_t volume_sequence_number;
	uint16_t partition_number;
} __attribute__((packed)) udf_type2_partition_map_t;

/* Metadata Partition Map (UDF 2.4.0 2.2.10) */
typedef struct udf_metadata_partition_map {
	uint8_t partition_map_type;
	uint8_t partition_map_length;
	uint8_t reserved1[2];
	udf_regid_t partition_ident;
	uint16_t volume_sequence_number;
	uint16_t partition_number;
	uint32_t metadata_fileloc;
	uint32_t metadata_mirror_fileloc;
	uint32_t metadata_bitmap_fileloc;
	uint32_t alloc_unit_size;
	uint16_t align_unit_size;
	uint8_t flags;
	uint8_t reserved2[5];
} __attribute__((packed)) udf_metadata_partition_map_t;

/* Partition Header Descriptor (ECMA 167 4/14.3) */
typedef struct udf_partition_header_descriptor {
	udf_short_ad_t unallocated_space_table;
	udf_short_ad_t unallocated_space_bitmap;
	udf_short_ad_t partition_integrity_table;
	udf_short_ad_t freed_space_table;
	udf_short_ad_t freed_space_bitmap;
	uint8_t reserved[88];
} __attribute__((packed)) udf_partition_header_descriptor_t;

extern errno_t udf_volume_recongnition(service_id_t);
extern errno_t udf_get_anchor_volume_descriptor(service_id_t,
    udf_anchor_volume_descriptor_t *);
extern errno_t udf_read_volume_descriptor_sequence(service_id_t, udf_extent_t);
extern fs_index_t udf_long_ad_to_pos(udf_instance_t *, udf_long_ad_t *);

#endif

/**
 * @}
 */
