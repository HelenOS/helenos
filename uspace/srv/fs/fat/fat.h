/*
 * Copyright (c) 2007 Jakub Jermar
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
#include <atomic.h>
#include <sys/types.h>
#include <bool.h>

#define dprintf(...)	printf(__VA_ARGS__)

typedef struct {
	uint8_t		ji[3];		/**< Jump instruction. */
	uint8_t		oem_name[8];
	/* BIOS Parameter Block */
	uint16_t	bps;		/**< Bytes per sector. */
	uint8_t		spc;		/**< Sectors per cluster. */
	uint16_t	rsc;		/**< Reserved sector count. */
	uint8_t		fatcnt;		/**< Number of FATs. */
	uint16_t	root_ent_max;	/**< Maximum number of root directory
					     entries. */
	uint16_t	totsec;		/**< Total sectors. */
	uint8_t		mdesc;		/**< Media descriptor. */
	uint16_t	sec_per_fat;	/**< Sectors per FAT12/FAT16. */
	uint16_t	sec_per_track;	/**< Sectors per track. */
	uint16_t	headcnt;	/**< Number of heads. */
	uint32_t	hidden_sec;	/**< Hidden sectors. */
	uint32_t	total_sec;	/**< Total sectors. */

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
			uint8_t		label;
			/** FAT type. */
			uint8_t		type[8];
			/** Boot code. */
			uint8_t		boot_code[420];
			/** Signature. */
			uint16_t	signature;
		} __attribute__ ((packed));
	}; 
} __attribute__ ((packed)) fat_bs_t;

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

extern uint8_t *plb_ro;

extern void fat_lookup(ipc_callid_t, ipc_call_t *);

/*
 * The following interfaces are rather fs-neutral and might be later moved to a
 * dedicated library (e.g. libfs). We just wait until the interfaces stabilize
 * and until there is more than one fs implementation.
 */
extern int block_read(int, unsigned long, void *);
extern int block_write(int, unsigned long, void *);

extern void node_add_mp(int, unsigned long);
extern void node_del_mp(int, unsigned long);
extern bool node_is_mp(int, unsigned long);

#endif

/**
 * @}
 */
