/*
 * SPDX-FileCopyrightText: 2011 Vojtech Horky
 *
 * SPDX-License-Identifier: BSD-3-Clause
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

static inline usbmid_interface_t *usbmid_interface_from_link(link_t *item)
{
	return list_get_instance(item, usbmid_interface_t, link);
}

#endif

/**
 * @}
 */
