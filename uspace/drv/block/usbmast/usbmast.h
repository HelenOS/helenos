/*
 * SPDX-FileCopyrightText: 2011 Jiri Svoboda
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup drvusbmast
 * @{
 */
/** @file
 * USB mass storage commands.
 */

#ifndef USBMAST_H_
#define USBMAST_H_

#include <bd_srv.h>
#include <stddef.h>
#include <stdint.h>
#include <usb/usb.h>

/** Mass storage device. */
typedef struct usbmast_dev {
	/** USB device */
	usb_device_t *usb_dev;
	/** Number of LUNs */
	unsigned lun_count;
	/** LUN functions */
	ddf_fun_t **luns;
	/** Data read pipe */
	usb_pipe_t *bulk_in_pipe;
	/** Data write pipe */
	usb_pipe_t *bulk_out_pipe;
} usbmast_dev_t;

/** Mass storage function.
 *
 * Serves as soft state for function/LUN.
 */
typedef struct {
	/** Mass storage device the function belongs to */
	usbmast_dev_t *mdev;
	/** DDF function */
	ddf_fun_t *ddf_fun;
	/** LUN */
	unsigned lun;
	/** Total number of blocks */
	uint64_t nblocks;
	/** Block size in bytes */
	size_t block_size;
	/** Block device service structure */
	bd_srvs_t bds;
} usbmast_fun_t;

#endif

/**
 * @}
 */
