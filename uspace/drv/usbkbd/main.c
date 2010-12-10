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
#include <usb/usbdrv.h>
#include <driver.h>
#include <ipc/driver.h>
#include <errno.h>
#include <fibril.h>
#include <usb/classes/hid.h>

#define BUFFER_SIZE 32
#define NAME "usbkbd"

static const usb_endpoint_t CONTROL_EP = 0;

static usb_hid_dev_kbd_t *usbkbd_init_device(device_t *dev)
{
	usb_hid_dev_kbd_t *kbd_dev = (usb_hid_dev_kbd_t *)malloc(
			sizeof(usb_hid_dev_kbd_t));

	if (kbd_dev == NULL) {
		fprintf(stderr, NAME ": No memory!\n");
		return NULL;
	}

	kbd_dev->device = dev;

	// get phone to my HC and save it as my parent's phone
	// TODO: maybe not a good idea if DDF will use parent_phone
	kbd_dev->device->parent_phone = usb_drv_hc_connect(dev, 0);

	kbd_dev->address = usb_drv_get_my_address(dev->parent_phone,
	    dev);

	// doesn't matter now that we have no address
//	if (kbd_dev->address < 0) {
//		fprintf(stderr, NAME ": No device address!\n");
//		free(kbd_dev);
//		return NULL;
//	}

	// default endpoint
	kbd_dev->default_ep = CONTROL_EP;

	// TODO: get descriptors

	// TODO: parse descriptors and save endpoints

	return kbd_dev;
}

static void usbkbd_process_interrupt_in(usb_hid_dev_kbd_t *kbd_dev,
					char *buffer, size_t actual_size)
{
	/*
	 * here, the parser will be called, probably with some callbacks
	 * now only take last 6 bytes and process, i.e. send to kbd
	 */
}

static void usbkbd_poll_keyboard(usb_hid_dev_kbd_t *kbd_dev)
{
	int rc;
	usb_handle_t handle;
	char buffer[BUFFER_SIZE];
	size_t actual_size;
	//usb_endpoint_t poll_endpoint = 1;

//	usb_address_t my_address = usb_drv_get_my_address(dev->parent_phone,
//	    dev);
//	if (my_address < 0) {
//		return;
//	}

	usb_target_t poll_target = {
		.address = kbd_dev->address,
		.endpoint = kbd_dev->default_ep
	};

	while (true) {
		rc = usb_drv_async_interrupt_in(kbd_dev->device->parent_phone,
		    poll_target, buffer, BUFFER_SIZE, &actual_size, &handle);

		if (rc != EOK) {
			continue;
		}

		rc = usb_drv_async_wait_for(handle);
		if (rc != EOK) {
			continue;
		}

		/*
		 * If the keyboard answered with NAK, it returned no data.
		 * This implies that no change happened since last query.
		 */
		if (actual_size == 0) {
			continue;
		}

		/*
		 * TODO: Process pressed keys.
		 */
		usbkbd_process_interrupt_in(kbd_dev, buffer, actual_size);
	}

	// not reached
	assert(0);
}

static int usbkbd_fibril_device(void *arg)
{
	printf("!!! USB device fibril\n");

	if (arg == NULL) {
		printf("No device!\n");
		return -1;
	}

	device_t *dev = (device_t *)arg;

	// initialize device (get and process descriptors, get address, etc.)
	usb_hid_dev_kbd_t *kbd_dev = usbkbd_init_device(dev);

	usbkbd_poll_keyboard(kbd_dev);

	return EOK;
}

static int usbkbd_add_device(device_t *dev)
{
	/* For now, fail immediately. */
	//return ENOTSUP;

	/*
	 * When everything is okay, connect to "our" HC.
	 *
	 * Not supported yet, skip..
	 */
//	int phone = usb_drv_hc_connect(dev, 0);
//	if (phone < 0) {
//		/*
//		 * Connecting to HC failed, roll-back and announce
//		 * failure.
//		 */
//		return phone;
//	}

//	dev->parent_phone = phone;

	/*
	 * Create new fibril for handling this keyboard
	 */
	fid_t fid = fibril_create(usbkbd_fibril_device, dev);
	if (fid == 0) {
		printf("%s: failed to start fibril for HID device\n", NAME);
		return ENOMEM;
	}
	fibril_add_ready(fid);

	/*
	 * Hurrah, device is initialized.
	 */
	return EOK;
}

static driver_ops_t kbd_driver_ops = {
	.add_device = usbkbd_add_device,
};

static driver_t kbd_driver = {
	.name = NAME,
	.driver_ops = &kbd_driver_ops
};

int main(int argc, char *argv[])
{
	return driver_main(&kbd_driver);
}
