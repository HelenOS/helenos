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
/**
 * @file udf_types.h
 * @brief UDF common types
 */

#ifndef UDF_UDF_TYPES_H_
#define UDF_UDF_TYPES_H_

#include <stdint.h>
#include <stdbool.h>
#include <byteorder.h>

#define GET_LE16(x)  x = uint16_t_le2host(x)
#define GET_LE32(x)  x = uint32_t_le2host(x)

#define FLE16(x)  uint16_t_le2host(x)
#define FLE32(x)  uint32_t_le2host(x)
#define FLE64(x)  uint64_t_le2host(x)

#define ALL_UP(n, b)  ((n) / (b) + ((n) % (b) != 0))

#define EXT_LENGTH(x)  (x & 0x3FFFFFFF)

typedef uint8_t udf_dstring;

/* Timestamp descriptor (ECMA 167 1/7.3) */
typedef struct udf_timestamp {
	uint16_t type_and_tz;
	uint16_t year;
	uint8_t month;
	uint8_t day;
	uint8_t hour;
	uint8_t min;
	uint8_t sec;
	uint8_t centisec;
	uint8_t h_of_mcsec;
	uint8_t mcsec;
} __attribute__((packed)) udf_timestamp_t;

/* Universal descriptor tag (ECMA 167 3/7.2) */
typedef struct udf_descriptor_tag {
	uint16_t id;
	uint16_t version;
	uint8_t checksum;
	uint8_t reserved;
	uint16_t serial;
	uint16_t descriptor_crc;
	uint16_t descriptor_crc_length;
	uint32_t location;
} __attribute__((packed)) udf_descriptor_tag_t;

/* Entity descriptor (ECMA 167 1/7.4) */
typedef struct udf_regid {
	uint8_t flags;
	uint8_t id[23];
	uint8_t id_suffix[8];
} __attribute__((packed)) udf_regid_t;

/* Character set specification (ECMA 167 1/7.2.1) */
typedef struct udf_charspec {
	uint8_t type;
	uint8_t info[63];
} __attribute__((packed)) udf_charspec_t;

/* Recorded address (ECMA 167 4/7.1) */
typedef struct udf_lb_addr {
	uint32_t lblock_num;     /* Relative to start of the partition (from 0) */
	uint16_t partition_num;  /* Relative to logical volume or not? */
} __attribute__((packed)) udf_lb_addr_t;

/* Long Allocation Descriptor (ECMA 167 4/14.14.2) */
typedef struct udf_long_ad {
	uint32_t length;
	udf_lb_addr_t location;
	uint8_t implementation_use[6];
} __attribute__((packed)) udf_long_ad_t;

/* Short Allocation Descriptor (ECMA 167 4/14.14.1) */
typedef struct udf_short_ad {
	uint32_t length;
	uint32_t position;
} __attribute__((packed)) udf_short_ad_t;

/* Extended Allocation Descriptor (ECMA 167 4/14.14.3) */
typedef struct udf_ext_ad {
	uint32_t extent_length;
	uint32_t recorded_length;
	uint32_t info_length;
	udf_lb_addr_t extent_location;
	uint8_t implementation_use[2];
} __attribute__((packed)) udf_ext_ad_t;

/* Extent descriptor (ECMA 167 3/7.1) */
typedef struct udf_extent {
	uint32_t length;    /* Bytes */
	uint32_t location;  /* Sectors */
} __attribute__((packed)) udf_extent_t;

/* Terminating Descriptor (ECMA 167 3/10.9) */
typedef struct udf_terminating_descriptor {
	udf_descriptor_tag_t tag;
	uint8_t reserved[496];
} __attribute__((packed)) udf_terminating_descriptor_t;

/* Unallocated Space Descriptor (ECMA 167 3/10.8) */
typedef struct udf_unallocated_space_descriptor {
	udf_descriptor_tag_t tag;
	uint32_t sequence_number;
	uint32_t allocation_descriptors_num;
	udf_extent_t allocation_descriptors[0];
} __attribute__((packed)) udf_unallocated_space_descriptor_t;

#endif

/**
 * @}
 */
