/*
 * SPDX-FileCopyrightText: 2008 Jakub Jermar
 * SPDX-FileCopyrightText: 2011 Oleg Romanenko
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup fat
 * @{
 */

#ifndef FAT_FAT_H_
#define FAT_FAT_H_

#include "fat_fat.h"
#include <fibril_synch.h>
#include <libfs.h>
#include <stdint.h>
#include <stdbool.h>
#include <macros.h>
#include "../../vfs/vfs.h"

#ifndef dprintf
#define dprintf(...)	printf(__VA_ARGS__)
#endif

/*
 * Convenience macros for accessing some frequently used boot sector members.
 */
#define BPS(bs)		uint16_t_le2host((bs)->bps)
#define SPC(bs)		(bs)->spc
#define RSCNT(bs)	uint16_t_le2host((bs)->rscnt)
#define FATCNT(bs)	(bs)->fatcnt

#define SF(bs)		(uint16_t_le2host((bs)->sec_per_fat) ? \
    uint16_t_le2host((bs)->sec_per_fat) : \
    uint32_t_le2host(bs->fat32.sectors_per_fat))

#define RDE(bs)		uint16_t_le2host((bs)->root_ent_max)

#define TS(bs)		(uint16_t_le2host((bs)->totsec16) ? \
    uint16_t_le2host((bs)->totsec16) : \
    uint32_t_le2host(bs->totsec32))

#define BS_BLOCK	0
#define BS_SIZE		512

typedef struct fat_bs {
	/** Jump instruction */
	uint8_t		ji[3];
	uint8_t		oem_name[8];

	/* BIOS Parameter Block */

	/** Bytes per sector */
	uint16_t	bps;
	/** Sectors per cluster */
	uint8_t		spc;
	/** Reserved sector count */
	uint16_t	rscnt;
	/** Number of FATs */
	uint8_t		fatcnt;
	/** Maximum number of root directory entries */
	uint16_t	root_ent_max;
	/** Total sectors. 16-bit version */
	uint16_t	totsec16;
	/** Media descriptor */
	uint8_t		mdesc;
	/** Sectors per FAT12/FAT16 */
	uint16_t	sec_per_fat;
	/** Sectors per track */
	uint16_t	sec_per_track;
	/** Number of heads */
	uint16_t	headcnt;
	/** Hidden sectors */
	uint32_t	hidden_sec;
	/** Total sectors. 32-bit version */
	uint32_t	totsec32;

	union {
		struct {
			/* FAT12/FAT16 only: Extended BIOS Parameter Block */
			/** Physical drive number. */
			uint8_t		pdn;
			uint8_t		reserved;
			/** Extended boot signature. */
			uint8_t		ebs;
			/** Serial number. */
			uint32_t	id;
			/** Volume label. */
			uint8_t		label[11];
			/** FAT type. */
			uint8_t		type[8];
			/** Boot code. */
			uint8_t		boot_code[448];
			/** Boot sector signature. */
			uint16_t	signature;
		} __attribute__((packed));
		struct {
			/* FAT32 only */
			/** Sectors per FAT. */
			uint32_t	sectors_per_fat;
			/** FAT flags. */
			uint16_t	flags;
			/** Version. */
			uint16_t	version;
			/** Cluster number of root directory. */
			uint32_t	root_cluster;
			/** Sector number of file system information sector. */
			uint16_t	fsinfo_sec;
			/** Sector number of boot sector copy. */
			uint16_t	bscopy_sec;
			uint8_t		reserved1[12];
			/** Physical drive number. */
			uint8_t		pdn;
			uint8_t		reserved2;
			/** Extended boot signature. */
			uint8_t		ebs;
			/** Serial number. */
			uint32_t	id;
			/** Volume label. */
			uint8_t		label[11];
			/** FAT type. */
			uint8_t		type[8];
			/** Boot code. */
			uint8_t		boot_code[420];
			/** Signature. */
			uint16_t	signature;
		} __attribute__((packed)) fat32;
	};
} __attribute__((packed)) fat_bs_t;

#define FAT32_FSINFO_SIG1	"RRaA"
#define FAT32_FSINFO_SIG2	"rrAa"
#define FAT32_FSINFO_SIG3	"\x00\x00\x55\xaa"

typedef struct {
	uint8_t	sig1[4];
	uint8_t res1[480];
	uint8_t sig2[4];
	uint32_t free_clusters;
	uint32_t last_allocated_cluster;
	uint8_t res2[12];
	uint8_t sig3[4];
} __attribute__((packed)) fat32_fsinfo_t;

typedef enum {
	FAT_INVALID,
	FAT_DIRECTORY,
	FAT_FILE
} fat_node_type_t;

struct fat_node;

/** FAT index structure.
 *
 * This structure exists to help us to overcome certain limitations of the FAT
 * file system design.  The problem with FAT is that it is hard to find
 * an entity which could represent a VFS index.  There are two candidates:
 *
 * a) number of the node's first cluster
 * b) the pair of the parent directory's first cluster and the dentry index
 *    within the parent directory
 *
 * We need VFS indices to be:
 * A) unique
 * B) stable in time, at least until the next mount
 *
 * Unfortunately a) does not meet the A) criterion because zero-length files
 * will have the first cluster field cleared.  And b) does not meet the B)
 * criterion because unlink() and rename() will both free up the original
 * dentry, which contains all the essential info about the file.
 *
 * Therefore, a completely opaque indices are used and the FAT server maintains
 * a mapping between them and otherwise nice b) variant.  On rename(), the VFS
 * index stays unaltered, while the internal FAT "physical tree address"
 * changes.  The unlink case is also handled this way thanks to an in-core node
 * pointer embedded in the index structure.
 */
typedef struct {
	/** Used indices (position) hash table link. */
	ht_link_t		uph_link;
	/** Used indices (index) hash table link. */
	ht_link_t		uih_link;

	fibril_mutex_t	lock;
	service_id_t	service_id;
	fs_index_t	index;
	/**
	 * Parent node's first cluster.
	 * Zero is used if this node is not linked, in which case nodep must
	 * contain a pointer to the in-core node structure.
	 * One is used when the parent is the root directory.
	 */
	fat_cluster_t	pfc;
	/** Directory entry index within the parent node. */
	unsigned	pdi;
	/** Pointer to in-core node instance. */
	struct fat_node	*nodep;
} fat_idx_t;

/** FAT in-core node. */
typedef struct fat_node {
	/** Back pointer to the FS node. */
	fs_node_t		*bp;

	fibril_mutex_t		lock;
	fat_node_type_t		type;
	fat_idx_t		*idx;
	/**
	 *  Node's first cluster.
	 *  Zero is used for zero-length nodes.
	 *  One is used to mark root directory.
	 */
	fat_cluster_t		firstc;
	/** FAT in-core node free list link. */
	link_t			ffn_link;
	aoff64_t		size;
	unsigned		lnkcnt;
	unsigned		refcnt;
	bool			dirty;

	/*
	 * Cache of the node's last and "current" cluster to avoid some
	 * unnecessary FAT walks.
	 */
	/* Node's last cluster in FAT. */
	bool		lastc_cached_valid;
	fat_cluster_t	lastc_cached_value;
	/* Node's "current" cluster, i.e. where the last I/O took place. */
	bool		currc_cached_valid;
	aoff64_t	currc_cached_bn;
	fat_cluster_t	currc_cached_value;
} fat_node_t;

typedef struct {
	bool lfn_enabled;
} fat_instance_t;

extern vfs_out_ops_t fat_ops;
extern libfs_ops_t fat_libfs_ops;

extern errno_t fat_idx_get_new(fat_idx_t **, service_id_t);
extern fat_idx_t *fat_idx_get_by_pos(service_id_t, fat_cluster_t, unsigned);
extern fat_idx_t *fat_idx_get_by_index(service_id_t, fs_index_t);
extern void fat_idx_destroy(fat_idx_t *);
extern void fat_idx_hashin(fat_idx_t *);
extern void fat_idx_hashout(fat_idx_t *);

extern errno_t fat_idx_init(void);
extern void fat_idx_fini(void);
extern errno_t fat_idx_init_by_service_id(service_id_t);
extern void fat_idx_fini_by_service_id(service_id_t);

#endif

/**
 * @}
 */
