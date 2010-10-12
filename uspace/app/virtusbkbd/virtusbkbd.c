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

/** @addtogroup usb
 * @{
 */
/**
 * @file
 * @brief Virtual USB keyboard.
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <vfs/vfs.h>
#include <fcntl.h>
#include <errno.h>
#include <str_error.h>
#include <bool.h>
#include <async.h>

#include <usb/hcd.h>
#include <usb/device.h>
#include <usb/hid.h>
#include <usbvirt/device.h>
#include <usbvirt/hub.h>
#include <usbvirt/ids.h>

#include "report.h"

#define LOOPS 5
#define NAME "virt-usb-kbd"

#define DEV_HCD_NAME "hcd-virt"

#define __QUOTEME(x) #x
#define _QUOTEME(x) __QUOTEME(x)

#define VERBOSE_EXEC(cmd, fmt, ...) \
	(printf("%s: %s" fmt "\n", NAME, _QUOTEME(cmd), __VA_ARGS__), cmd(__VA_ARGS__))

static int on_incoming_data(struct usbvirt_device *dev,
    usb_endpoint_t endpoint, void *buffer, size_t size)
{
	printf("%s: ignoring incomming data to endpoint %d\n", NAME, endpoint);
	
	return EOK;
}

static int on_class_request(struct usbvirt_device *dev,
    usb_device_request_setup_packet_t *request, uint8_t *data)
{
	printf("%s: class request (%d)\n", NAME, (int) request->request);
	
	return EOK;
}

static usb_standard_device_descriptor_t std_descriptor = {
	.length = sizeof(usb_standard_device_descriptor_t),
	.descriptor_type = 1,
	.usb_spec_version = 0x110,
	.device_class = 0x03,
	.device_subclass = 0,
	.device_protocol = 0,
	.max_packet_size = 64,
	.configuration_count = 1
};

/** Keyboard callbacks.
 * We abuse the fact that static variables are zero-filled.
 */
static usbvirt_device_ops_t keyboard_ops = {
	.on_devreq_class = on_class_request,
	.on_data = on_incoming_data
};

/** Keyboard device.
 * Rest of the items will be initialized later.
 */
static usbvirt_device_t keyboard_dev = {
	.ops = &keyboard_ops,
	.standard_descriptor = &std_descriptor,
	.device_id_ = USBVIRT_DEV_KEYBOARD_ID
};


static void fibril_sleep(size_t sec)
{
	while (sec-- > 0) {
		async_usleep(1000*1000);
	}
}


int main(int argc, char * argv[])
{
	printf("Dump of report descriptor (%u bytes):\n", report_descriptor_size);
	size_t i;
	for (i = 0; i < report_descriptor_size; i++) {
		printf("  0x%02X", report_descriptor[i]);
		if (((i > 0) && (((i+1) % 10) == 0))
		    || (i + 1 == report_descriptor_size)) {
			printf("\n");
		}
	}
	
	
	int rc = usbvirt_connect(&keyboard_dev, DEV_HCD_NAME);
	if (rc != EOK) {
		printf("%s: Unable to start comunication with VHCD at usb://%s (%s).\n",
		    NAME, DEV_HCD_NAME, str_error(rc));
		return rc;
	}
	
	
	for (i = 0; i < LOOPS; i++) {
		size_t size = 5;
		char *data = (char *) "Hullo, World!";
		
		if (i > 0) {
			fibril_sleep(2);
		}
		
		printf("%s: Will send data to VHCD...\n", NAME);
		int rc = keyboard_dev.send_data(&keyboard_dev, 0, data, size);
		printf("%s:   ...data sent (%s).\n", NAME, str_error(rc));
	}
	
	fibril_sleep(1);
	printf("%s: Terminating...\n", NAME);
	
	usbvirt_disconnect();
	
	return 0;
}


/** @}
 */
