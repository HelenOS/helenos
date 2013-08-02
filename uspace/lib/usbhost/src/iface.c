/*
 * Copyright (c) 2011 Jan Vesely
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

/** @addtogroup libusbhost
 * @{
 */
/** @file
 * @brief HCD DDF interface implementation
 */

#include <ddf/driver.h>
#include <errno.h>

#include <usb/debug.h>
#include <usb/host/endpoint.h>
#include <usb/host/hcd.h>
#include "ddf_helpers.h"

/** Inbound communication interface function.
 * @param fun DDF function.
 * @param target Communication target.
 * @param setup_data Data to use in setup stage (control transfers).
 * @param data Pointer to data buffer.
 * @param size Size of the data buffer.
 * @param callback Function to call on communication end.
 * @param arg Argument passed to the callback function.
 * @return Error code.
 */
static int usb_read(ddf_fun_t *fun, usb_target_t target, uint64_t setup_data,
    uint8_t *data, size_t size, usbhc_iface_transfer_in_callback_t callback,
    void *arg)
{
	return hcd_send_batch(dev_to_hcd(ddf_fun_get_dev(fun)), target,
	    USB_DIRECTION_IN, data, size, setup_data, callback, NULL, arg,
	    "READ");
}

/** Outbound communication interface function.
 * @param fun DDF function.
 * @param target Communication target.
 * @param setup_data Data to use in setup stage (control transfers).
 * @param data Pointer to data buffer.
 * @param size Size of the data buffer.
 * @param callback Function to call on communication end.
 * @param arg Argument passed to the callback function.
 * @return Error code.
 */
static int usb_write(ddf_fun_t *fun, usb_target_t target, uint64_t setup_data,
    const uint8_t *data, size_t size,
    usbhc_iface_transfer_out_callback_t callback, void *arg)
{
	return hcd_send_batch(dev_to_hcd(ddf_fun_get_dev(fun)),
	    target, USB_DIRECTION_OUT, (uint8_t*)data, size, setup_data, NULL,
	    callback, arg, "WRITE");
}

/** usbhc Interface implementation using hcd_t from libusbhost library. */
usbhc_iface_t hcd_iface = {
	.read = usb_read,
	.write = usb_write,
};

/**
 * @}
 */
