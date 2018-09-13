/*
 * Copyright (c) 2011 Jiri Svoboda
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
