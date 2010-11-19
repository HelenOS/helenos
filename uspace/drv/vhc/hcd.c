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
/** @file
 * @brief Virtual host controller driver.
 */

#include <devmap.h>
#include <ipc/ipc.h>
#include <async.h>
#include <unistd.h>
#include <stdlib.h>
#include <sysinfo.h>
#include <stdio.h>
#include <errno.h>
#include <str_error.h>
#include <driver.h>

#include <usb/hcd.h>
#include "vhcd.h"
#include "hc.h"
#include "devices.h"
#include "hub.h"
#include "conn.h"

static int vhc_count = 0;
static int vhc_add_device(usb_hc_device_t *dev)
{
	printf("%s: new device registered.\n", NAME);
	/*
	 * Currently, we know how to simulate only single HC.
	 */
	if (vhc_count > 0) {
		return ELIMIT;
	}

	vhc_count++;

	dev->transfer_ops = &vhc_transfer_ops;
	/* Fail because of bug in libusb that caused devman to hang. */
	return ENOTSUP;
}

static usb_hc_driver_t vhc_driver = {
	.name = NAME,
	.add_hc = &vhc_add_device
};

int main(int argc, char * argv[])
{	
	/*
	 * For debugging purpose to enable viewing the output
	 * within devman tty.
	 */
	sleep(5);

	printf("%s: virtual USB host controller driver.\n", NAME);

	return usb_hcd_main(&vhc_driver);
}


/**
 * @}
 */
