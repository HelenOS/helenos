/*
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
