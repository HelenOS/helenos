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

#ifndef LIBEXT4_DIRECTORY_H_
#define LIBEXT4_DIRECTORY_H_

#include "ext4/types.h"

extern uint32_t ext4_directory_entry_ll_get_inode(ext4_directory_entry_ll_t *);
extern void ext4_directory_entry_ll_set_inode(ext4_directory_entry_ll_t *,
    uint32_t);
extern uint16_t ext4_directory_entry_ll_get_entry_length(
    ext4_directory_entry_ll_t *);
extern void ext4_directory_entry_ll_set_entry_length(ext4_directory_entry_ll_t *,
    uint16_t);
extern uint16_t ext4_directory_entry_ll_get_name_length(ext4_superblock_t *,
    ext4_directory_entry_ll_t *);
extern void ext4_directory_entry_ll_set_name_length(ext4_superblock_t *,
    ext4_directory_entry_ll_t *, uint16_t);
extern uint8_t ext4_directory_entry_ll_get_inode_type(ext4_superblock_t *,
    ext4_directory_entry_ll_t *);
extern void ext4_directory_entry_ll_set_inode_type(ext4_superblock_t *,
    ext4_directory_entry_ll_t *, uint8_t);

extern errno_t ext4_directory_iterator_init(ext4_directory_iterator_t *,
    ext4_inode_ref_t *, aoff64_t);
extern errno_t ext4_directory_iterator_next(ext4_directory_iterator_t *);
extern errno_t ext4_directory_iterator_fini(ext4_directory_iterator_t *);

extern void ext4_directory_write_entry(ext4_superblock_t *,
    ext4_directory_entry_ll_t *, uint16_t, ext4_inode_ref_t *,
    const char *, size_t);
extern errno_t ext4_directory_add_entry(ext4_inode_ref_t *, const char *,
    ext4_inode_ref_t *);
extern errno_t ext4_directory_find_entry(ext4_directory_search_result_t *,
    ext4_inode_ref_t *, const char *);
extern errno_t ext4_directory_remove_entry(ext4_inode_ref_t *, const char *);

extern errno_t ext4_directory_try_insert_entry(ext4_superblock_t *, block_t *,
    ext4_inode_ref_t *, const char *, uint32_t);

extern errno_t ext4_directory_find_in_block(block_t *, ext4_superblock_t *, size_t,
    const char *, ext4_directory_entry_ll_t **);

extern errno_t ext4_directory_destroy_result(ext4_directory_search_result_t *);

#endif

/**
 * @}
 */
