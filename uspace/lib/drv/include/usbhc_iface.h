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
 * @brief USB host controller interface definition.
 */

#ifndef LIBDRV_USBHC_IFACE_H_
#define LIBDRV_USBHC_IFACE_H_

#include "ddf/driver.h"
#include <usb/usb.h>
#include <bool.h>


/** IPC methods for communication with HC through DDF interface.
 *
 * Notes for async methods:
 *
 * Methods for sending data to device (OUT transactions)
 * - e.g. IPC_M_USBHC_INTERRUPT_OUT -
 * always use the same semantics:
 * - first, IPC call with given method is made
 *   - argument #1 is target address
 *   - argument #2 is target endpoint
 *   - argument #3 is max packet size of the endpoint
 * - this call is immediately followed by IPC data write (from caller)
 * - the initial call (and the whole transaction) is answer after the
 *   transaction is scheduled by the HC and acknowledged by the device
 *   or immediately after error is detected
 * - the answer carries only the error code
 *
 * Methods for retrieving data from device (IN transactions)
 * - e.g. IPC_M_USBHC_INTERRUPT_IN -
 * also use the same semantics:
 * - first, IPC call with given method is made
 *   - argument #1 is target address
 *   - argument #2 is target endpoint
 *   - argument #3 is max packet size of the endpoint
 * - this call is immediately followed by IPC data read (async version)
 * - the call is not answered until the device returns some data (or until
 *   error occurs)
 *
 * Some special methods (NO-DATA transactions) do not send any data. These
 * might behave as both OUT or IN transactions because communication parts
 * where actual buffers are exchanged are omitted.
 **
 * For all these methods, wrap functions exists. Important rule: functions
 * for IN transactions have (as parameters) buffers where retrieved data
 * will be stored. These buffers must be already allocated and shall not be
 * touch until the transaction is completed
 * (e.g. not before calling usb_wait_for() with appropriate handle).
 * OUT transactions buffers can be freed immediately after call is dispatched
 * (i.e. after return from wrapping function).
 *
 */
typedef enum {
	/** Reserve usage of default address.
	 * This call informs the host controller that the caller will be
	 * using default USB address. It is duty of the HC driver to ensure
	 * that only single entity will have it reserved.
	 * The address is returned via IPC_M_USBHC_RELEASE_DEFAULT_ADDRESS.
	 * The caller can start using the address after receiving EOK
	 * answer.
	 */
	IPC_M_USBHC_RESERVE_DEFAULT_ADDRESS,

	/** Release usage of default address.
	 * @see IPC_M_USBHC_RESERVE_DEFAULT_ADDRESS
	 */
	IPC_M_USBHC_RELEASE_DEFAULT_ADDRESS,

	/** Asks for address assignment by host controller.
	 * Answer:
	 * - ELIMIT - host controller run out of address
	 * - EOK - address assigned
	 * Answer arguments:
	 * - assigned address
	 *
	 * The address must be released by via IPC_M_USBHC_RELEASE_ADDRESS.
	 */
	IPC_M_USBHC_REQUEST_ADDRESS,

	/** Bind USB address with devman handle.
	 * Parameters:
	 * - USB address
	 * - devman handle
	 * Answer:
	 * - EOK - address binded
	 * - ENOENT - address is not in use
	 */
	IPC_M_USBHC_BIND_ADDRESS,

	/** Release address in use.
	 * Arguments:
	 * - address to be released
	 * Answer:
	 * - ENOENT - address not in use
	 * - EPERM - trying to release default USB address
	 */
	IPC_M_USBHC_RELEASE_ADDRESS,


	/** Send interrupt data to device.
	 * See explanation at usb_iface_funcs_t (OUT transaction).
	 */
	IPC_M_USBHC_INTERRUPT_OUT,

	/** Get interrupt data from device.
	 * See explanation at usb_iface_funcs_t (IN transaction).
	 */
	IPC_M_USBHC_INTERRUPT_IN,

	/** Send bulk data to device.
	 * See explanation at usb_iface_funcs_t (OUT transaction).
	 */
	IPC_M_USBHC_BULK_OUT,

	/** Get bulk data from device.
	 * See explanation at usb_iface_funcs_t (IN transaction).
	 */
	IPC_M_USBHC_BULK_IN,

	/** Issue control WRITE transfer.
	 * See explanation at usb_iface_funcs_t (OUT transaction) for
	 * call parameters.
	 * This call is immediately followed by two IPC data writes
	 * from the caller (setup packet and actual data).
	 */
	IPC_M_USBHC_CONTROL_WRITE,

	/** Issue control READ transfer.
	 * See explanation at usb_iface_funcs_t (IN transaction) for
	 * call parameters.
	 * This call is immediately followed by IPC data write from the caller
	 * (setup packet) and IPC data read (buffer that was read).
	 */
	IPC_M_USBHC_CONTROL_READ,

	/* IPC_M_USB_ */
} usbhc_iface_funcs_t;

/** Callback for outgoing transfer. */
typedef void (*usbhc_iface_transfer_out_callback_t)(ddf_fun_t *,
    int, void *);

/** Callback for incoming transfer. */
typedef void (*usbhc_iface_transfer_in_callback_t)(ddf_fun_t *,
    int, size_t, void *);


/** Out transfer processing function prototype. */
typedef int (*usbhc_iface_transfer_out_t)(ddf_fun_t *, usb_target_t, size_t,
    void *, size_t,
    usbhc_iface_transfer_out_callback_t, void *);

/** Setup transfer processing function prototype. @deprecated */
typedef usbhc_iface_transfer_out_t usbhc_iface_transfer_setup_t;

/** In transfer processing function prototype. */
typedef int (*usbhc_iface_transfer_in_t)(ddf_fun_t *, usb_target_t, size_t,
    void *, size_t,
    usbhc_iface_transfer_in_callback_t, void *);

/** USB host controller communication interface. */
typedef struct {
	int (*reserve_default_address)(ddf_fun_t *, usb_speed_t);
	int (*release_default_address)(ddf_fun_t *);
	int (*request_address)(ddf_fun_t *, usb_speed_t, usb_address_t *);
	int (*bind_address)(ddf_fun_t *, usb_address_t, devman_handle_t);
	int (*release_address)(ddf_fun_t *, usb_address_t);

	usbhc_iface_transfer_out_t interrupt_out;
	usbhc_iface_transfer_in_t interrupt_in;

	usbhc_iface_transfer_out_t bulk_out;
	usbhc_iface_transfer_in_t bulk_in;

	int (*control_write)(ddf_fun_t *, usb_target_t,
	    size_t,
	    void *, size_t, void *, size_t,
	    usbhc_iface_transfer_out_callback_t, void *);

	int (*control_read)(ddf_fun_t *, usb_target_t,
	    size_t,
	    void *, size_t, void *, size_t,
	    usbhc_iface_transfer_in_callback_t, void *);
} usbhc_iface_t;


#endif
/**
 * @}
 */
