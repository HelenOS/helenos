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

/** @addtogroup libusbvirt
 * @{
 */
/** @file
 * @brief Virtual USB device.
 */
#ifndef LIBUSBVIRT_HUB_H_
#define LIBUSBVIRT_HUB_H_

#include "device.h"

/** USB transaction type.
 * This types does not correspond directly to types in USB specification,
 * as actually DATA transactions are marked with these types to identify
 * their direction (and tag).
 */
typedef enum {
	USBVIRT_TRANSACTION_SETUP,
	USBVIRT_TRANSACTION_IN,
	USBVIRT_TRANSACTION_OUT
} usbvirt_transaction_type_t;

const char *usbvirt_str_transaction_type(usbvirt_transaction_type_t type);

/** Telephony methods of virtual devices. */
typedef enum {
	IPC_M_USBVIRT_GET_NAME = IPC_FIRST_USER_METHOD,
	IPC_M_USBVIRT_TRANSACTION_SETUP,
	IPC_M_USBVIRT_TRANSACTION_OUT,
	IPC_M_USBVIRT_TRANSACTION_IN,
} usbvirt_device_method_t;

int usbvirt_connect(usbvirt_device_t *);
int usbvirt_connect_local(usbvirt_device_t *);
int usbvirt_disconnect(usbvirt_device_t *dev);

#endif
/**
 * @}
 */
