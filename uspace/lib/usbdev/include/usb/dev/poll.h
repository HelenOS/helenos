/*
 * Copyright (c) 2011 Vojtech Horky
 * Copyright (c) 2017 Petr Manek
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
#include <fibril_synch.h>


/** USB automated polling. */
typedef struct usb_polling {
	/** Mandatory parameters - user is expected to configure these. */

	/** USB device to poll. */
	usb_device_t *device;

	/** Device enpoint mapping to use for polling. */
	usb_endpoint_mapping_t *ep_mapping;

	/** Size of the recieved data. */
	size_t request_size;

	/**
	 * Data buffer of at least `request_size`. User is responsible for its
	 * allocation.
	 */
	uint8_t *buffer;

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


	/**
	 * Optional parameters - user can customize them, but they are
	 * defaulted to  some reasonable values.
	 */

	/** Level of debugging messages from auto polling.
	 * 0 - nothing (default)
	 * 1 - inform about errors and polling start/end
	 * 2 - also dump every retrieved buffer
	 */
	int debug;

	/**
	 * Maximum number of consecutive errors before polling termination
	 * (default 3).
	 */
	size_t max_failures;

	/** Delay between poll requests in milliseconds.
	 * By default, value from endpoint descriptor used.
	 */
	int delay;

	/** Whether to automatically try to clear the HALT feature after
	 * the endpoint stalls (true by default).
	 */
	bool auto_clear_halt;

	/** Argument to pass to callbacks (default NULL). */
	void *arg;

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


	/**
	 * Internal parameters - user is not expected to set them. Messing with
	 * them can result in unexpected behavior if you do not know what you
	 * are doing.
	 */

	/** Fibril used for polling. */
	fid_t fibril;

	/** True if polling is currently in operation. */
	volatile bool running;

	/** True if polling should terminate as soon as possible. */
	volatile bool joining;

	/** Synchronization primitives for joining polling end. */
	fibril_mutex_t guard;
	fibril_condvar_t cv;
} usb_polling_t;

errno_t usb_polling_init(usb_polling_t *);
void usb_polling_fini(usb_polling_t *);

errno_t usb_polling_start(usb_polling_t *);
errno_t usb_polling_join(usb_polling_t *);

#endif
/**
 * @}
 */
