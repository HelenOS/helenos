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
#include <block.h>

#define EXFAT_ROOT_IDX		0
#define EXFAT_BITMAP_IDX	1
#define EXFAT_UCTABLE_IDX	2

#define EXFAT_ROOT_PAR	0
#define EXFAT_ROOT_POS	0

#define EXFAT_CLST_FIRST	0x00000002
#define EXFAT_CLST_LAST		0xfffffff6
#define EXFAT_CLST_BAD		0xfffffff7
#define EXFAT_CLST_EOF		0xffffffff

/* forward declarations */
struct block;
struct exfat_node;
struct exfat_bs;

typedef uint32_t exfat_cluster_t;

#define exfat_clusters_get(numc, bs, sid, fc) \
    exfat_cluster_walk((bs), (sid), (fc), NULL, (numc), (uint32_t) -1)

extern errno_t exfat_cluster_walk(struct exfat_bs *, service_id_t,
    exfat_cluster_t, exfat_cluster_t *, uint32_t *, uint32_t);
extern errno_t exfat_block_get(block_t **, struct exfat_bs *, struct exfat_node *,
    aoff64_t, int);
extern errno_t exfat_block_get_by_clst(block_t **, struct exfat_bs *, service_id_t,
    bool, exfat_cluster_t, exfat_cluster_t *, aoff64_t, int);

extern errno_t exfat_get_cluster(struct exfat_bs *, service_id_t, exfat_cluster_t,
    exfat_cluster_t *);
extern errno_t exfat_set_cluster(struct exfat_bs *, service_id_t, exfat_cluster_t,
    exfat_cluster_t);
extern errno_t exfat_sanity_check(struct exfat_bs *);

extern errno_t exfat_append_clusters(struct exfat_bs *, struct exfat_node *,
    exfat_cluster_t, exfat_cluster_t);
extern errno_t exfat_chop_clusters(struct exfat_bs *, struct exfat_node *,
    exfat_cluster_t);
extern errno_t exfat_alloc_clusters(struct exfat_bs *, service_id_t, unsigned,
    exfat_cluster_t *, exfat_cluster_t *);
extern errno_t exfat_free_clusters(struct exfat_bs *, service_id_t, exfat_cluster_t);
extern errno_t exfat_zero_cluster(struct exfat_bs *, service_id_t, exfat_cluster_t);

extern errno_t exfat_read_uctable(struct exfat_bs *, struct exfat_node *,
    uint8_t *);

#endif

/**
 * @}
 */
