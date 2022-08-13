/*
 * SPDX-FileCopyrightText: 2009 Jiri Svoboda
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup ata_bd
 * @{
 */
/** @file ATA driver definitions.
 */

#ifndef __ATA_BD_H__
#define __ATA_BD_H__

#include <async.h>
#include <bd_srv.h>
#include <ddf/driver.h>
#include <fibril_synch.h>
#include <str.h>
#include <stdint.h>
#include <stddef.h>
#include "ata_hw.h"

#define NAME "ata_bd"

/** Base addresses for ATA I/O blocks. */
typedef struct {
	uintptr_t cmd;	/**< Command block base address. */
	uintptr_t ctl;	/**< Control block base address. */
} ata_base_t;

/** Timeout definitions. Unit is 10 ms. */
enum ata_timeout {
	TIMEOUT_PROBE	=  100, /*  1 s */
	TIMEOUT_BSY	=  100, /*  1 s */
	TIMEOUT_DRDY	= 1000  /* 10 s */
};

enum ata_dev_type {
	ata_reg_dev,	/* Register device (no packet feature set support) */
	ata_pkt_dev	/* Packet device (supports packet feature set). */
};

/** Register device block addressing mode. */
enum rd_addr_mode {
	am_chs,		/**< CHS block addressing */
	am_lba28,	/**< LBA-28 block addressing */
	am_lba48	/**< LBA-48 block addressing */
};

/** Block coordinates */
typedef struct {
	enum rd_addr_mode amode;

	union {
		/** CHS coordinates */
		struct {
			uint8_t sector;
			uint8_t cyl_lo;
			uint8_t cyl_hi;
		};
		/** LBA coordinates */
		struct {
			uint8_t c0;
			uint8_t c1;
			uint8_t c2;
			uint8_t c3;
			uint8_t c4;
			uint8_t c5;
		};
	};

	/** Lower 4 bits for device/head register */
	uint8_t h;
} block_coord_t;

/** ATA device state structure. */
typedef struct {
	bool present;
	struct ata_ctrl *ctrl;
	struct ata_fun *afun;

	/** Device type */
	enum ata_dev_type dev_type;

	/** Addressing mode to use (if register device) */
	enum rd_addr_mode amode;

	/*
	 * Geometry. Only valid if operating in CHS mode.
	 */
	struct {
		unsigned heads;
		unsigned cylinders;
		unsigned sectors;
	} geom;

	uint64_t blocks;
	size_t block_size;

	char model[STR_BOUNDS(40) + 1];

	int disk_id;
} disk_t;

/** ATA controller */
typedef struct ata_ctrl {
	/** DDF device */
	ddf_dev_t *dev;
	/** I/O base address of the command registers */
	uintptr_t cmd_physical;
	/** I/O base address of the control registers */
	uintptr_t ctl_physical;

	/** Command registers */
	ata_cmd_t *cmd;
	/** Control registers */
	ata_ctl_t *ctl;

	/** Per-disk state. */
	disk_t disk[MAX_DISKS];

	fibril_mutex_t lock;
} ata_ctrl_t;

typedef struct ata_fun {
	ddf_fun_t *fun;
	disk_t *disk;
	bd_srvs_t bds;
} ata_fun_t;

extern errno_t ata_ctrl_init(ata_ctrl_t *, ata_base_t *);
extern errno_t ata_ctrl_remove(ata_ctrl_t *);
extern errno_t ata_ctrl_gone(ata_ctrl_t *);

extern bd_ops_t ata_bd_ops;

#endif

/** @}
 */
