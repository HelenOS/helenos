/*
 * Copyright (c) 2011 Vojtech Horky
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

/** @addtogroup libusb
 * @{
 */
/** @file
 * USB device driver framework.
 */
#ifndef LIBUSB_DEVDRV_H_
#define LIBUSB_DEVDRV_H_

#include <usb/pipes.h>

/** USB device structure. */
typedef struct {
	/** The default control pipe. */
	usb_pipe_t ctrl_pipe;
	/** Other endpoint pipes.
	 * This is an array of other endpoint pipes in the same order as
	 * in usb_driver_t.
	 */
	usb_endpoint_mapping_t *pipes;
	/** Current interface.
	 * Usually, drivers operate on single interface only.
	 * This item contains the value of the interface or -1 for any.
	 */
	int interface_no;
	/** Generic DDF device backing this one. */
	ddf_dev_t *ddf_dev;
	/** Custom driver data.
	 * Do not use the entry in generic device, that is already used
	 * by the framework.
	 */
	void *driver_data;

	/** Connection backing the pipes.
	 * Typically, you will not need to use this attribute at all.
	 */
	usb_device_connection_t wire;
} usb_device_t;

/** USB driver ops. */
typedef struct {
	/** Callback when new device is about to be controlled by the driver. */
	int (*add_device)(usb_device_t *);
} usb_driver_ops_t;

/** USB driver structure. */
typedef struct {
	/** Driver name.
	 * This name is copied to the generic driver name and must be exactly
	 * the same as the directory name where the driver executable resides.
	 */
	const char *name;
	/** Expected endpoints description, excluding default control endpoint.
	 *
	 * It MUST be of size expected_enpoints_count(excluding default ctrl) + 1
	 * where the last record MUST BE NULL, otherwise catastrophic things may
	 * happen.
	 */
	usb_endpoint_description_t **endpoints;
	/** Driver ops. */
	usb_driver_ops_t *ops;
} usb_driver_t;

int usb_driver_main(usb_driver_t *);

typedef bool (*usb_polling_callback_t)(usb_device_t *,
    uint8_t *, size_t, void *);
typedef void (*usb_polling_terminted_callback_t)(usb_device_t *, bool, void *);


int usb_device_auto_poll(usb_device_t *, size_t,
    usb_polling_callback_t, size_t, usb_polling_terminted_callback_t, void *);

#endif
/**
 * @}
 */
