/*
 * Copyright (c) 2008 Jakub Jermar
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

#ifndef FAT_FAT_FAT_H_
#define FAT_FAT_FAT_H_

#include "../../vfs/vfs.h"
#include <stdint.h>

#define FAT1		0

#define FAT_CLST_RES0	0x0000
#define FAT_CLST_RES1	0x0001
#define FAT_CLST_FIRST	0x0002
#define FAT_CLST_BAD	0xfff7
#define FAT_CLST_LAST1	0xfff8
#define FAT_CLST_LAST8  0xffff

/* internally used to mark root directory's parent */
#define FAT_CLST_ROOTPAR	FAT_CLST_RES0
/* internally used to mark root directory */
#define FAT_CLST_ROOT		FAT_CLST_RES1


/* forward declarations */
struct block;
struct fat_node;

typedef uint16_t fat_cluster_t;

#define fat_block_get(np, off) \
    _fat_block_get((np)->idx->dev_handle, (np)->firstc, (off))
    
extern struct block *_fat_block_get(dev_handle_t, fat_cluster_t, off_t);
extern uint16_t _fat_blcks_get(dev_handle_t, fat_cluster_t, fat_cluster_t *);
extern uint16_t fat_bps_get(dev_handle_t);
  
extern void fat_append_clusters(struct fat_node *, fat_cluster_t);
extern int fat_alloc_clusters(dev_handle_t, unsigned, fat_cluster_t *,
    fat_cluster_t *);
extern void fat_alloc_shadow_clusters(dev_handle_t, fat_cluster_t *, unsigned);
extern void fat_mark_cluster(dev_handle_t, unsigned, fat_cluster_t,
    fat_cluster_t);
extern void fat_fill_gap(struct fat_node *, fat_cluster_t, off_t);

#endif

/**
 * @}
 */
