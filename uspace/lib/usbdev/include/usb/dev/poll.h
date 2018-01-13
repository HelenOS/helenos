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

/** @addtogroup libusbdev
 * @{
 */
/** @file
 * USB device polling functions.
 */
#ifndef LIBUSBDEV_POLL_H_
#define LIBUSBDEV_POLL_H_

#include <usb/usb.h>
#include <usb/dev/device.h>
#include <usb/dev/pipes.h>

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

/** Parameters and callbacks for automated polling. */
typedef struct {
	/** Level of debugging messages from auto polling.
	 * 0 - nothing
	 * 1 - inform about errors and polling start/end
	 * 2 - also dump every retrieved buffer
	 */
	int debug;
	/** Maximum number of consecutive errors before polling termination. */
	size_t max_failures;
	/** Delay between poll requests in milliseconds.
	 * Set to negative value to use value from endpoint descriptor.
	 */
	int delay;
	/** Whether to automatically try to clear the HALT feature after
	 * the endpoint stalls.
	 */
	bool auto_clear_halt;
	/** Callback when data arrives.
	 *
	 * @param dev Device that was polled.
	 * @param data Data buffer (in USB endianness).
	 * @param data_size Size of the @p data buffer in bytes.
	 * @param arg Custom argument.
	 * @return Whether to continue in polling.
	 */
	bool (*on_data)(usb_device_t *dev, uint8_t *data, size_t data_size,
	    void *arg);
	/** Callback when polling is terminated.
	 *
	 * @param dev Device where the polling was terminated.
	 * @param due_to_errors Whether polling stopped due to several failures.
	 * @param arg Custom argument.
	 */
	void (*on_polling_end)(usb_device_t *dev, bool due_to_errors,
	    void *arg);
	/** Callback when error occurs.
	 *
	 * @param dev Device where error occurred.
	 * @param err_code Error code (as returned from usb_pipe_read).
	 * @param arg Custom argument.
	 * @return Whether to continue in polling.
	 */
	bool (*on_error)(usb_device_t *dev, errno_t err_code, void *arg);
	/** Argument to pass to callbacks. */
	void *arg;
} usb_device_auto_polling_t;

typedef bool (*usb_polling_callback_t)(usb_device_t *, uint8_t *, size_t, void *);
typedef void (*usb_polling_terminted_callback_t)(usb_device_t *, bool, void *);

extern errno_t usb_device_auto_polling(usb_device_t *, usb_endpoint_t,
    const usb_device_auto_polling_t *, size_t);

extern errno_t usb_device_auto_poll(usb_device_t *, usb_endpoint_t,
    usb_polling_callback_t, size_t, int, usb_polling_terminted_callback_t, void *);

extern errno_t usb_device_auto_polling_desc(usb_device_t *,
    const usb_endpoint_description_t *, const usb_device_auto_polling_t *,
    size_t);

extern errno_t usb_device_auto_poll_desc(usb_device_t *,
    const usb_endpoint_description_t *, usb_polling_callback_t, size_t, int,
    usb_polling_terminted_callback_t, void *);

#endif
/**
 * @}
 */
