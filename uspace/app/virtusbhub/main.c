/*
 * Copyright (c) 2011 Vojtech Horky
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

/** @addtogroup usbvirthub
 * @{
 */
/**
 * @file
 * @brief Virtual USB hub.
 */

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <str_error.h>
#include <bool.h>

#include <usb/usb.h>
#include <usb/descriptor.h>
#include <usb/classes/hub.h>
#include <usbvirt/device.h>
#include <usbvirt/hub.h>

#include "virthub.h"

#define NAME "vuh"

static usbvirt_device_t hub_device;

#define VERBOSE_SLEEP(sec, msg, ...) \
	do { \
		char _status[HUB_PORT_COUNT + 2]; \
		printf(NAME ": doing nothing for %zu seconds...\n", \
		    (size_t) (sec)); \
		fibril_sleep((sec)); \
		virthub_get_status(&hub_device, _status, HUB_PORT_COUNT + 1); \
		printf(NAME ": " msg " [%s]\n" #__VA_ARGS__, _status); \
	} while (0)

static void fibril_sleep(size_t sec)
{
	while (sec-- > 0) {
		async_usleep(1000*1000);
	}
}

static int dev1 = 1;

int main(int argc, char * argv[])
{
	int rc;

	printf(NAME ": virtual USB hub.\n");

	rc = virthub_init(&hub_device);
	if (rc != EOK) {
		printf(NAME ": Unable to start communication with VHCD (%s).\n",
		    str_error(rc));
		return rc;
	}
	
	while (true) {
		VERBOSE_SLEEP(8, "will pretend device plug-in...");
		virthub_connect_device(&hub_device, &dev1);

		VERBOSE_SLEEP(8, "will pretend device un-plug...");
		virthub_disconnect_device(&hub_device, &dev1);
	}

	usbvirt_disconnect(&hub_device);
	
	return 0;
}


/** @}
 */
