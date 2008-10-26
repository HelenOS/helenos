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

#ifndef FAT_FAT_H_
#define FAT_FAT_H_

#include <ipc/ipc.h>
#include <libfs.h>
#include <atomic.h>
#include <sys/types.h>
#include <bool.h>
#include "../../vfs/vfs.h"

#ifndef dprintf
#define dprintf(...)	printf(__VA_ARGS__)
#endif

typedef struct {
	uint8_t		ji[3];		/**< Jump instruction. */
	uint8_t		oem_name[8];
	/* BIOS Parameter Block */
	uint16_t	bps;		/**< Bytes per sector. */
	uint8_t		spc;		/**< Sectors per cluster. */
	uint16_t	rscnt;		/**< Reserved sector count. */
	uint8_t		fatcnt;		/**< Number of FATs. */
	uint16_t	root_ent_max;	/**< Maximum number of root directory
					     entries. */
	uint16_t	totsec16;	/**< Total sectors. 16-bit version. */
	uint8_t		mdesc;		/**< Media descriptor. */
	uint16_t	sec_per_fat;	/**< Sectors per FAT12/FAT16. */
	uint16_t	sec_per_track;	/**< Sectors per track. */
	uint16_t	headcnt;	/**< Number of heads. */
	uint32_t	hidden_sec;	/**< Hidden sectors. */
	uint32_t	totsec32;	/**< Total sectors. 32-bit version. */

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
		} __attribute__ ((packed));
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
		} __attribute__ ((packed));
	}; 
} __attribute__ ((packed)) fat_bs_t;

typedef uint16_t fat_cluster_t;

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
	link_t		uph_link;
	/** Used indices (index) hash table link. */
	link_t		uih_link;

	futex_t		lock;
	dev_handle_t	dev_handle;
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
	futex_t			lock;
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
	size_t			size;
	unsigned		lnkcnt;
	unsigned		refcnt;
	bool			dirty;
} fat_node_t;

extern fs_reg_t fat_reg;

extern void fat_mounted(ipc_callid_t, ipc_call_t *);
extern void fat_mount(ipc_callid_t, ipc_call_t *);
extern void fat_lookup(ipc_callid_t, ipc_call_t *);
extern void fat_read(ipc_callid_t, ipc_call_t *);
extern void fat_write(ipc_callid_t, ipc_call_t *);

extern fat_idx_t *fat_idx_get_by_pos(dev_handle_t, fat_cluster_t, unsigned);
extern fat_idx_t *fat_idx_get_by_index(dev_handle_t, fs_index_t);

extern int fat_idx_init(void);
extern void fat_idx_fini(void);
extern int fat_idx_init_by_dev_handle(dev_handle_t);
extern void fat_idx_fini_by_dev_handle(dev_handle_t);

#endif

/**
 * @}
 */
