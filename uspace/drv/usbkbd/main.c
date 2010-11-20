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
#include <errno.h>

#define BUFFER_SIZE 32

/* Call this periodically to check keyboard status changes. */
static void poll_keyboard(device_t *dev)
{
	int rc;
	usb_handle_t handle;
	char buffer[BUFFER_SIZE];
	size_t actual_size;
	usb_endpoint_t poll_endpoint = 1;

	usb_address_t my_address = usb_drv_get_my_address(dev->parent_phone,
	    dev);
	if (my_address < 0) {
		return;
	}

	usb_target_t poll_target = {
		.address = my_address,
		.endpoint = poll_endpoint
	};

	rc = usb_drv_async_interrupt_in(dev->parent_phone, poll_target,
	    buffer, BUFFER_SIZE, &actual_size, &handle);
	if (rc != EOK) {
		return;
	}

	rc = usb_drv_async_wait_for(handle);
	if (rc != EOK) {
		return;
	}

	/*
	 * If the keyboard answered with NAK, it returned no data.
	 * This implies that no change happened since last query.
	 */
	if (actual_size == 0) {
		return;
	}

	/*
	 * Process pressed keys.
	 */
}

static int add_kbd_device(device_t *dev)
{
	/* For now, fail immediately. */
	return ENOTSUP;

	/*
	 * When everything is okay, connect to "our" HC.
	 */
	int phone = usb_drv_hc_connect(dev, 0);
	if (phone < 0) {
		/*
		 * Connecting to HC failed, roll-back and announce
		 * failure.
		 */
		return phone;
	}

	dev->parent_phone = phone;

	/*
	 * Just for fun ;-).
	 */
	poll_keyboard(dev);

	/*
	 * Hurrah, device is initialized.
	 */
	return EOK;
}

static driver_ops_t kbd_driver_ops = {
	.add_device = add_kbd_device,
};

static driver_t kbd_driver = {
	.name = "usbkbd",
	.driver_ops = &kbd_driver_ops
};

int main(int argc, char *argv[])
{
	return driver_main(&kbd_driver);
}
