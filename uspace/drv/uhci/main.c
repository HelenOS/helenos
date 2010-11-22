/*
 * Copyright (c) 2010 Vojtech Horky
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
#include <usb/hcdhubd.h>
#include <errno.h>

static int enqueue_transfer_out(usb_hc_device_t *hc,
    usb_hcd_attached_device_info_t *dev, usb_hc_endpoint_info_t *endpoint,
    void *buffer, size_t size,
    usb_hcd_transfer_callback_out_t callback, void *arg)
{
	printf("UHCI: transfer OUT [%d.%d (%s); %u]\n",
	    dev->address, endpoint->endpoint,
	    usb_str_transfer_type(endpoint->transfer_type),
	    size);
	return ENOTSUP;
}

static int enqueue_transfer_setup(usb_hc_device_t *hc,
    usb_hcd_attached_device_info_t *dev, usb_hc_endpoint_info_t *endpoint,
    void *buffer, size_t size,
    usb_hcd_transfer_callback_out_t callback, void *arg)
{
	printf("UHCI: transfer SETUP [%d.%d (%s); %u]\n",
	    dev->address, endpoint->endpoint,
	    usb_str_transfer_type(endpoint->transfer_type),
	    size);
	return ENOTSUP;
}

static int enqueue_transfer_in(usb_hc_device_t *hc,
    usb_hcd_attached_device_info_t *dev, usb_hc_endpoint_info_t *endpoint,
    void *buffer, size_t size,
    usb_hcd_transfer_callback_in_t callback, void *arg)
{
	printf("UHCI: transfer IN [%d.%d (%s); %u]\n",
	    dev->address, endpoint->endpoint,
	    usb_str_transfer_type(endpoint->transfer_type),
	    size);
	return ENOTSUP;
}

static usb_hcd_transfer_ops_t uhci_transfer_ops = {
	.transfer_out = enqueue_transfer_out,
	.transfer_in = enqueue_transfer_in,
	.transfer_setup = enqueue_transfer_setup
};

static int uhci_add_hc(usb_hc_device_t *device)
{
	device->transfer_ops = &uhci_transfer_ops;

	/*
	 * We need to announce the presence of our root hub.
	 * Commented out until the problem which causes the whole task to
	 * block is solved.
	 */
	//usb_hcd_add_root_hub(device);

	return EOK;
}

usb_hc_driver_t uhci_driver = {
	.name = "uhci",
	.add_hc = uhci_add_hc
};

int main(int argc, char *argv[])
{
	/*
	 * Do some global initializations.
	 */

	return usb_hcd_main(&uhci_driver);
}
