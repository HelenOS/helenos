/*
 * SPDX-FileCopyrightText: 2011 Oleg Romanenko
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup exfat
 * @{
 */

#ifndef EXFAT_EXFAT_BITMAP_H_
#define EXFAT_EXFAT_BITMAP_H_

#include <stdint.h>
#include "exfat.h"
#include "exfat_fat.h"

/* forward declarations */
struct exfat_node;
struct exfat_bs;

extern errno_t exfat_bitmap_alloc_clusters(struct exfat_bs *, service_id_t,
    exfat_cluster_t *, exfat_cluster_t);
extern errno_t exfat_bitmap_append_clusters(struct exfat_bs *, struct exfat_node *,
    exfat_cluster_t);
extern errno_t exfat_bitmap_free_clusters(struct exfat_bs *, struct exfat_node *,
    exfat_cluster_t);
extern errno_t exfat_bitmap_replicate_clusters(struct exfat_bs *, struct exfat_node *);

extern errno_t exfat_bitmap_is_free(struct exfat_bs *, service_id_t, exfat_cluster_t);
extern errno_t exfat_bitmap_set_cluster(struct exfat_bs *, service_id_t, exfat_cluster_t);
extern errno_t exfat_bitmap_clear_cluster(struct exfat_bs *, service_id_t,
    exfat_cluster_t);

extern errno_t exfat_bitmap_set_clusters(struct exfat_bs *, service_id_t,
    exfat_cluster_t, exfat_cluster_t);
extern errno_t exfat_bitmap_clear_clusters(struct exfat_bs *, service_id_t,
    exfat_cluster_t, exfat_cluster_t);

#endif

/**
 * @}
 */
