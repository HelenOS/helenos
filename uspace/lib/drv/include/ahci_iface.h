/*
 * SPDX-FileCopyrightText: 2012 Petr Jerman
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup libdrv
 * @{
 */
/** @file
 * @brief AHCI interface definition.
 */

#ifndef LIBDRV_AHCI_IFACE_H_
#define LIBDRV_AHCI_IFACE_H_

#include "ddf/driver.h"
#include <async.h>

extern async_sess_t *ahci_get_sess(devman_handle_t, char **);

extern errno_t ahci_get_sata_device_name(async_sess_t *, size_t, char *);
extern errno_t ahci_get_num_blocks(async_sess_t *, uint64_t *);
extern errno_t ahci_get_block_size(async_sess_t *, size_t *);
extern errno_t ahci_read_blocks(async_sess_t *, uint64_t, size_t, void *);
extern errno_t ahci_write_blocks(async_sess_t *, uint64_t, size_t, void *);

/** AHCI device communication interface. */
typedef struct {
	errno_t (*get_sata_device_name)(ddf_fun_t *, size_t, char *);
	errno_t (*get_num_blocks)(ddf_fun_t *, uint64_t *);
	errno_t (*get_block_size)(ddf_fun_t *, size_t *);
	errno_t (*read_blocks)(ddf_fun_t *, uint64_t, size_t, void *);
	errno_t (*write_blocks)(ddf_fun_t *, uint64_t, size_t, void *);
} ahci_iface_t;

#endif

/**
 * @}
 */
