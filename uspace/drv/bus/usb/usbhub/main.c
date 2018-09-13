/*
 * Copyright (c) 2010 Vojtech Horky
 * Copyright (c) 2011 Jan Vesely
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

/** @addtogroup drvusbhub
 * @{
 */

#include <ddf/driver.h>
#include <errno.h>
#include <async.h>
#include <stdio.h>
#include <io/logctl.h>

#include <usb/dev/driver.h>
#include <usb/classes/classes.h>
#include <usb/debug.h>

#include "usbhub.h"

/** USB hub driver operations. */
static const usb_driver_ops_t usb_hub_driver_ops = {
	.device_add = usb_hub_device_add,
	.device_remove = usb_hub_device_remove,
	.device_gone = usb_hub_device_gone,
};

/** Static usb hub driver information. */
static const usb_driver_t usb_hub_driver = {
	.name = NAME,
	.ops = &usb_hub_driver_ops,
	.endpoints = usb_hub_endpoints
};

int main(int argc, char *argv[])
{
	printf(NAME ": HelenOS USB hub driver.\n");
	log_init(NAME);
	logctl_set_log_level(NAME, LVL_NOTE);
	return usb_driver_main(&usb_hub_driver);
}

/**
 * @}
 */
