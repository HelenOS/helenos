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

#include <usb/usb.h>
#include <usb/descriptor.h>
#include <usb/classes/hid.h>
#include <usbvirt/device.h>
#include <usbvirt/hub.h>

#include "kbdconfig.h"
#include "keys.h"
#include "stdreq.h"

#define LOOPS 5
#define NAME "virt-usb-kbd"


#define __QUOTEME(x) #x
#define _QUOTEME(x) __QUOTEME(x)

#define VERBOSE_EXEC(cmd, fmt, ...) \
	(printf("%s: %s" fmt "\n", NAME, _QUOTEME(cmd), __VA_ARGS__), cmd(__VA_ARGS__))

kb_status_t status;

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

static int on_request_for_data(struct usbvirt_device *dev,
    usb_endpoint_t endpoint, void *buffer, size_t size, size_t *actual_size)
{
	if (size < 2 + KB_MAX_KEYS_AT_ONCE) {
		return EINVAL;
	}
	
	*actual_size = 2 + KB_MAX_KEYS_AT_ONCE;
	
	uint8_t data[2 + KB_MAX_KEYS_AT_ONCE];
	data[0] = status.modifiers;
	data[1] = 0;
	
	size_t i;
	for (i = 0; i < KB_MAX_KEYS_AT_ONCE; i++) {
		data[2 + i] = status.pressed_keys[i];
	}
	
	memcpy(buffer, &data, *actual_size);
	
	return EOK;
}


/** Keyboard callbacks.
 * We abuse the fact that static variables are zero-filled.
 */
static usbvirt_device_ops_t keyboard_ops = {
	.on_standard_request[USB_DEVREQ_GET_DESCRIPTOR]
	    = stdreq_on_get_descriptor,
	.on_class_device_request = on_class_request,
	.on_data = on_incoming_data,
	.on_data_request = on_request_for_data
};

usbvirt_device_configuration_extras_t extra_descriptors[] = {
	{
		.data = (uint8_t *) &std_interface_descriptor,
		.length = sizeof(std_interface_descriptor)
	},
	{
		.data = (uint8_t *) &hid_descriptor,
		.length = sizeof(hid_descriptor)
	},
	{
		.data = (uint8_t *) &endpoint_descriptor,
		.length = sizeof(endpoint_descriptor)
	}
};

/** Keyboard configuration. */
usbvirt_device_configuration_t configuration = {
	.descriptor = &std_configuration_descriptor,
	.extra = extra_descriptors,
	.extra_count = sizeof(extra_descriptors)/sizeof(extra_descriptors[0])
};

/** Keyboard standard descriptors. */
usbvirt_descriptors_t descriptors = {
	.device = &std_device_descriptor,
	.configuration = &configuration,
	.configuration_count = 1,
};

/** Keyboard device.
 * Rest of the items will be initialized later.
 */
static usbvirt_device_t keyboard_dev = {
	.ops = &keyboard_ops,
	.descriptors = &descriptors,
	.name = "keyboard"
};


static void fibril_sleep(size_t sec)
{
	while (sec-- > 0) {
		async_usleep(1000*1000);
	}
}


/** Callback when keyboard status changed.
 *
 * @param status Current keyboard status.
 */
static void on_keyboard_change(kb_status_t *status)
{
	printf("%s: Current keyboard status: %08hhb", NAME, status->modifiers);
	size_t i;
	for (i = 0; i < KB_MAX_KEYS_AT_ONCE; i++) {
		printf(" 0x%02X", (int)status->pressed_keys[i]);
	}
	printf("\n");
	
	fibril_sleep(1);
}

/** Simulated keyboard events. */
static kb_event_t keyboard_events[] = {
	/* Switch to VT6 (Alt+F6) */
	M_DOWN(KB_MOD_LEFT_ALT),
	K_PRESS(KB_KEY_F6),
	M_UP(KB_MOD_LEFT_ALT),
	/* Type the word 'Hello' */
	M_DOWN(KB_MOD_LEFT_SHIFT),
	K_PRESS(KB_KEY_H),
	M_UP(KB_MOD_LEFT_SHIFT),
	K_PRESS(KB_KEY_E),
	K_PRESS(KB_KEY_L),
	K_PRESS(KB_KEY_L),
	K_PRESS(KB_KEY_O)
};
static size_t keyboard_events_count =
    sizeof(keyboard_events)/sizeof(keyboard_events[0]);



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
	
	kb_init(&status);
	
	
	int rc = usbvirt_connect(&keyboard_dev);
	if (rc != EOK) {
		printf("%s: Unable to start communication with VHCD (%s).\n",
		    NAME, str_error(rc));
		return rc;
	}
	
	printf("%s: Simulating keyboard events...\n", NAME);
	kb_process_events(&status, keyboard_events, keyboard_events_count,
	    on_keyboard_change);
	
	printf("%s: Terminating...\n", NAME);
	
	usbvirt_disconnect(&keyboard_dev);
	
	return 0;
}


/** @}
 */
