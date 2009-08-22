/*
 * Copyright (c) 2009 Jiri Svoboda
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

/** @addtogroup bd
 * @{
 */
/** @file
 */

#ifndef __ATA_BD_H__
#define __ATA_BD_H__

#include <sys/types.h>
#include <fibril_sync.h>
#include <string.h>

enum {
	CTL_READ_START	= 0,
	CTL_WRITE_START = 1,
};

enum {
	STATUS_FAILURE	= 0
};

enum {
	MAX_DISKS	= 2
};

/** ATA Command Register Block. */
typedef union {
	/* Read/Write */
	struct {
		uint16_t data_port;
		uint8_t sector_count;
		uint8_t sector_number;
		uint8_t cylinder_low;
		uint8_t cylinder_high;
		uint8_t drive_head;
		uint8_t pad_rw0;		
	};

	/* Read Only */
	struct {
		uint8_t pad_ro0;
		uint8_t error;
		uint8_t pad_ro1[5];
		uint8_t status;
	};

	/* Write Only */
	struct {
		uint8_t pad_wo0;
		uint8_t features;
		uint8_t pad_wo1[5];
		uint8_t command;
	};
} ata_cmd_t;

typedef union {
	/* Read */
	struct {
		uint8_t pad0[6];
		uint8_t alt_status;
		uint8_t drive_address;
	};

	/* Write */
	struct {
		uint8_t pad1[6];
		uint8_t device_control;
		uint8_t pad2;
	};
} ata_ctl_t;

enum devctl_bits {
	DCR_SRST	= 0x04, /**< Software Reset */
	DCR_nIEN	= 0x02  /**< Interrupt Enable (negated) */
};

enum status_bits {
	SR_BSY		= 0x80, /**< Busy */
	SR_DRDY		= 0x40, /**< Drive Ready */
	SR_DWF		= 0x20, /**< Drive Write Fault */
	SR_DSC		= 0x10, /**< Drive Seek Complete */
	SR_DRQ		= 0x08, /**< Data Request */
	SR_CORR		= 0x04, /**< Corrected Data */
	SR_IDX		= 0x02, /**< Index */
	SR_ERR		= 0x01  /**< Error */
};

enum drive_head_bits {
	DHR_LBA		= 0x40,	/**< Use LBA addressing mode */
	DHR_DRV		= 0x10	/**< Select device 1 */
};

enum error_bits {
	ER_BBK		= 0x80, /**< Bad Block Detected */
	ER_UNC		= 0x40, /**< Uncorrectable Data Error */
	ER_MC		= 0x20, /**< Media Changed */
	ER_IDNF		= 0x10, /**< ID Not Found */
	ER_MCR		= 0x08, /**< Media Change Request */
	ER_ABRT		= 0x04, /**< Aborted Command */
	ER_TK0NF	= 0x02, /**< Track 0 Not Found */
	ER_AMNF		= 0x01  /**< Address Mark Not Found */
};

enum ata_command {
	CMD_IDENTIFY_DRIVE	= 0xEC,
	CMD_READ_SECTORS	= 0x20,
	CMD_WRITE_SECTORS	= 0x30
};

/** Timeout definitions. Unit is 10 ms. */
enum ata_timeout {
	TIMEOUT_PROBE	=  100, /*  1 s */
	TIMEOUT_BSY	=  100, /*  1 s */
	TIMEOUT_DRDY	= 1000  /* 10 s */
};

/** Data returned from @c identify command. */
typedef struct {
	uint16_t gen_conf;
	uint16_t cylinders;
	uint16_t _res2;
	uint16_t heads;
	uint16_t _vs4;
	uint16_t _vs5;
	uint16_t sectors;
	uint16_t _vs7;
	uint16_t _vs8;
	uint16_t _vs9;

	uint16_t serial_number[10];
	uint16_t _vs20;
	uint16_t _vs21;
	uint16_t vs_bytes;
	uint16_t firmware_rev[4];
	uint16_t model_name[20];

	uint16_t max_rw_multiple;
	uint16_t _res48;
	uint16_t caps;
	uint16_t _res50;
	uint16_t pio_timing;
	uint16_t dma_timing;

	uint16_t validity;
	uint16_t cur_cyl;
	uint16_t cur_heads;
	uint16_t cur_sectors;
	uint16_t cur_capacity0;
	uint16_t cur_capacity1;
	uint16_t mss;
	uint16_t total_lba28_0;
	uint16_t total_lba28_1;
	uint16_t sw_dma;
	uint16_t mw_dma;
	uint16_t pio_modes;
	uint16_t min_mw_dma_cycle;
	uint16_t rec_mw_dma_cycle;
	uint16_t min_raw_pio_cycle;
	uint16_t min_iordy_pio_cycle;

	uint16_t _res69;
	uint16_t _res70;
	uint16_t _res71;
	uint16_t _res72;
	uint16_t _res73;
	uint16_t _res74;

	uint16_t queue_depth;
	uint16_t _res76[1 + 79 - 76];
	uint16_t version_maj;
	uint16_t version_min;
	uint16_t cmd_set0;
	uint16_t cmd_set1;
	uint16_t csf_sup_ext;
	uint16_t csf_enabled0;
	uint16_t csf_enabled1;
	uint16_t csf_default;
	uint16_t udma;

	uint16_t _res89[1 + 99 - 89];

	/* Total number of blocks in LBA-48 addressing */
	uint16_t total_lba48_0;
	uint16_t total_lba48_1;
	uint16_t total_lba48_2;
	uint16_t total_lba48_3;

	/* Note: more fields are defined in ATA/ATAPI-7 */
	uint16_t _res104[1 + 127 - 104];
	uint16_t _vs128[1 + 159 - 128];
	uint16_t _res160[1 + 255 - 160];
} identify_data_t;

enum ata_caps {
	cap_iordy	= 0x0800,
	cap_iordy_cbd	= 0x0400,
	cap_lba		= 0x0200,
	cap_dma		= 0x0100
};

/** Bits of @c identify_data_t.cmd_set1 */
enum ata_cs1 {
	cs1_addr48	= 0x0400	/**< 48-bit address feature set */
};

/** Block addressing mode. */
enum addr_mode {
	am_chs,		/**< CHS block addressing */
	am_lba28,	/**< LBA-28 block addressing */
	am_lba48	/**< LBA-48 block addressing */
};

typedef struct {
	bool present;
	enum addr_mode amode;

	/*
	 * Geometry. Only valid if operating in CHS mode.
	 */
	struct {
		unsigned heads;
		unsigned cylinders;
		unsigned sectors;
	} geom;

	uint64_t blocks;

	char model[STR_BOUNDS(40) + 1];

	fibril_mutex_t lock;
	dev_handle_t dev_handle;
} disk_t;

#endif

/** @}
 */
