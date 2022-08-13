/*
 * SPDX-FileCopyrightText: 2011 Vojtech Horky
 *
 * SPDX-License-Identifier: BSD-3-Clause
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
