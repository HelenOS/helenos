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

/** Pause between individual key-presses in seconds. */
#define KEY_PRESS_DELAY 2
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


/** Compares current and last status of pressed keys.
 *
 * @warning Has side-efect - changes status_last field.
 *
 * @param status_now Status now.
 * @param status_last Last status.
 * @param len Size of status.
 * @return Whether they are the same.
 */
static bool keypress_check_with_last_request(uint8_t *status_now,
    uint8_t *status_last, size_t len)
{
	bool same = true;
	size_t i;
	for (i = 0; i < len; i++) {
		if (status_now[i] != status_last[i]) {
			status_last[i] = status_now[i];
			same = false;
		}
	}
	return same;
}

static int on_request_for_data(struct usbvirt_device *dev,
    usb_endpoint_t endpoint, void *buffer, size_t size, size_t *actual_size)
{
	static uint8_t last_data[2 + KB_MAX_KEYS_AT_ONCE];

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
	
	if (keypress_check_with_last_request(data, last_data,
	    2 + KB_MAX_KEYS_AT_ONCE)) {
		*actual_size = 0;
		return EOK;
	}

	memcpy(buffer, &data, *actual_size);
	
	return EOK;
}

static usbvirt_control_transfer_handler_t endpoint_zero_handlers[] = {
	{
		.request_type = USBVIRT_MAKE_CONTROL_REQUEST_TYPE(
		    USB_DIRECTION_IN,
		    USBVIRT_REQUEST_TYPE_STANDARD,
		    USBVIRT_REQUEST_RECIPIENT_DEVICE),
		.request = USB_DEVREQ_GET_DESCRIPTOR,
		.name = "GetDescriptor",
		.callback = stdreq_on_get_descriptor
	},
	{
		.request_type = USBVIRT_MAKE_CONTROL_REQUEST_TYPE(
		    USB_DIRECTION_IN,
		    USBVIRT_REQUEST_TYPE_CLASS,
		    USBVIRT_REQUEST_RECIPIENT_DEVICE),
		.request = USB_DEVREQ_GET_DESCRIPTOR,
		.name = "GetDescriptor",
		.callback = stdreq_on_get_descriptor
	},
	USBVIRT_CONTROL_TRANSFER_HANDLER_LAST
};

/** Keyboard callbacks.
 * We abuse the fact that static variables are zero-filled.
 */
static usbvirt_device_ops_t keyboard_ops = {
	.control_transfer_handlers = endpoint_zero_handlers,
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
	.lib_debug_level = 3,
	.lib_debug_enabled_tags = USBVIRT_DEBUGTAG_ALL,
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
	printf("%s: Current keyboard status: %02hhx", NAME, status->modifiers);
	size_t i;
	for (i = 0; i < KB_MAX_KEYS_AT_ONCE; i++) {
		printf(" 0x%02X", (int)status->pressed_keys[i]);
	}
	printf("\n");
	
	fibril_sleep(KEY_PRESS_DELAY);
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
	printf("Dump of report descriptor (%zu bytes):\n", report_descriptor_size);
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
	fibril_sleep(10);
	//while (1) {
		kb_process_events(&status, keyboard_events, keyboard_events_count,
			on_keyboard_change);
	//}
	
	printf("%s: Terminating...\n", NAME);
	
	usbvirt_disconnect(&keyboard_dev);
	
	return 0;
}


/** @}
 */
