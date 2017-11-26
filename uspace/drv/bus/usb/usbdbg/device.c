/*
 * Copyright (c) 2017 Petr Manek
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

/** @addtogroup drvusbdbg
 * @{
 */
/**
 * @file
 * Code for managing debug device structures.
 */
#include <errno.h>
#include <usb/debug.h>

#include "device.h"

#define NAME "usbdbg"

static int device_init(usb_dbg_dev_t *dev)
{
	// TODO: allocate data structures, set up stuffs

	return EOK;
}

static void device_fini(usb_dbg_dev_t *dev)
{
	// TODO: tear down data structures
}

int usb_dbg_dev_create(usb_device_t *dev, usb_dbg_dev_t **out_dbg_dev)
{
	assert(dev);
	assert(out_dbg_dev);

	usb_dbg_dev_t *dbg_dev = usb_device_data_alloc(dev, sizeof(usb_dbg_dev_t));
	if (!dbg_dev)
		return ENOMEM;

	dbg_dev->usb_dev = dev;

	int err;
	if ((err = device_init(dbg_dev)))
		goto err_init;

	*out_dbg_dev = dbg_dev;
	return EOK;

err_init:
	/* There is no usb_device_data_free. */
	return err;
}

void usb_dbg_dev_destroy(usb_dbg_dev_t *dev)
{
	assert(dev);

	device_fini(dev);
	/* There is no usb_device_data_free. */
}

/**
 * @}
 */
