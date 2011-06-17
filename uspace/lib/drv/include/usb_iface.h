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

/** @addtogroup libdrv
 * @addtogroup usb
 * @{
 */
/** @file
 * @brief USB interface definition.
 */

#ifndef LIBDRV_USB_IFACE_H_
#define LIBDRV_USB_IFACE_H_

#include "ddf/driver.h"
#include <usb/usb.h>
typedef enum {
	/** Tell USB address assigned to device.
	 * Parameters:
	 * - devman handle id
	 * Answer:
	 * - EINVAL - unknown handle or handle not managed by this driver
	 * - ENOTSUP - operation not supported (shall not happen)
	 * - arbitrary error code if returned by remote implementation
	 * - EOK - handle found, first parameter contains the USB address
	 *
	 * The handle must be the one used for binding USB address with
	 * it (IPC_M_USBHC_BIND_ADDRESS), otherwise the host controller
	 * (that this request would eventually reach) would not be able
	 * to find it.
	 * The problem is that this handle is actually assigned to the
	 * function inside driver of the parent device (usually hub driver).
	 * To bypass this problem, the initial caller specify handle as
	 * zero and the first parent assigns the actual value.
	 * See usb_iface_get_address_hub_child_impl() implementation
	 * that could be assigned to device ops of a child device of in a
	 * hub driver.
	 * For example, the USB multi interface device driver (MID)
	 * passes this initial zero without any modification because the
	 * handle must be resolved by its parent.
	 */
	IPC_M_USB_GET_ADDRESS,

	/** Tell interface number given device can use.
	 * Parameters
	 * - devman handle id of the device
	 * Answer:
	 * - ENOTSUP - operation not supported (can also mean any interface)
	 * - EOK - operation okay, first parameter contains interface number
	 */
	IPC_M_USB_GET_INTERFACE,

	/** Tell devman handle of device host controller.
	 * Parameters:
	 * - none
	 * Answer:
	 * - EOK - request processed without errors
	 * - ENOTSUP - this indicates invalid USB driver
	 * Parameters of the answer:
	 * - devman handle of HC caller is physically connected to
	 */
	IPC_M_USB_GET_HOST_CONTROLLER_HANDLE
} usb_iface_funcs_t;

/** USB device communication interface. */
typedef struct {
	int (*get_address)(ddf_fun_t *, devman_handle_t, usb_address_t *);
	int (*get_interface)(ddf_fun_t *, devman_handle_t, int *);
	int (*get_hc_handle)(ddf_fun_t *, devman_handle_t *);
} usb_iface_t;


#endif
/**
 * @}
 */
