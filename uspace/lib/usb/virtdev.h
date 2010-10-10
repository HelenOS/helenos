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

/** @addtogroup libusb usb
 * @{
 */
/** @file
 * @brief Virtual USB device.
 */
#ifndef LIBUSB_VIRTDEV_H_
#define LIBUSB_VIRTDEV_H_

#include <ipc/ipc.h>
#include <async.h>
#include "hcd.h"

#define USB_VIRTDEV_KEYBOARD_ID 1
#define USB_VIRTDEV_KEYBOARD_ADDRESS 1

typedef void (*usb_virtdev_on_data_from_host_t)(usb_endpoint_t, void *, size_t);

int usb_virtdev_connect(const char *, int, usb_virtdev_on_data_from_host_t);
int usb_virtdev_data_to_host(int, usb_endpoint_t,
    void *, size_t);

typedef enum {
	IPC_M_USB_VIRTDEV_DATA_TO_DEVICE = IPC_FIRST_USER_METHOD,
	IPC_M_USB_VIRTDEV_DATA_FROM_DEVICE
} usb_virtdev_method_t;

#endif
/**
 * @}
 */
