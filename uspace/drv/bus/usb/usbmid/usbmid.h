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

/** @addtogroup drvusbmid
 * @{
 */
/** @file
 * Common definitions.
 */

#ifndef USBMID_H_
#define USBMID_H_

#include <adt/list.h>
#include <ddf/driver.h>
#include <usb/usb.h>
#include <usb/dev/pipes.h>
#include <usb/debug.h>
#include <usb/dev/driver.h>

#define NAME "usbmid"

/** Container for single interface in a MID device. */
typedef struct {
	/** Function container. */
	ddf_fun_t *fun;
	/** Interface number. */
	int interface_no;
	/** List link. */
	link_t link;
} usbmid_interface_t;

/** Container to hold all the function pointers */
typedef struct usb_mid {
	ddf_fun_t *ctl_fun;
	list_t interface_list;
} usb_mid_t;

extern errno_t usbmid_explore_device(usb_device_t *);
extern errno_t usbmid_spawn_interface_child(usb_device_t *, usbmid_interface_t **,
    const usb_standard_device_descriptor_t *,
    const usb_standard_interface_descriptor_t *);
extern void usbmid_dump_descriptors(uint8_t *, size_t);
extern errno_t usbmid_interface_destroy(usbmid_interface_t *mid_iface);

static inline usbmid_interface_t * usbmid_interface_from_link(link_t *item)
{
	return list_get_instance(item, usbmid_interface_t, link);
}

#endif

/**
 * @}
 */
