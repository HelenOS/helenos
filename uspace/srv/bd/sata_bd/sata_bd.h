/*
 * SPDX-FileCopyrightText: 2012 Petr Jerman
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup sata_bd
 * @{
 */
/** @file SATA block device driver definitions.
 */

#ifndef __SATA_BD_H__
#define __SATA_BD_H__

#define SATA_DEV_NAME_LENGTH 256

#include <async.h>
#include <bd_srv.h>
#include <loc.h>
#include <stddef.h>
#include <stdint.h>

/** SATA Block Device. */
typedef struct {
	/** Device name in device tree. */
	char *dev_name;
	/** SATA Device name. */
	char sata_dev_name[SATA_DEV_NAME_LENGTH];
	/** Session to device methods. */
	async_sess_t *sess;
	/** Loc service id. */
	service_id_t service_id;
	/** Number of blocks. */
	uint64_t blocks;
	/** Size of block. */
	size_t block_size;
	/** Block device server structure */
	bd_srvs_t bds;
} sata_bd_dev_t;

#endif

/** @}
 */
