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

/* Super block table.  The root file system and every mounted file system
 * has an entry here.  The entry holds information about the sizes of the bit
 * maps and inodes.  The s_ninodes field gives the number of inodes available
 * for files and directories, including the root directory.  Inode 0 is 
 * on the disk, but not used.  Thus s_ninodes = 4 means that 5 bits will be
 * used in the bit map, bit 0, which is always 1 and not used, and bits 1-4
 * for files and directories.  The disk layout is:
 *
 *    Item        # blocks
 *    boot block      1
 *    super block     1    (offset 1kB)
 *    inode map     s_imap_blocks
 *    zone map      s_zmap_blocks
 *    inodes        (s_ninodes + 'inodes per block' - 1)/'inodes per block'
 *    unused        whatever is needed to fill out the current zone
 *    data zones    (s_zones - s_firstdatazone) << s_log_zone_size
 *
 */

struct mfs_v3_superblock {
	uint32_t	s_ninodes;		/*# usable inodes on the device*/
	uint16_t	s_nzones;		/*total device size including bitmaps etc*/
	int16_t		s_imap_blocks;		/*# of blocks used by inode bitmap*/
	int16_t		s_zmap_blocks;		/*# of blocks used by zone bitmap*/
	uint16_t	s_firstdatazone_old;	/*# of first data zone (small)*/
	int16_t		s_log_zone_size;	/*log2 of blocks / zone*/
	int16_t		s_pad;			/*try to avoid compiler dependent padding*/
	int32_t		s_max_size;		/*maximum file size*/
	uint32_t	s_zones;		/*number of zones*/
	int16_t		s_magic;		/*magic number to recognize superblocks*/

	/* The following items are valid on disk only for V3 and above */
	
	int16_t		s_pad2;
	uint16_t	s_block_size;		/*block size in bytes*/
	int8_t		s_disk_version;		/*filesystem format sub-version*/
} __attribute__ ((packed));

#endif

/**
 * @}
 */ 
