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

#include <sys/types.h>

// TODO better constant definition !!!
#define	EXT4_N_BLOCKS	15

/*
 * Structure of an inode on the disk
 */
typedef struct ext4_inode {
	uint16_t i_mode; // File mode
	uint16_t i_uid; // Low 16 bits of owner uid
	uint32_t i_size_lo; // Size in bytes
	uint32_t i_atime; // Access time
	uint32_t i_ctime; // Inode change time
	uint32_t i_mtime; // Modification time
	uint32_t i_dtime; // Deletion time
	uint16_t i_gid; // Low 16 bits of group id
	uint16_t i_links_count; // Links count
	uint32_t i_blocks_lo; // Blocks count
	uint32_t i_flags; // File flags

	/*
        union {
                struct {
                        __le32  l_i_version;
                } linux1;
                struct {
                        __u32  h_i_translator;
                } hurd1;
                struct {
                        __u32  m_i_reserved1;
                } masix1;
        } osd1;
	*/
	uint32_t unused_osd1; // OS dependent - not used in HelenOS

    uint32_t i_block[EXT4_N_BLOCKS]; // Pointers to blocks
    uint32_t i_generation; // File version (for NFS)
    uint32_t i_file_acl_lo; // File ACL
    uint32_t i_size_high;
    uint32_t i_obso_faddr; // Obsoleted fragment address

    /*
        union {
                struct {
                        __le16  l_i_blocks_high;
                        __le16  l_i_file_acl_high;
                        __le16  l_i_uid_high;
                        __le16  l_i_gid_high;
                        __u32   l_i_reserved2;
                } linux2;
                struct {
                        __le16  h_i_reserved1;
                        __u16   h_i_mode_high;
                        __u16   h_i_uid_high;
                        __u16   h_i_gid_high;
                        __u32   h_i_author;
                } hurd2;
                struct {
                        __le16  h_i_reserved1;
                        __le16  m_i_file_acl_high;
                        __u32   m_i_reserved2[2];
                } masix2;
        } osd2;
        */

        uint32_t unused_osd2[3]; // OS dependent - not used in HelenOS
        uint16_t i_extra_isize;
        uint16_t i_pad1;
        uint32_t  i_ctime_extra; // Extra change time (nsec << 2 | epoch)
        uint32_t i_mtime_extra; // Extra Modification time (nsec << 2 | epoch)
        uint32_t i_atime_extra; // Extra Access time (nsec << 2 | epoch)
        uint32_t i_crtime; // File creation time
        uint32_t i_crtime_extra; // Extra file creation time (nsec << 2 | epoch)
        uint32_t i_version_hi;   // High 32 bits for 64-bit version
} __attribute__ ((packed)) ext4_inode_t;



#endif

/**
 * @}
 */
