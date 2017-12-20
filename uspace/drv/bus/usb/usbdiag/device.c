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

/** @addtogroup drvusbdiag
 * @{
 */
/**
 * @file
 * Code for managing debug device structures.
 */
#include <errno.h>
#include <str_error.h>
#include <macros.h>
#include <usb/debug.h>
#include <usbdiag_iface.h>

#include "device.h"
#include "tests.h"

#define NAME "usbdiag"

#define TRANSLATE_FUNC_NAME(fun) translate_##fun
#define TRANSLATE_FUNC(fun) \
	static int TRANSLATE_FUNC_NAME(fun)(ddf_fun_t *f, int cycles, size_t size)\
	{\
		usb_diag_dev_t *dev = ddf_fun_to_usb_diag_dev(f);\
		return fun(dev, cycles, size);\
	}

TRANSLATE_FUNC(usb_diag_stress_bulk_out)
TRANSLATE_FUNC(usb_diag_stress_bulk_in)

static usbdiag_iface_t diag_interface = {
	.stress_bulk_out = TRANSLATE_FUNC_NAME(usb_diag_stress_bulk_out),
	.stress_bulk_in = TRANSLATE_FUNC_NAME(usb_diag_stress_bulk_in)
};

#undef TRANSLATE_FUNC_NAME
#undef TRANSLATE_FUNC

static ddf_dev_ops_t diag_ops = {
	.interfaces[USBDIAG_DEV_IFACE] = &diag_interface
};

static int device_init(usb_diag_dev_t *dev)
{
	int rc;
	ddf_fun_t *fun = usb_device_ddf_fun_create(dev->usb_dev, fun_exposed, "tmon");
	if (!fun) {
		rc = ENOMEM;
		goto err;
	}

	ddf_fun_set_ops(fun, &diag_ops);
	dev->fun = fun;

#define _MAP_EP(target, ep_no) do {\
	usb_endpoint_mapping_t *epm = usb_device_get_mapped_ep(dev->usb_dev, USB_DIAG_EP_##ep_no);\
	if (!epm || !epm->present) {\
		usb_log_error("Failed to map endpoint: " #ep_no ".\n");\
		rc = ENOENT;\
		goto err_fun;\
	}\
	target = &epm->pipe;\
	} while (0);

	_MAP_EP(dev->intr_in, INTR_IN);
	_MAP_EP(dev->intr_out, INTR_OUT);
	_MAP_EP(dev->bulk_in, BULK_IN);
	_MAP_EP(dev->bulk_out, BULK_OUT);
	_MAP_EP(dev->isoch_in, ISOCH_IN);
	_MAP_EP(dev->isoch_out, ISOCH_OUT);

#undef _MAP_EP

	return EOK;

err_fun:
	ddf_fun_destroy(fun);
err:
	return rc;
}

static void device_fini(usb_diag_dev_t *dev)
{
	ddf_fun_destroy(dev->fun);
}

int usb_diag_dev_create(usb_device_t *dev, usb_diag_dev_t **out_diag_dev)
{
	assert(dev);
	assert(out_diag_dev);

	usb_diag_dev_t *diag_dev = usb_device_data_alloc(dev, sizeof(usb_diag_dev_t));
	if (!diag_dev)
		return ENOMEM;

	diag_dev->usb_dev = dev;

	int err;
	if ((err = device_init(diag_dev)))
		goto err_init;

	*out_diag_dev = diag_dev;
	return EOK;

err_init:
	/* There is no usb_device_data_free. */
	return err;
}

void usb_diag_dev_destroy(usb_diag_dev_t *dev)
{
	assert(dev);

	device_fini(dev);
	/* There is no usb_device_data_free. */
}

/**
 * @}
 */
