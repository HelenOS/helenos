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

/** @addtogroup drvusbmouse
 * @{
 */
/**
 * @file
 * Common definitions for USB mouse driver.
 */

#ifndef USBMOUSE_MOUSE_H_
#define USBMOUSE_MOUSE_H_

#include <usb/dev/driver.h>
#include <usb/dev/pipes.h>
#include <time.h>
#include <async.h>

#define POLL_PIPE(dev) \
	((dev)->pipes[0].pipe)

/** Container for USB mouse device. */
typedef struct {
	/** Generic device container. */
	usb_device_t *dev;
	
	/** Function representing the device. */
	ddf_fun_t *mouse_fun;
	
	/** Polling interval in microseconds. */
	suseconds_t poll_interval_us;
	
	/** Callback session to console (consumer). */
	async_sess_t *console_sess;
} usb_mouse_t;

extern const usb_endpoint_description_t poll_endpoint_description;

extern int usb_mouse_create(usb_device_t *);
extern bool usb_mouse_polling_callback(usb_device_t *, uint8_t *, size_t,
    void *);
extern void usb_mouse_polling_ended_callback(usb_device_t *, bool, void *);

#endif

/**
 * @}
 */
