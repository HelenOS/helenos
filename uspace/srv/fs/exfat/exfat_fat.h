/*
 * SPDX-FileCopyrightText: 2008 Jakub Jermar
 * SPDX-FileCopyrightText: 2011 Oleg Romanenko
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup exfat
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
