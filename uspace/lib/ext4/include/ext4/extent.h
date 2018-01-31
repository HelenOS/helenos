/*
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

#ifndef LIBEXT4_EXTENT_H_
#define LIBEXT4_EXTENT_H_

#include "ext4/types.h"

extern uint32_t ext4_extent_get_first_block(ext4_extent_t *);
extern void ext4_extent_set_first_block(ext4_extent_t *, uint32_t);
extern uint16_t ext4_extent_get_block_count(ext4_extent_t *);
extern void ext4_extent_set_block_count(ext4_extent_t *, uint16_t);
extern uint64_t ext4_extent_get_start(ext4_extent_t *);
extern void ext4_extent_set_start(ext4_extent_t *, uint64_t);

extern uint32_t ext4_extent_index_get_first_block(ext4_extent_index_t *);
extern void ext4_extent_index_set_first_block(ext4_extent_index_t *, uint32_t);
extern uint64_t ext4_extent_index_get_leaf(ext4_extent_index_t *);
extern void ext4_extent_index_set_leaf(ext4_extent_index_t *, uint64_t);

extern uint16_t ext4_extent_header_get_magic(ext4_extent_header_t *);
extern void ext4_extent_header_set_magic(ext4_extent_header_t *, uint16_t);
extern uint16_t ext4_extent_header_get_entries_count(ext4_extent_header_t *);
extern void ext4_extent_header_set_entries_count(ext4_extent_header_t *,
    uint16_t);
extern uint16_t ext4_extent_header_get_max_entries_count(ext4_extent_header_t *);
extern void ext4_extent_header_set_max_entries_count(ext4_extent_header_t *,
    uint16_t);
extern uint16_t ext4_extent_header_get_depth(ext4_extent_header_t *);
extern void ext4_extent_header_set_depth(ext4_extent_header_t *, uint16_t);
extern uint32_t ext4_extent_header_get_generation(ext4_extent_header_t *);
extern void ext4_extent_header_set_generation(ext4_extent_header_t *, uint32_t);

extern errno_t ext4_extent_find_block(ext4_inode_ref_t *, uint32_t, uint32_t *);
extern errno_t ext4_extent_release_blocks_from(ext4_inode_ref_t *, uint32_t);

extern errno_t ext4_extent_append_block(ext4_inode_ref_t *, uint32_t *, uint32_t *,
    bool);

#endif

/**
 * @}
 */
