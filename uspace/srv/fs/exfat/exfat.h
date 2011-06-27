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
#define EXFAT_FAT_H_

#include <fibril_synch.h>
#include <libfs.h>
#include <atomic.h>
#include <sys/types.h>
#include <bool.h>
#include "../../vfs/vfs.h"

#ifndef dprintf
#define dprintf(...)	printf(__VA_ARGS__)
#endif

#define BS_BLOCK		0
#define BS_SIZE			512

#define BPS(bs)			(1 << (bs->bytes_per_sector))
#define SPC(bs)			(1 << (bs->sec_per_cluster))
#define VOL_ST(bs)		uint64_t_le2host(bs->volume_start)
#define VOL_CNT(bs)		uint64_t_le2host(bs->volume_count)
#define FAT_ST(bs)		uint32_t_le2host(bs->fat_sector_start)
#define FAT_CNT(bs)		uint32_t_le2host(bs->fat_sector_count)
#define DATA_ST(bs)		uint32_t_le2host(bs->data_start_sector)
#define DATA_CNT(bs)	uint32_t_le2host(bs->data_clusters)
#define ROOT_ST(bs)		uint32_t_le2host(bs->rootdir_cluster)
#define VOL_FLAGS		uint16_t_le2host(bs->volume_flags)


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


extern fs_reg_t exfat_reg;

extern void exfat_mounted(ipc_callid_t, ipc_call_t *);
extern void exfat_mount(ipc_callid_t, ipc_call_t *);
extern void exfat_unmounted(ipc_callid_t, ipc_call_t *);
extern void exfat_unmount(ipc_callid_t, ipc_call_t *);

#endif

/**
 * @}
 */
