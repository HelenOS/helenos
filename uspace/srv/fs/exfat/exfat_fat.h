/*
 * Copyright (c) 2008 Jakub Jermar
 * Copyright (c) 2011 Oleg Romanenko
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

#ifndef EXFAT_FAT_FAT_H_
#define EXFAT_FAT_FAT_H_

#include "../../vfs/vfs.h"
#include <stdint.h>
#include <libblock.h>

#define EXFAT_ROOT_IDX		0
#define EXFAT_BITMAP_IDX	1
#define EXFAT_UCTABLE_IDX	2

#define EXFAT_ROOT_PAR	0

#define EXFAT_CLST_FIRST	0x00000002
#define EXFAT_CLST_LAST		0xfffffff6
#define EXFAT_CLST_BAD		0xfffffff7
#define EXFAT_CLST_EOF		0xffffffff

/* forward declarations */
struct block;
struct exfat_node;
struct exfat_bs;

typedef uint32_t exfat_cluster_t;


#define fat_clusters_get(numc, bs, dh, fc) \
    fat_cluster_walk((bs), (dh), (fc), NULL, (numc), (uint32_t) -1)
extern int fat_cluster_walk(struct exfat_bs *bs, devmap_handle_t devmap_handle, 
    exfat_cluster_t firstc, exfat_cluster_t *lastc, uint32_t *numc,
    uint32_t max_clusters);
extern int exfat_block_get(block_t **block, struct exfat_bs *bs,
    struct exfat_node *nodep, aoff64_t bn, int flags);
extern int exfat_block_get_by_clst(block_t **block, struct exfat_bs *bs, 
    devmap_handle_t devmap_handle, bool fragmented, exfat_cluster_t fcl,
    exfat_cluster_t *clp, aoff64_t bn, int flags);

extern int fat_get_cluster(struct exfat_bs *bs, devmap_handle_t devmap_handle,
    exfat_cluster_t clst, exfat_cluster_t *value);
extern int fat_set_cluster(struct exfat_bs *bs, devmap_handle_t devmap_handle,
    exfat_cluster_t clst, exfat_cluster_t value);
extern int exfat_sanity_check(struct exfat_bs *, devmap_handle_t);



#endif

/**
 * @}
 */
