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

/** @addtogroup libusbvirt
 * @{
 */
/** @file
 * IPC wrappers for virtual USB.
 */

#ifndef LIBUSBVIRT_IPC_H_
#define LIBUSBVIRT_IPC_H_

#include <ipc/common.h>
#include <usb/usb.h>
#include <stdbool.h>
#include <usbvirt/device.h>
#include <async.h>

/** IPC methods communication between host controller and virtual device. */
typedef enum {
	IPC_M_USBVIRT_GET_NAME = IPC_FIRST_USER_METHOD + 80,
	IPC_M_USBVIRT_CONTROL_READ,
	IPC_M_USBVIRT_CONTROL_WRITE,
	IPC_M_USBVIRT_INTERRUPT_IN,
	IPC_M_USBVIRT_INTERRUPT_OUT,
	IPC_M_USBVIRT_BULK_IN,
	IPC_M_USBVIRT_BULK_OUT
} usbvirt_hc_to_device_method_t;

extern errno_t usbvirt_ipc_send_control_read(async_sess_t *, void *, size_t,
    void *, size_t, size_t *);
extern errno_t usbvirt_ipc_send_control_write(async_sess_t *, void *, size_t,
    void *, size_t);
extern errno_t usbvirt_ipc_send_data_in(async_sess_t *, usb_endpoint_t,
    usb_transfer_type_t, void *, size_t, size_t *);
extern errno_t usbvirt_ipc_send_data_out(async_sess_t *, usb_endpoint_t,
    usb_transfer_type_t, void *, size_t);

extern bool usbvirt_is_usbvirt_method(sysarg_t);
extern bool usbvirt_ipc_handle_call(usbvirt_device_t *, ipc_call_t *);

#endif

/**
 * @}
 */
