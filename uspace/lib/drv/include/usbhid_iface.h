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
 * @{
 */
/** @file
 * USB HID interface definition.
 */

#ifndef LIBDRV_USBHID_IFACE_H_
#define LIBDRV_USBHID_IFACE_H_

#include "ddf/driver.h"
#include <usb/usb.h>

/** IPC methods for USB HID device interface. */
typedef enum {
	/** Get number of events reported in single burst.
	 * Parameters: none
	 * Answer:
	 * - EOK (expected always as long as device support USB HID interface)
	 * Parameters of the answer:
	 * - number of items
	 */
	IPC_M_USBHID_GET_EVENT_LENGTH,
	/** Get single event from the HID device.
	 * The word single refers to set of individual events that were
	 * available at particular point in time.
	 * Parameters:
	 * - flags
	 * The call is followed by data read expecting two concatenated
	 * arrays.
	 * Answer:
	 * - EOK - events returned
	 * - EAGAIN - no event ready (only in non-blocking mode)
	 *
	 * It is okay if the client requests less data. Extra data must
	 * be truncated by the driver.
	 */
	IPC_M_USBHID_GET_EVENT
} usbhid_iface_funcs_t;

/** USB HID interface flag - return immediately if no data are available. */
#define USBHID_IFACE_FLAG_NON_BLOCKING (1 << 0)

/** USB HID device communication interface. */
typedef struct {
	/** Get number of items in the event.
	 *
	 * @param[in] fun DDF function answering the request.
	 * @return Number of events or error code.
	 */
	int (*get_event_length)(ddf_fun_t *fun);

	/** Get single event from the HID device.
	 *
	 * @param[in] fun DDF function answering the request.
	 * @param[out] usage_page Array of usage pages and usages.
	 * @param[out] usage Array of data (1:1 with @p usage).
	 * @param[in] size Size of @p usage and @p data arrays.
	 * @param[out] act_size Actual number of returned events.
	 * @param[in] flags Flags (see USBHID_IFACE_FLAG_*).
	 * @return Error code.
	 */
	int (*get_event)(ddf_fun_t *fun,
	    uint16_t *usage_page, uint16_t *usage, size_t size, size_t *act_size,
	    unsigned int flags);
} usbhid_iface_t;


#endif
/**
 * @}
 */
