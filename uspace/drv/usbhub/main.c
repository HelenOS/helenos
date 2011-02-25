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

/** @addtogroup drvusbhub
 * @{
 */

#include <ddf/driver.h>
#include <errno.h>
#include <async.h>
#include <stdio.h>

#include "usbhub.h"
#include "usbhub_private.h"


usb_general_list_t usb_hub_list;
fibril_mutex_t usb_hub_list_lock;

static driver_ops_t hub_driver_ops = {
	.add_device = usb_add_hub_device,
};

static driver_t hub_driver = {
	.name = "usbhub",
	.driver_ops = &hub_driver_ops
};

int main(int argc, char *argv[])
{
	usb_log_enable(USB_LOG_LEVEL_DEBUG, NAME);
	dprintf(USB_LOG_LEVEL_INFO, "starting hub driver");

	//this is probably not needed anymore
	fibril_mutex_initialize(&usb_hub_list_lock);
	fibril_mutex_lock(&usb_hub_list_lock);
	usb_lst_init(&usb_hub_list);
	fibril_mutex_unlock(&usb_hub_list_lock);
	
	return ddf_driver_main(&hub_driver);
}

/**
 * @}
 */

