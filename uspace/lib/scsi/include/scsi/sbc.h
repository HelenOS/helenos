/*
 * SPDX-FileCopyrightText: 2011 Jiri Svoboda
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup libscsi
 * @{
 */
/**
 * @file SCSI Block Commands.
 */

#ifndef LIBSCSI_SBC_H_
#define LIBSCSI_SBC_H_

#include <stdint.h>

/** SCSI command codes defined in SCSI-SBC */
enum scsi_cmd_sbc {
	SCSI_CMD_READ_6			= 0x08,
	SCSI_CMD_READ_10		= 0x28,
	SCSI_CMD_READ_12		= 0xa8,
	SCSI_CMD_READ_16		= 0x88,
	SCSI_CMD_READ_32		= 0x7f,

	SCSI_CMD_READ_CAPACITY_10	= 0x25,
	SCSI_CMD_READ_CAPACITY_16	= 0x9e,

	SCSI_CMD_SYNC_CACHE_10		= 0x35,
	SCSI_CMD_SYNC_CACHE_16		= 0x91,

	SCSI_CMD_WRITE_6		= 0x0a,
	SCSI_CMD_WRITE_10		= 0x2a,
	SCSI_CMD_WRITE_12		= 0xaa,
	SCSI_CMD_WRITE_16		= 0x8a
};

/** SCSI Read (10) command */
typedef struct {
	/** Operation code (SCSI_CMD_READ_10) */
	uint8_t op_code;
	/** RdProtect, DPO, FUA, Reserved, FUA_NV, Obsolete */
	uint8_t flags;
	/** Logical block address */
	uint32_t lba;
	/** Reserved, Group Number */
	uint8_t group_no;
	/** Transfer length */
	uint16_t xfer_len;
	/** Control */
	uint8_t control;
} __attribute__((packed)) scsi_cdb_read_10_t;

/** SCSI Read (12) command */
typedef struct {
	/** Operation code (SCSI_CMD_READ_12) */
	uint8_t op_code;
	/** RdProtect, DPO, FUA, Reserved, FUA_NV, Obsolete */
	uint8_t flags;
	/** Logical block address */
	uint32_t lba;
	/** Transfer length */
	uint32_t xfer_len;
	/** Reserved, Group Number */
	uint8_t group_no;
	/** Control */
	uint8_t control;
} __attribute__((packed)) scsi_cdb_read_12_t;

/** SCSI Read (16) command */
typedef struct {
	/** Operation code (SCSI_CMD_READ_16) */
	uint8_t op_code;
	/** RdProtect, DPO, FUA, Reserved, FUA_NV, Reserved */
	uint8_t flags;
	/** Logical block address */
	uint64_t lba;
	/** Transfer length */
	uint32_t xfer_len;
	/** Reserved, Group Number */
	uint8_t group_no;
	/** Control */
	uint8_t control;
} __attribute__((packed)) scsi_cdb_read_16_t;

/** SCSI Read Capacity (10) command */
typedef struct {
	/** Operation code (SCSI_CMD_READ_CAPACITY_10) */
	uint8_t op_code;
	/** Reserved, Obsolete */
	uint8_t reserved_1;
	/** Logical block address */
	uint32_t lba;
	/** Reserved */
	uint32_t reserved_6;
	/** Reserved, PM */
	uint8_t pm;
	/** Control */
	uint8_t control;
} __attribute__((packed)) scsi_cdb_read_capacity_10_t;

/** Read Capacity (10) parameter data.
 *
 * Returned for Read Capacity (10) command.
 */
typedef struct {
	/** Logical address of last block */
	uint32_t last_lba;
	/** Size of block in bytes */
	uint32_t block_size;
} scsi_read_capacity_10_data_t;

/** SCSI Synchronize Cache (10) command */
typedef struct {
	/** Operation code (SCSI_CMD_SYNC_CACHE_10) */
	uint8_t op_code;
	/** Reserved, Sync_NV, Immed, Reserved */
	uint8_t flags;
	/** Logical block address */
	uint32_t lba;
	/** Reserved, Group Number */
	uint8_t group_no;
	/** Number of Logical Blocks */
	uint16_t numlb;
	/** Control */
	uint8_t control;
} __attribute__((packed)) scsi_cdb_sync_cache_10_t;

/** SCSI Synchronize Cache (16) command */
typedef struct {
	/** Operation code (SCSI_CMD_SYNC_CACHE_16) */
	uint8_t op_code;
	/** Reserved, Sync_NV, Immed, Reserved */
	uint8_t flags;
	/** Logical block address */
	uint64_t lba;
	/** Number of Logical Blocks */
	uint32_t numlb;
	/** Reserved, Group Number */
	uint8_t group_no;
	/** Control */
	uint8_t control;
} __attribute__((packed)) scsi_cdb_sync_cache_16_t;

/** SCSI Write (10) command */
typedef struct {
	/** Operation code (SCSI_CMD_WRITE_10) */
	uint8_t op_code;
	/** WrProtect, DPO, FUA, Reserved, FUA_NV, Obsolete */
	uint8_t flags;
	/** Logical block address */
	uint32_t lba;
	/** Reserved, Group Number */
	uint8_t group_no;
	/** Transfer length */
	uint16_t xfer_len;
	/** Control */
	uint8_t control;
} __attribute__((packed)) scsi_cdb_write_10_t;

/** SCSI Write (12) command */
typedef struct {
	/** Operation code (SCSI_CMD_WRITE_12) */
	uint8_t op_code;
	/** WrProtect, DPO, FUA, Reserved, FUA_NV, Obsolete */
	uint8_t flags;
	/** Logical block address */
	uint32_t lba;
	/** Transfer length */
	uint32_t xfer_len;
	/** Reserved, Group Number */
	uint8_t group_no;
	/** Control */
	uint8_t control;
} __attribute__((packed)) scsi_cdb_write_12_t;

/** SCSI Write (16) command */
typedef struct {
	/** Operation code (SCSI_CMD_WRITE_16) */
	uint8_t op_code;
	/** WrProtect, DPO, FUA, Reserved, FUA_NV, Reserved */
	uint8_t flags;
	/** Logical block address */
	uint64_t lba;
	/** Transfer length */
	uint32_t xfer_len;
	/** Reserved, Group Number */
	uint8_t group_no;
	/** Control */
	uint8_t control;
} __attribute__((packed)) scsi_cdb_write_16_t;

#endif

/** @}
 */
