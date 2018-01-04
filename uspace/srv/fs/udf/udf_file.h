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

/** @addtogroup fs
 * @{
 */

#ifndef UDF_FILE_H_
#define UDF_FILE_H_

#include <stddef.h>
#include <stdint.h>
#include <ipc/loc.h>
#include <block.h>
#include "udf_types.h"

#define UDF_TAG_FILESET  256
#define UDF_TAG_FILEID   257

#define UDF_ICB_TERMINAL   260
#define UDF_FILE_ENTRY     261
#define UDF_UASPACE_ENTRY  263
#define UDF_SPACE_BITMAP   264
#define UDF_EFILE_ENTRY    266

#define UDF_FE_OFFSET   176
#define UDF_EFE_OFFSET  216
#define UDF_SB_OFFSET   24

/* ECMA 4/14.11 */
#define UDF_UASE_OFFSET  40

/* ECMA 167 4/14.6.8 */
#define UDF_ICBFLAG_MASK  7

/* ECMA 167 4/17 */
#define UDF_ICBTYPE_UASE  1
#define UDF_ICBTYPE_DIR   4

/* ECMA 167 4/14.6.8 */
#define UDF_SHORT_AD     0
#define UDF_LONG_AD      1
#define UDF_EXTENDED_AD  2

/* File in allocation descriptor */
#define UDF_DATA_AD  3

/* File Set Descriptor (ECMA 167 4/14.1) */
typedef struct udf_fileset_descriptor {
	udf_descriptor_tag_t tag;
	udf_timestamp_t recording_date_and_time;
	uint16_t interchange_level;
	uint16_t max_interchange_level;
	uint32_t charset_list;
	uint32_t max_charset_list;
	uint32_t fileset_number;
	uint32_t fileset_descriptor_number;
	udf_charspec_t logical_volume_id_charset;
	udf_dstring logical_volume_id[128];
	udf_charspec_t fileset_charset;
	udf_dstring fileset_id[32];
	udf_dstring copyright_file_id[32];
	udf_dstring abstract_file_id[32];
	udf_long_ad_t root_dir_icb;
	udf_regid_t domain_id;
	udf_long_ad_t next_extent;
	udf_long_ad_t system_stream_dir_icb;
	uint8_t reserved[32];
} __attribute__((packed)) udf_fileset_descriptor_t;

/* File identifier descriptor format (ECMA 167 4/14.4) */
typedef struct udf_file_identifier_descriptor {
	udf_descriptor_tag_t tag;
	uint16_t file_version_number;
	uint8_t file_characteristics;
	uint8_t lenght_file_id;
	udf_long_ad_t icb;
	uint16_t lenght_iu;
	uint8_t implementation_use[0];
	udf_dstring file_id[0];
}__attribute__((packed)) udf_file_identifier_descriptor_t;

/* ICB tag - Information Control Block  (ECMA 167 4/14.6) */
typedef struct udf_icbtag {
	uint32_t prior_recorder_nimber;
	uint16_t strategy_type;
	uint8_t strategy_parameter[2];
	uint16_t max_number;
	uint8_t reserved[1];
	uint8_t file_type;
	udf_lb_addr_t parent_icb;
	uint16_t flags;
} __attribute__((packed)) udf_icbtag_t;

/* File Entry format (ECMA 167 4/14.9) */
typedef struct udf_file_entry_descriptor {
	udf_descriptor_tag_t tag;
	udf_icbtag_t icbtag;
	uint32_t uid;
	uint32_t gid;
	uint32_t permission;
	uint16_t file_link_count;
	uint8_t record_format;
	uint8_t record_display_attributes;
	uint32_t record_lenght;
	uint64_t info_lenght;
	uint64_t lblocks_recorded;
	udf_timestamp_t access_data_and_time;
	udf_timestamp_t mod_data_and_time;
	udf_timestamp_t attribute_data_and_time;
	uint32_t checkpoint;
	udf_long_ad_t extended_attribute_icb;
	udf_regid_t implementation_id;
	uint64_t unique_id;
	uint32_t ea_lenght;
	uint32_t ad_lenght;
	uint8_t extended_attributes [0];
	uint8_t allocation_descriptors[0];
} __attribute__((packed)) udf_file_entry_descriptor_t;

/* Extended File Entry format (ECMA 167 4/14.17) */
typedef struct udf_extended_file_entry_descriptor {
	udf_descriptor_tag_t tag;
	udf_icbtag_t icbtag;
	uint32_t uid;
	uint32_t gid;
	uint32_t permission;
	uint16_t file_link_count;
	uint8_t record_format;
	uint8_t record_display_attributes;
	uint32_t record_lenght;
	uint64_t info_lenght;
	uint64_t object_size;
	uint64_t lblocks_recorded;
	udf_timestamp_t access_data_and_time;
	udf_timestamp_t mod_data_and_time;
	udf_timestamp_t creation_data_and_time;
	udf_timestamp_t attribute_data_and_time;
	uint32_t checkpoint;
	udf_long_ad_t extended_attribute_icb;
	udf_regid_t implementation_id;
	uint64_t unique_id;
	uint32_t ea_lenght;
	uint32_t ad_lenght;
	uint8_t extended_attributes [0];
	uint8_t allocation_descriptors[0];
} __attribute__((packed)) udf_extended_file_entry_descriptor_t;

/* Terminal Entry Descriptor (ECMA 167 4/14.8) */
typedef struct terminal_entry_descriptor {
	udf_descriptor_tag_t tag;
	udf_icbtag_t icbtag;
} __attribute__((packed)) udf_terminal_entry_descriptor;

/* Unallocated Space Entry format (ECMA 167 4/14.11)*/
typedef struct udf_unallocated_space_entry_descriptor {
	udf_descriptor_tag_t tag;
	udf_icbtag_t icbtag;
	uint32_t ad_lenght;
	uint8_t allocation_descriptors[0];
}__attribute__((packed)) udf_unallocated_space_entry_descriptor_t;

/* Space Bitmap Descriptor format (ECMA 167 4/14.12) */
typedef struct udf_space_bitmap_descriptor {
	udf_descriptor_tag_t tag;
	uint32_t bits_number;
	uint32_t byts_number;
	uint8_t bitmap[0];
}__attribute__((packed)) udf_space_bitmap_descriptor_t;

extern errno_t udf_node_get_core(udf_node_t *);
extern errno_t udf_read_icb(udf_node_t *);
extern errno_t udf_read_allocation_sequence(udf_node_t *, uint8_t *, uint16_t,
    uint32_t, uint32_t);
extern errno_t udf_read_file(size_t *, ipc_callid_t, udf_node_t *, aoff64_t,
    size_t);
extern errno_t udf_get_fid(udf_file_identifier_descriptor_t **, block_t **,
    udf_node_t *, aoff64_t);
extern errno_t udf_get_fid_in_allocator(udf_file_identifier_descriptor_t **,
    block_t **, udf_node_t *, aoff64_t);
extern errno_t udf_get_fid_in_sector(udf_file_identifier_descriptor_t **,
    block_t **, udf_node_t *, aoff64_t, size_t *, void **, size_t *);

#endif /* UDF_FILE_H_ */

/**
 * @}
 */
