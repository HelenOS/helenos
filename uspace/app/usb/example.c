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
 * @brief Simple application that connects to USB/HCD.
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
#include <usb/devreq.h>

#define LOOPS 5
#define MAX_SIZE_RECEIVE 64
#define NAME "hcd-example"

#define DEV_HCD_NAME "hcd-virt"

#define __QUOTEME(x) #x
#define _QUOTEME(x) __QUOTEME(x)

#define VERBOSE_EXEC(cmd, fmt, ...) \
	(printf("%s: %s" fmt "\n", NAME, _QUOTEME(cmd), __VA_ARGS__), cmd(__VA_ARGS__))

#define EXEC2(cmd, fmt, ...) \
	do { \
		printf("%s: " fmt " = ", NAME, __VA_ARGS__); \
		fflush(stdout); \
		int _rc = cmd; \
		if (_rc != EOK) { \
			printf("E%d\n", _rc); \
			printf("%s: ... aborting.\n", NAME); \
			exit(_rc); \
		} \
		printf("EOK\n"); \
	} while (false)
#define EXEC(cmd, fmt, ...) \
	EXEC2(cmd(__VA_ARGS__), _QUOTEME(cmd) fmt, __VA_ARGS__)

static void fibril_sleep(size_t sec)
{
	while (sec-- > 0) {
		async_usleep(1000*1000);
	}
}

static void data_dump(uint8_t *data, size_t len)
{
	size_t i;
	for (i = 0; i < len; i++) {
		printf("  0x%02X", data[i]);
		if (((i > 0) && (((i+1) % 10) == 0))
		    || (i + 1 == len)) {
			printf("\n");
		}
	}
}

int main(int argc, char * argv[])
{
	int hcd_phone = usb_hcd_connect(DEV_HCD_NAME);
	if (hcd_phone < 0) {
		printf("%s: Unable to start comunication with HCD at usb://%s (%d: %s).\n",
		    NAME, DEV_HCD_NAME, hcd_phone, str_error(hcd_phone));
		return 1;
	}
	
	printf("%s: example communication with HCD\n", NAME);
	
	usb_target_t target = {0, 0};
	usb_handle_t handle;
	
	
	
	usb_device_request_setup_packet_t setup_packet = {
		.request_type = 0,
		.request = USB_DEVREQ_SET_ADDRESS,
		.index = 0,
		.length = 0,
	};
	setup_packet.value = 5;
	
	printf("\n%s: === setting device address to %d ===\n", NAME,
	    (int)setup_packet.value);
	EXEC2(usb_hcd_async_transfer_control_write_setup(hcd_phone, target,
	    &setup_packet, sizeof(setup_packet), &handle),
	    "usb_hcd_async_transfer_control_write_setup(%d, {%d:%d}, &data, %u, &h)",
	    hcd_phone, target.address, target.endpoint, sizeof(setup_packet));
	    
	EXEC(usb_hcd_async_wait_for, "(h=%x)", handle);
	
	EXEC2(usb_hcd_async_transfer_control_write_status(hcd_phone, target,
	    &handle),
	    "usb_hcd_async_transfer_control_write_status(%d, {%d:%d}, &h)",
	    hcd_phone, target.address, target.endpoint);
	
	EXEC(usb_hcd_async_wait_for, "(h=%x)", handle);
	
	target.address = setup_packet.value;
	
	
	printf("\n%s: === getting standard device descriptor ===\n", NAME);
	usb_device_request_setup_packet_t get_descriptor = {
		.request_type = 128,
		.request = USB_DEVREQ_GET_DESCRIPTOR,
		.index = 0,
		.length = MAX_SIZE_RECEIVE,
	};
	get_descriptor.value_low = 0;
	get_descriptor.value_high = 1;
	
	uint8_t descriptor[MAX_SIZE_RECEIVE];
	size_t descriptor_length;
	
	EXEC2(usb_hcd_async_transfer_control_read_setup(hcd_phone, target,
	    &get_descriptor, sizeof(get_descriptor), &handle),
	    "usb_hcd_async_transfer_control_read_setup(%d, {%d:%d}, &data, %u, &h)",
	    hcd_phone, target.address, target.endpoint, sizeof(get_descriptor));
	
	EXEC(usb_hcd_async_wait_for, "(h=%x)", handle);
	
	usb_handle_t data_handle;
	EXEC2(usb_hcd_async_transfer_control_read_data(hcd_phone, target,
	    descriptor, MAX_SIZE_RECEIVE, &descriptor_length, &data_handle),
	    "usb_hcd_async_transfer_control_read_data(%d, {%d:%d}, &data, %u, &len, &h2)",
	    hcd_phone, target.address, target.endpoint, MAX_SIZE_RECEIVE);
	
	EXEC2(usb_hcd_async_transfer_control_read_status(hcd_phone, target,
	    &handle),
	    "usb_hcd_async_transfer_control_read_status(%d, {%d:%d}, &h)",
	    hcd_phone, target.address, target.endpoint);
	
	EXEC(usb_hcd_async_wait_for, "(h=%x)", handle);
	EXEC(usb_hcd_async_wait_for, "(h2=%x)", data_handle);
	
	printf("%s: standard device descriptor dump (%dB):\n", NAME, descriptor_length);
	data_dump(descriptor, descriptor_length);
	
	fibril_sleep(1);

	printf("%s: exiting.\n", NAME);
	
	ipc_hangup(hcd_phone);
	
	return 0;
}


/** @}
 */
