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

#define dprintf(...)	printf(__VA_ARGS__)

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

#define FAT_ATTR_RDONLY		(1 << 0)
#define FAT_ATTR_VOLLABEL	(1 << 3)
#define FAT_ATTR_SUBDIR		(1 << 4)

typedef struct {
	uint8_t		name[8];
	uint8_t		ext[3];
	uint8_t		attr;
	uint8_t		reserved;
	uint8_t		ctime_fine;
	uint16_t	ctime;
	uint16_t	cdate;
	uint16_t	adate;
	union {
		uint16_t	eaidx;		/* FAT12/FAT16 */
		uint16_t	firstc_hi;	/* FAT32 */
	};
	uint16_t	mtime;
	uint16_t	mdate;
	union {
		uint16_t	firstc;		/* FAT12/FAT16 */
		uint16_t	firstc_lo;	/* FAT32 */
	};
	uint32_t	size;
} __attribute__ ((packed)) fat_dentry_t;

typedef enum {
	FAT_INVALID,
	FAT_DIRECTORY,
	FAT_FILE
} fat_node_type_t;

/** FAT in-core node. */
typedef struct {
	fat_node_type_t		type;
	/** VFS index is the node's first allocated cluster. */
	fs_index_t		index;
	/** VFS index of the parent node. */
	fs_index_t		pindex;
	dev_handle_t		dev_handle;
	/** FAT in-core node hash table link. */
	link_t 			fin_link;
	/** FAT in-core node free list link. */
	link_t			ffn_link;
	size_t			size;
	unsigned		lnkcnt;
	unsigned		refcnt;
	bool			dirty;
} fat_node_t;

extern fs_reg_t fat_reg;

extern void fat_lookup(ipc_callid_t, ipc_call_t *);

#endif

/**
 * @}
 */
