/*
 * Copyright (c) 2010 Vojtech Horky, Jan Vesely
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
#include <driver.h>
#include <errno.h>
#include <str_error.h>
#include <usb_iface.h>

#include "debug.h"
//#include "iface.h"
#include "root_hub.h"

static int usb_iface_get_hc_handle(device_t *dev, devman_handle_t *handle)
{
	/* This shall be called only for the UHCI itself. */
	assert(dev);
	assert(dev->driver_data);
	*handle = ((uhci_root_hub_t*)dev->driver_data)->hc_handle;

	return EOK;
}

static usb_iface_t uhci_rh_usb_iface = {
	.get_hc_handle = usb_iface_get_hc_handle
};

static device_ops_t uhci_rh_ops = {
	.interfaces[USB_DEV_IFACE] = &uhci_rh_usb_iface,
};

static int uhci_rh_add_device(device_t *device)
{
	if (!device)
		return ENOTSUP;

	uhci_print_info("%s called device %d\n", __FUNCTION__, device->handle);
	device->ops = &uhci_rh_ops;

	uhci_root_hub_t *rh = malloc(sizeof(uhci_root_hub_t));
	if (!rh) {
		return ENOMEM;
	}
	int ret = uhci_root_hub_init(rh, (void*)0xc030, 4, device);
	if (ret != EOK) {
		free(rh);
		return ret;
	}
	device->driver_data = rh;
	return EOK;
}

static driver_ops_t uhci_rh_driver_ops = {
	.add_device = uhci_rh_add_device,
};

static driver_t uhci_rh_driver = {
	.name = NAME,
	.driver_ops = &uhci_rh_driver_ops
};

int main(int argc, char *argv[])
{
	/*
	 * Do some global initializations.
	 */
	usb_dprintf_enable(NAME, DEBUG_LEVEL_INFO);

	return driver_main(&uhci_rh_driver);
}
