/*
 * Copyright (c) 2010 Vojtech Horky, Jan Vesely
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
#include <usb/hcdhubd.h>
#include <errno.h>

#include "debug.h"
#include "iface.h"
#include "name.h"
#include "uhci.h"


static device_ops_t uhci_ops = {
	.interfaces[USBHC_DEV_IFACE] = &uhci_iface,
};

static int uhci_add_device(device_t *device)
{
//	usb_dprintf(NAME, DEBUG, "uhci_add_device() called\n");
	uhci_print_info( "uhci_add_device() called\n" );
	device->ops = &uhci_ops;

	uhci_init( device, (void*)0xc020 );

	return EOK;
}

static driver_ops_t uhci_driver_ops = {
	.add_device = uhci_add_device,
};

static driver_t uhci_driver = {
	.name = NAME,
	.driver_ops = &uhci_driver_ops
};

int main(int argc, char *argv[])
{
	/*
	 * Do some global initializations.
	 */
	sleep( 5 );
	usb_dprintf_enable(NAME, DEBUG_LEVEL_MAX);

	return driver_main(&uhci_driver);
}
