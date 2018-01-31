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

#ifndef FAT_FAT_FAT_H_
#define FAT_FAT_FAT_H_

#include "../../vfs/vfs.h"
#include <stdint.h>
#include <block.h>

#define FAT1		0

#define FAT_CLST_RES0	  0
#define FAT_CLST_RES1	  1
#define FAT_CLST_FIRST	  2

#define FAT32_CLST_BAD    0x0ffffff7
#define FAT32_CLST_LAST1  0x0ffffff8
#define FAT32_CLST_LAST8  0x0fffffff

#define FAT12_MASK        0x0fff
#define FAT16_MASK        0xffff
#define FAT32_MASK        0x0fffffff

#define FAT12_CLST_MAX    4085
#define FAT16_CLST_MAX    65525

/* Size in bytes for cluster value of FAT */
#define FAT12_CLST_SIZE   2
#define FAT16_CLST_SIZE   2
#define FAT32_CLST_SIZE   4

/* internally used to mark root directory's parent */
#define FAT_CLST_ROOTPAR	FAT_CLST_RES0
/* internally used to mark root directory */
#define FAT_CLST_ROOT		FAT_CLST_RES1

/*
 * Convenience macros for computing some frequently used values from the
 * primitive boot sector members.
 */
#define RDS(bs)	  ((sizeof(fat_dentry_t) * RDE((bs))) / BPS((bs))) + \
		   (((sizeof(fat_dentry_t) * RDE((bs))) % BPS((bs))) != 0)
#define SSA(bs)	  (RSCNT((bs)) + FATCNT((bs)) * SF((bs)) + RDS(bs))
#define DS(bs)	  (TS(bs) - SSA(bs))
#define CC(bs)	  (DS(bs) / SPC(bs))

#define CLBN2PBN(bs, cl, bn) \
	(SSA((bs)) + ((cl) - FAT_CLST_FIRST) * SPC((bs)) + (bn) % SPC((bs)))

#define FAT_IS_FAT12(bs)	(CC(bs) < FAT12_CLST_MAX)
#define FAT_IS_FAT16(bs) \
    ((CC(bs) >= FAT12_CLST_MAX) && (CC(bs) < FAT16_CLST_MAX))
#define FAT_IS_FAT32(bs)	(CC(bs) >= FAT16_CLST_MAX)

#define FAT_CLST_SIZE(bs) \
    (FAT_IS_FAT32(bs) ? FAT32_CLST_SIZE : FAT16_CLST_SIZE)

#define FAT_MASK(bs) \
    (FAT_IS_FAT12(bs) ? FAT12_MASK : \
    (FAT_IS_FAT32(bs) ? FAT32_MASK : FAT16_MASK))

#define FAT_CLST_LAST1(bs)      (FAT32_CLST_LAST1 & FAT_MASK((bs)))
#define FAT_CLST_LAST8(bs)      (FAT32_CLST_LAST8 & FAT_MASK((bs)))
#define FAT_CLST_BAD(bs)        (FAT32_CLST_BAD & FAT_MASK((bs)))

#define FAT_ROOT_CLST(bs) \
    (FAT_IS_FAT32(bs) ? uint32_t_le2host(bs->fat32.root_cluster) : \
    FAT_CLST_ROOT)

/* forward declarations */
struct block;
struct fat_node;
struct fat_bs;

typedef uint32_t fat_cluster_t;

#define fat_clusters_get(numc, bs, sid, fc) \
    fat_cluster_walk((bs), (sid), (fc), NULL, (numc), (uint32_t) -1)
extern errno_t fat_cluster_walk(struct fat_bs *, service_id_t, fat_cluster_t,
    fat_cluster_t *, uint32_t *, uint32_t);

extern errno_t fat_block_get(block_t **, struct fat_bs *, struct fat_node *,
    aoff64_t, int);
extern errno_t _fat_block_get(block_t **, struct fat_bs *, service_id_t,
    fat_cluster_t, fat_cluster_t *, aoff64_t, int);

extern errno_t fat_append_clusters(struct fat_bs *, struct fat_node *,
    fat_cluster_t, fat_cluster_t);
extern errno_t fat_chop_clusters(struct fat_bs *, struct fat_node *,
    fat_cluster_t);
extern errno_t fat_alloc_clusters(struct fat_bs *, service_id_t, unsigned,
    fat_cluster_t *, fat_cluster_t *);
extern errno_t fat_free_clusters(struct fat_bs *, service_id_t, fat_cluster_t);
extern errno_t fat_alloc_shadow_clusters(struct fat_bs *, service_id_t,
    fat_cluster_t *, unsigned);
extern errno_t fat_get_cluster(struct fat_bs *, service_id_t, unsigned,
    fat_cluster_t, fat_cluster_t *);
extern errno_t fat_set_cluster(struct fat_bs *, service_id_t, unsigned,
    fat_cluster_t, fat_cluster_t);
extern errno_t fat_fill_gap(struct fat_bs *, struct fat_node *, fat_cluster_t,
    aoff64_t);
extern errno_t fat_zero_cluster(struct fat_bs *, service_id_t, fat_cluster_t);
extern errno_t fat_sanity_check(struct fat_bs *, service_id_t);

#endif

/**
 * @}
 */
