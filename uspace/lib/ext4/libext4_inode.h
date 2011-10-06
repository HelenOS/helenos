/*
 * Copyright (c) 2011 Frantisek Princ
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

/** @addtogroup libext4
 * @{
 */ 

#ifndef LIBEXT4_LIBEXT4_INODE_H_
#define LIBEXT4_LIBEXT4_INODE_H_

#include <libblock.h>
#include <sys/types.h>
#include "libext4_superblock.h"


#define EXT4_INODE_DIRECT_BLOCK_COUNT		12
#define EXT4_INODE_INDIRECT_BLOCK 			EXT4_INODE_DIRECT_BLOCK_COUNT
#define EXT4_INODE_DOUBLE_INDIRECT_BLOCK	(EXT4_INODE_INDIRECT_BLOCK + 1)
#define EXT4_INODE_TRIPPLE_INDIRECT_BLOCK	(EXT4_INODE_DOUBLE_INDIRECT_BLOCK + 1)
#define EXT4_INODE_BLOCKS					(EXT4_INODE_TRIPPLE_INDIRECT_BLOCK + 1)
#define EXT4_INODE_INDIRECT_BLOCK_COUNT		(EXT4_INODE_BLOCKS - EXT4_INODE_DIRECT_BLOCK_COUNT)

/*
 * Structure of an inode on the disk
 */
typedef struct ext4_inode {
	uint16_t mode; // File mode
	uint16_t uid; // Low 16 bits of owner uid
	uint32_t size_lo; // Size in bytes
	uint32_t acess_time; // Access time
	uint32_t change_inode_time; // Inode change time
	uint32_t modification_time; // Modification time
	uint32_t deletion_time; // Deletion time
	uint16_t gid; // Low 16 bits of group id
	uint16_t links_count; // Links count
	uint32_t blocks_count_lo; // Blocks count
	uint32_t flags; // File flags
	uint32_t unused_osd1; // OS dependent - not used in HelenOS
    uint32_t blocks[EXT4_INODE_BLOCKS]; // Pointers to blocks
    uint32_t generation; // File version (for NFS)
    uint32_t file_acl_lo; // File ACL
    uint32_t size_hi;
    uint32_t obso_faddr; // Obsoleted fragment address
    union {
    	struct {
    		uint16_t blocks_high; /* were l_i_reserved1 */
    		uint16_t file_acl_high;
    		uint16_t uid_high;   /* these 2 fields */
    		uint16_t gid_high;   /* were reserved2[0] */
    		uint32_t reserved2;
    	} linux2;
    	struct {
    		uint16_t reserved1;  /* Obsoleted fragment number/size which are removed in ext4 */
    		uint16_t mode_high;
    		uint16_t uid_high;
    		uint16_t gid_high;
    		uint32_t author;
    	} hurd2;
    	struct {
    		uint16_t reserved1;  /* Obsoleted fragment number/size which are removed in ext4 */
    		uint16_t file_acl_high;
    		uint32_t reserved2[2];
    	} masix2;
    } __attribute__ ((packed)) osd2;

    uint16_t extra_isize;
    uint16_t pad1;
    uint32_t ctime_extra; // Extra change time (nsec << 2 | epoch)
    uint32_t mtime_extra; // Extra Modification time (nsec << 2 | epoch)
    uint32_t atime_extra; // Extra Access time (nsec << 2 | epoch)
    uint32_t crtime; // File creation time
    uint32_t crtime_extra; // Extra file creation time (nsec << 2 | epoch)
    uint32_t version_hi;   // High 32 bits for 64-bit version
} __attribute__ ((packed)) ext4_inode_t;

#define EXT4_INODE_MODE_FIFO		0x1000
#define EXT4_INODE_MODE_CHARDEV		0x2000
#define EXT4_INODE_MODE_DIRECTORY	0x4000
#define EXT4_INODE_MODE_BLOCKDEV	0x6000
#define EXT4_INODE_MODE_FILE		0x8000
#define EXT4_INODE_MODE_SOFTLINK	0xA000
#define EXT4_INODE_MODE_SOCKET		0xC000
#define EXT4_INODE_MODE_TYPE_MASK	0xF000

#define EXT4_INODE_ROOT_INDEX	2

typedef struct ext4_inode_ref {
	block_t *block; // Reference to a block containing this inode
	ext4_inode_t *inode;
	uint32_t index; // Index number of this inode
} ext4_inode_ref_t;


extern uint32_t ext4_inode_get_mode(ext4_superblock_t *, ext4_inode_t *);
extern bool ext4_inode_is_type(ext4_superblock_t *, ext4_inode_t *, uint32_t);
/*
extern uint32_t ext4_inode_get_uid(ext4_inode_t *);
*/
extern uint64_t ext4_inode_get_size(ext4_superblock_t *, ext4_inode_t *);
/*
extern uint32_t ext4_inode_get_access_time(ext4_inode_t *);
extern uint32_t ext4_inode_get_change_inode_time(ext4_inode_t *);
extern uint32_t ext4_inode_get_modification_time(ext4_inode_t *);
extern uint32_t ext4_inode_get_deletion_time(ext4_inode_t *);
extern uint32_t ext4_inode_get_gid(ext4_inode_t *);
*/
extern uint16_t ext4_inode_get_links_count(ext4_inode_t *);
/*
extern uint64_t ext4_inode_get_blocks_count(ext4_inode_t *);
extern uint32_t ext4_inode_get_flags(ext4_inode_t *);
*/

uint32_t ext4_inode_get_direct_block(ext4_inode_t *, uint8_t);
uint32_t ext4_inode_get_indirect_block(ext4_inode_t *, uint8_t);

/*
uint32_t blocks[EXT4_INODE_BLOCKS]; // Pointers to blocks
uint32_t generation;
uint32_t file_acl_lo; // File ACL
uint16_t extra_isize;
uint32_t ctime_extra; // Extra change time (nsec << 2 | epoch)
uint32_t mtime_extra; // Extra Modification time (nsec << 2 | epoch)
uint32_t atime_extra; // Extra Access time (nsec << 2 | epoch)
uint32_t crtime; // File creation time
uint32_t crtime_extra; // Extra file creation time (nsec << 2 | epoch)
uint32_t version_hi;   // High 32 bits for 64-bit version
*/

#endif

/**
 * @}
 */
