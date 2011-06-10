/*
 * Copyright (c) 2007 Josef Cejka
 * Copyright (c) 2009 Jiri Svoboda
 * Copyright (c) 2010 Lenka Trochtova
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

/** @addtogroup libc
 * @{
 */
/** @file
 */

#include <str.h>
#include <stdio.h>
#include <ipc/services.h>
#include <ipc/devman.h>
#include <devman_obsolete.h>
#include <async.h>
#include <async_obsolete.h>
#include <ns.h>
#include <ns_obsolete.h>
#include <fibril_synch.h>
#include <errno.h>
#include <malloc.h>
#include <bool.h>
#include <adt/list.h>

static int devman_phone_driver = -1;
static int devman_phone_client = -1;

static FIBRIL_MUTEX_INITIALIZE(devman_phone_mutex);

int devman_obsolete_get_phone(devman_interface_t iface, unsigned int flags)
{
	switch (iface) {
	case DEVMAN_DRIVER:
		fibril_mutex_lock(&devman_phone_mutex);
		if (devman_phone_driver >= 0) {
			fibril_mutex_unlock(&devman_phone_mutex);
			return devman_phone_driver;
		}
		
		if (flags & IPC_FLAG_BLOCKING)
			devman_phone_driver = service_obsolete_connect_blocking(
			    SERVICE_DEVMAN, DEVMAN_DRIVER, 0);
		else
			devman_phone_driver = service_obsolete_connect(SERVICE_DEVMAN,
			    DEVMAN_DRIVER, 0);
		
		fibril_mutex_unlock(&devman_phone_mutex);
		return devman_phone_driver;
	case DEVMAN_CLIENT:
		fibril_mutex_lock(&devman_phone_mutex);
		if (devman_phone_client >= 0) {
			fibril_mutex_unlock(&devman_phone_mutex);
			return devman_phone_client;
		}
		
		if (flags & IPC_FLAG_BLOCKING) {
			devman_phone_client = service_obsolete_connect_blocking(
			    SERVICE_DEVMAN, DEVMAN_CLIENT, 0);
		} else {
			devman_phone_client = service_obsolete_connect(SERVICE_DEVMAN,
			    DEVMAN_CLIENT, 0);
		}
		
		fibril_mutex_unlock(&devman_phone_mutex);
		return devman_phone_client;
	default:
		return -1;
	}
}

void devman_obsolete_hangup_phone(devman_interface_t iface)
{
	switch (iface) {
	case DEVMAN_DRIVER:
		if (devman_phone_driver >= 0) {
			async_obsolete_hangup(devman_phone_driver);
			devman_phone_driver = -1;
		}
		break;
	case DEVMAN_CLIENT:
		if (devman_phone_client >= 0) {
			async_obsolete_hangup(devman_phone_client);
			devman_phone_client = -1;
		}
		break;
	default:
		break;
	}
}

int devman_obsolete_device_connect(devman_handle_t handle, unsigned int flags)
{
	int phone;
	
	if (flags & IPC_FLAG_BLOCKING) {
		phone = service_obsolete_connect_blocking(SERVICE_DEVMAN,
		    DEVMAN_CONNECT_TO_DEVICE, handle);
	} else {
		phone = service_obsolete_connect(SERVICE_DEVMAN,
		    DEVMAN_CONNECT_TO_DEVICE, handle);
	}
	
	return phone;
}

int devman_obsolete_parent_device_connect(devman_handle_t handle, unsigned int flags)
{
	int phone;
	
	if (flags & IPC_FLAG_BLOCKING) {
		phone = service_obsolete_connect_blocking(SERVICE_DEVMAN,
		    DEVMAN_CONNECT_TO_PARENTS_DEVICE, handle);
	} else {
		phone = service_obsolete_connect(SERVICE_DEVMAN,
		    DEVMAN_CONNECT_TO_PARENTS_DEVICE, handle);
	}
	
	return phone;
}

/** @}
 */
