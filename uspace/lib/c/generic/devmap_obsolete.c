/*
 * Copyright (c) 2007 Josef Cejka
 * Copyright (c) 2009 Jiri Svoboda
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

#include <str.h>
#include <ipc/services.h>
#include <ns.h>
#include <ns_obsolete.h>
#include <ipc/devmap.h>
#include <devmap_obsolete.h>
#include <async.h>
#include <async_obsolete.h>
#include <errno.h>
#include <malloc.h>
#include <bool.h>

static int devmap_phone_driver = -1;
static int devmap_phone_client = -1;

/** Get phone to device mapper task. */
int devmap_obsolete_get_phone(devmap_interface_t iface, unsigned int flags)
{
	switch (iface) {
	case DEVMAP_DRIVER:
		if (devmap_phone_driver >= 0)
			return devmap_phone_driver;
		
		if (flags & IPC_FLAG_BLOCKING)
			devmap_phone_driver = service_obsolete_connect_blocking(SERVICE_DEVMAP,
			    DEVMAP_DRIVER, 0);
		else
			devmap_phone_driver = service_obsolete_connect(SERVICE_DEVMAP,
			    DEVMAP_DRIVER, 0);
		
		return devmap_phone_driver;
	case DEVMAP_CLIENT:
		if (devmap_phone_client >= 0)
			return devmap_phone_client;
		
		if (flags & IPC_FLAG_BLOCKING)
			devmap_phone_client = service_obsolete_connect_blocking(SERVICE_DEVMAP,
			    DEVMAP_CLIENT, 0);
		else
			devmap_phone_client = service_obsolete_connect(SERVICE_DEVMAP,
			    DEVMAP_CLIENT, 0);
		
		return devmap_phone_client;
	default:
		return -1;
	}
}

void devmap_obsolete_hangup_phone(devmap_interface_t iface)
{
	switch (iface) {
	case DEVMAP_DRIVER:
		if (devmap_phone_driver >= 0) {
			async_obsolete_hangup(devmap_phone_driver);
			devmap_phone_driver = -1;
		}
		break;
	case DEVMAP_CLIENT:
		if (devmap_phone_client >= 0) {
			async_obsolete_hangup(devmap_phone_client);
			devmap_phone_client = -1;
		}
		break;
	default:
		break;
	}
}

int devmap_obsolete_device_connect(devmap_handle_t handle, unsigned int flags)
{
	int phone;
	
	if (flags & IPC_FLAG_BLOCKING) {
		phone = service_obsolete_connect_blocking(SERVICE_DEVMAP,
		    DEVMAP_CONNECT_TO_DEVICE, handle);
	} else {
		phone = service_obsolete_connect(SERVICE_DEVMAP,
		    DEVMAP_CONNECT_TO_DEVICE, handle);
	}
	
	return phone;
}

/** @}
 */
