/*
 * Copyright (c) 2011 Maurizio Lombardi
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

#ifndef _MFS_SUPER_H_
#define _MFS_SUPER_H_

#include "mfs_const.h"
#include "../../vfs/vfs.h"

#define MFS_MAGIC_V1		0x137F
#define MFS_MAGIC_V1R		0x7F13

#define MFS_MAGIC_V2		0x2468
#define MFS_MAGIC_V2R		0x6824

#define MFS_MAGIC_V3		0x4D5A
#define MFS_MAGIC_V3R		0x5A4D

struct mfs_superblock {
	/*Total number of inodes on the device*/
	uint32_t	s_ninodes;
	/*Device size expressed as number of zones (unused)*/
	uint16_t	s_nzones;
	/*Number of inode bitmap blocks*/
	int16_t		s_ibmap_blocks;
	/*Number of zone bitmap blocks*/
	int16_t		s_zbmap_blocks;
	/*First data zone on device*/
	uint16_t	s_first_data_zone;
	/*Base 2 logarithm of the zone to block ratio*/
	int16_t		s_log2_zone_size;
	int16_t		s_pad;
	/*Maximum file size expressed in bytes*/
	int32_t		s_max_file_size;
	/*Total number of zones on the device*/
	uint32_t	s_total_zones;
	/*Magic number used to recognize MinixFS and to detect on-disk endianness*/
	int16_t		s_magic;

	/* The following fields are valid only for MinixFS V3 */

	int16_t		s_pad2;
	/*Filesystem block size expressed in bytes*/
	uint16_t	s_block_size;
	/*Filesystem disk format version*/
	int8_t		s_disk_version;
} __attribute__ ((packed));

typedef enum {
	MFS_VERSION_V1 = 1,
	MFS_VERSION_V2,
	MFS_VERSION_V3
} mfs_version_t;

void mfs_mounted(ipc_callid_t rid, ipc_call_t *request);

#endif

/**
 * @}
 */ 
