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
#include <usb/virtdev.h>

#define LOOPS 5
#define NAME "virt-usb-kbd"

#define DEV_HCD_NAME "hcd-virt"

#define __QUOTEME(x) #x
#define _QUOTEME(x) __QUOTEME(x)

#define VERBOSE_EXEC(cmd, fmt, ...) \
	(printf("%s: %s" fmt "\n", NAME, _QUOTEME(cmd), __VA_ARGS__), cmd(__VA_ARGS__))

static void fibril_sleep(size_t sec)
{
	while (sec-- > 0) {
		async_usleep(1000*1000);
	}
}

static void on_data_from_host(usb_endpoint_t endpoint, void *buffer, size_t len)
{
	printf("%s: ignoring incomming data to endpoint %d\n", NAME, endpoint);
}

int main(int argc, char * argv[])
{
	int vhcd_phone = usb_virtdev_connect(DEV_HCD_NAME,
	    USB_VIRTDEV_KEYBOARD_ID, on_data_from_host);
	
	if (vhcd_phone < 0) {
		printf("%s: Unable to start comunication with VHCD at usb://%s (%s).\n",
		    NAME, DEV_HCD_NAME, str_error(vhcd_phone));
		return vhcd_phone;
	}
	
	
	
	size_t i;
	for (i = 0; i < LOOPS; i++) {
		size_t size = 5;
		char *data = (char *) "Hullo, World!";
		
		if (i > 0) {
			fibril_sleep(2);
		}
		
		printf("%s: Will send data to VHCD...\n", NAME);
		int rc = usb_virtdev_data_to_host(vhcd_phone, 0, data, size);
		printf("%s:   ...data sent (%s).\n", NAME, str_error(rc));
	}
	
	fibril_sleep(1);
	printf("%s: Terminating...\n", NAME);
	
	ipc_hangup(vhcd_phone);
	
	return 0;
}


/** @}
 */
