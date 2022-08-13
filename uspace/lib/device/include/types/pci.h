/*
 * SPDX-FileCopyrightText: 2019 Jiri Svoboda
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup libdevice
 * @{
 */
/** @file
 */

#ifndef LIBDEVICE_TYPES_PCI_H
#define LIBDEVICE_TYPES_PCI_H

#include <async.h>
#include <ipc/devman.h>
#include <stdint.h>

/** PCI device information */
typedef struct {
	/** Devman function handle */
	devman_handle_t dev_handle;
	/** Bus number */
	uint8_t bus_num;
	/** Device number */
	uint8_t dev_num;
	/** Function number */
	uint8_t fn_num;
	/** Vendor ID */
	uint16_t vendor_id;
	/** Device ID */
	uint16_t device_id;
} pci_dev_info_t;

/** PCI service */
typedef struct {
	/** Session with PCI control service */
	async_sess_t *sess;
} pci_t;

#endif

/** @}
 */
