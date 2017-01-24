/*
 * Copyright (c) 2011 Martin Sucha
 * Copyright (c) 2012 Frantisek Princ
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

#ifndef LIBEXT4_INODE_H_
#define LIBEXT4_INODE_H_

#include <block.h>
#include <stdint.h>
#include "ext4/types.h"

extern uint32_t ext4_inode_get_mode(ext4_superblock_t *, ext4_inode_t *);
extern void ext4_inode_set_mode(ext4_superblock_t *, ext4_inode_t *, uint32_t);
extern uint32_t ext4_inode_get_uid(ext4_inode_t *);
extern void ext4_inode_set_uid(ext4_inode_t *, uint32_t);
extern uint64_t ext4_inode_get_size(ext4_superblock_t *, ext4_inode_t *);
extern void ext4_inode_set_size(ext4_inode_t *, uint64_t);
extern uint32_t ext4_inode_get_access_time(ext4_inode_t *);
extern void ext4_inode_set_access_time(ext4_inode_t *, uint32_t);
extern uint32_t ext4_inode_get_change_inode_time(ext4_inode_t *);
extern void ext4_inode_set_change_inode_time(ext4_inode_t *, uint32_t);
extern uint32_t ext4_inode_get_modification_time(ext4_inode_t *);
extern void ext4_inode_set_modification_time(ext4_inode_t *, uint32_t);
extern uint32_t ext4_inode_get_deletion_time(ext4_inode_t *);
extern void ext4_inode_set_deletion_time(ext4_inode_t *, uint32_t);
extern uint32_t ext4_inode_get_gid(ext4_inode_t *);
extern void ext4_inode_set_gid(ext4_inode_t *, uint32_t);
extern uint16_t ext4_inode_get_links_count(ext4_inode_t *);
extern void ext4_inode_set_links_count(ext4_inode_t *, uint16_t);
extern uint64_t ext4_inode_get_blocks_count(ext4_superblock_t *,
    ext4_inode_t *);
extern errno_t ext4_inode_set_blocks_count(ext4_superblock_t *, ext4_inode_t *,
    uint64_t);
extern uint32_t ext4_inode_get_flags(ext4_inode_t *);
extern void ext4_inode_set_flags(ext4_inode_t *, uint32_t);
extern uint32_t ext4_inode_get_generation(ext4_inode_t *);
extern void ext4_inode_set_generation(ext4_inode_t *, uint32_t);
extern uint64_t ext4_inode_get_file_acl(ext4_inode_t *, ext4_superblock_t *);
extern void ext4_inode_set_file_acl(ext4_inode_t *, ext4_superblock_t *,
    uint64_t);

extern uint32_t ext4_inode_get_direct_block(ext4_inode_t *, uint32_t);
extern void ext4_inode_set_direct_block(ext4_inode_t *, uint32_t, uint32_t);
extern uint32_t ext4_inode_get_indirect_block(ext4_inode_t *, uint32_t);
extern void ext4_inode_set_indirect_block(ext4_inode_t *, uint32_t, uint32_t);
extern ext4_extent_header_t *ext4_inode_get_extent_header(ext4_inode_t *);
extern bool ext4_inode_is_type(ext4_superblock_t *, ext4_inode_t *, uint32_t);
extern bool ext4_inode_has_flag(ext4_inode_t *, uint32_t);
extern void ext4_inode_clear_flag(ext4_inode_t *, uint32_t);
extern void ext4_inode_set_flag(ext4_inode_t *, uint32_t);
extern bool ext4_inode_can_truncate(ext4_superblock_t *, ext4_inode_t *);

#endif

/**
 * @}
 */
