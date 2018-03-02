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

#ifndef EXFAT_EXFAT_H_
#define EXFAT_EXFAT_H_

#include "exfat_fat.h"
#include <fibril_synch.h>
#include <libfs.h>
#include <atomic.h>
#include <stdint.h>
#include <stdbool.h>
#include "../../vfs/vfs.h"

#ifndef dprintf
#define dprintf(...)	printf(__VA_ARGS__)
#endif

#define BS_BLOCK		0
#define BS_SIZE			512

#define BPS(bs)			((uint32_t) (1 << (bs->bytes_per_sector)))
#define SPC(bs)			((uint32_t) (1 << (bs->sec_per_cluster)))
#define BPC(bs)			((uint32_t) (BPS(bs) * SPC(bs)))
#define VOL_FS(bs)		uint64_t_le2host(bs->volume_start)
#define VOL_CNT(bs)		uint64_t_le2host(bs->volume_count)
#define FAT_FS(bs)		uint32_t_le2host(bs->fat_sector_start)
#define FAT_CNT(bs)		uint32_t_le2host(bs->fat_sector_count)
#define DATA_FS(bs)		uint32_t_le2host(bs->data_start_sector)
#define DATA_CNT(bs)		uint32_t_le2host(bs->data_clusters)
#define ROOT_FC(bs)		uint32_t_le2host(bs->rootdir_cluster)
#define VOL_FLAGS(bs)		uint16_t_le2host(bs->volume_flags)

#define EXFAT_NODE(node)	((node) ? (exfat_node_t *) (node)->data : NULL)
#define FS_NODE(node)	((node) ? (node)->bp : NULL)
#define DPS(bs) (BPS((bs)) / sizeof(exfat_dentry_t))

typedef struct exfat_bs {
	uint8_t jump[3];				/* 0x00 jmp and nop instructions */
	uint8_t oem_name[8];			/* 0x03 "EXFAT   " */
	uint8_t	__reserved[53];			/* 0x0B always 0 */
	uint64_t volume_start;			/* 0x40 partition first sector */
	uint64_t volume_count;			/* 0x48 partition sectors count */
	uint32_t fat_sector_start;		/* 0x50 FAT first sector */
	uint32_t fat_sector_count;		/* 0x54 FAT sectors count */
	uint32_t data_start_sector;		/* 0x58 Data region first cluster sector */
	uint32_t data_clusters;			/* 0x5C total clusters count */
	uint32_t rootdir_cluster;		/* 0x60 first cluster of the root dir */
	uint32_t volume_serial;			/* 0x64 volume serial number */
	struct {						/* 0x68 FS version */
		uint8_t minor;
		uint8_t major;
	} __attribute__ ((packed)) version;
	uint16_t volume_flags;			/* 0x6A volume state flags */
	uint8_t bytes_per_sector;		/* 0x6C sector size as (1 << n) */
	uint8_t sec_per_cluster;		/* 0x6D sectors per cluster as (1 << n) */
	uint8_t fat_count;				/* 0x6E always 1 */
	uint8_t drive_no;				/* 0x6F always 0x80 */
	uint8_t allocated_percent;		/* 0x70 percentage of allocated space */
	uint8_t _reserved2[7];			/* 0x71 reserved */
	uint8_t bootcode[390];			/* Boot code */
	uint16_t signature;				/* the value of 0xAA55 */
} __attribute__((__packed__)) exfat_bs_t;

typedef enum {
	EXFAT_UNKNOW,
	EXFAT_DIRECTORY,
	EXFAT_FILE,
	EXFAT_BITMAP,
	EXFAT_UCTABLE
} exfat_node_type_t;

struct exfat_node;
struct exfat_idx_t;

typedef struct {
	/** Used indices (position) hash table link. */
	ht_link_t		uph_link;
	/** Used indices (index) hash table link. */
	ht_link_t		uih_link;

	fibril_mutex_t	lock;
	service_id_t	service_id;
	fs_index_t	index;

	/* Does parent node fragmented or not? */
	bool parent_fragmented;
	/**
	 * Parent node's first cluster.
	 * Zero is used if this node is not linked, in which case nodep must
	 * contain a pointer to the in-core node structure.
	 * One is used when the parent is the root directory.
	 */
	exfat_cluster_t	pfc;
	/** Directory entry index within the parent node. */
	unsigned	pdi;
	/** Pointer to in-core node instance. */
	struct exfat_node	*nodep;
} exfat_idx_t;

/** exFAT in-core node. */
typedef struct exfat_node {
	/** Back pointer to the FS node. */
	fs_node_t		*bp;

	fibril_mutex_t		lock;
	exfat_node_type_t	type;
	exfat_idx_t			*idx;
	/**
	 *  Node's first cluster.
	 *  Zero is used for zero-length nodes.
	 *  One is used to mark root directory.
	 */
	exfat_cluster_t		firstc;
	/** exFAT in-core node free list link. */
	link_t			ffn_link;
	aoff64_t		size;
	unsigned		lnkcnt;
	unsigned		refcnt;
	bool			dirty;
	/* Should we do walk-on-FAT or not */
	bool			fragmented;

	/*
	 * Cache of the node's last and "current" cluster to avoid some
	 * unnecessary FAT walks.
	 */
	/* Node's last cluster in FAT. */
	bool		lastc_cached_valid;
	exfat_cluster_t	lastc_cached_value;
	/* Node's "current" cluster, i.e. where the last I/O took place. */
	bool		currc_cached_valid;
	aoff64_t	currc_cached_bn;
	exfat_cluster_t	currc_cached_value;
} exfat_node_t;


extern vfs_out_ops_t exfat_ops;
extern libfs_ops_t exfat_libfs_ops;

extern errno_t exfat_idx_get_new(exfat_idx_t **, service_id_t);
extern exfat_idx_t *exfat_idx_get_by_pos(service_id_t, exfat_cluster_t, unsigned);
extern exfat_idx_t *exfat_idx_get_by_index(service_id_t, fs_index_t);
extern void exfat_idx_destroy(exfat_idx_t *);
extern void exfat_idx_hashin(exfat_idx_t *);
extern void exfat_idx_hashout(exfat_idx_t *);

extern errno_t exfat_idx_init(void);
extern void exfat_idx_fini(void);
extern errno_t exfat_idx_init_by_service_id(service_id_t);
extern void exfat_idx_fini_by_service_id(service_id_t);

extern errno_t exfat_node_expand(service_id_t, exfat_node_t *, exfat_cluster_t);
extern errno_t exfat_node_put(fs_node_t *);
extern errno_t exfat_bitmap_get(fs_node_t **, service_id_t);
extern errno_t exfat_uctable_get(fs_node_t **, service_id_t);


#endif

/**
 * @}
 */
