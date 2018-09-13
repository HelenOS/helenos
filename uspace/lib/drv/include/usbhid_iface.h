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

extern errno_t usbhid_dev_get_event_length(async_sess_t *, size_t *);
extern errno_t usbhid_dev_get_event(async_sess_t *, uint8_t *, size_t, size_t *,
    int *, unsigned int);
extern errno_t usbhid_dev_get_report_descriptor_length(async_sess_t *, size_t *);
extern errno_t usbhid_dev_get_report_descriptor(async_sess_t *, uint8_t *, size_t,
    size_t *);

/** USB HID device communication interface. */
typedef struct {
	/** Get size of the event in bytes.
	 *
	 * @param[in] fun DDF function answering the request.
	 * @return Size of the event in bytes.
	 */
	size_t (*get_event_length)(ddf_fun_t *fun);

	/** Get single event from the HID device.
	 *
	 * @param[in] fun DDF function answering the request.
	 * @param[out] buffer Buffer with raw data from the device.
	 * @param[out] act_size Actual number of returned events.
	 * @param[in] flags Flags (see USBHID_IFACE_FLAG_*).
	 * @return Error code.
	 */
	errno_t (*get_event)(ddf_fun_t *fun, uint8_t *buffer, size_t size,
	    size_t *act_size, int *event_nr, unsigned int flags);

	/** Get size of the report descriptor in bytes.
	 *
	 * @param[in] fun DDF function answering the request.
	 * @return Size of the report descriptor in bytes.
	 */
	size_t (*get_report_descriptor_length)(ddf_fun_t *fun);

	/** Get the report descriptor from the HID device.
	 *
	 * @param[in] fun DDF function answering the request.
	 * @param[out] desc Buffer with the report descriptor.
	 * @param[in] size Size of the allocated @p desc buffer.
	 * @param[out] act_size Actual size of the report descriptor returned.
	 * @return Error code.
	 */
	errno_t (*get_report_descriptor)(ddf_fun_t *fun, uint8_t *desc,
	    size_t size, size_t *act_size);
} usbhid_iface_t;

#endif
/**
 * @}
 */
