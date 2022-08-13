/*
 * SPDX-FileCopyrightText: 2010 Vojtech Horky
 * SPDX-FileCopyrightText: 2011 Vojtech Horky
 * SPDX-FileCopyrightText: 2018 Ondrej Hlavaty, Petr Manek
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup drvusbhub
 * @{
 */
/** @file
 * @brief Hub driver.
 */

#ifndef DRV_USBHUB_USBHUB_H
#define DRV_USBHUB_USBHUB_H

#include <ddf/driver.h>
#include <fibril_synch.h>

#include <usb/classes/hub.h>

#include <usb/dev/pipes.h>
#include <usb/dev/driver.h>
#include <usb/dev/poll.h>

#define NAME "usbhub"

#include "port.h"
#include "status.h"

/** Information about attached hub. */
struct usb_hub_dev {
	/** Number of ports. */
	size_t port_count;
	/** Port structures, one for each port */
	usb_hub_port_t *ports;
	/** Speed of the hub */
	usb_speed_t speed;
	/** Generic usb device data */
	usb_device_t *usb_device;
	/** Data polling handle. */
	usb_polling_t polling;
	/** Pointer to usbhub function. */
	ddf_fun_t *hub_fun;
	/** Device communication pipe. */
	usb_pipe_t *control_pipe;
	/** Hub supports port power switching. */
	bool power_switched;
	/** Each port is switched individually. */
	bool per_port_power;
	/** Whether MTT is available */
	bool mtt_available;
};

extern const usb_endpoint_description_t *usb_hub_endpoints [];

errno_t usb_hub_device_add(usb_device_t *);
errno_t usb_hub_device_remove(usb_device_t *);
errno_t usb_hub_device_gone(usb_device_t *);

errno_t usb_hub_set_depth(const usb_hub_dev_t *);
errno_t usb_hub_get_port_status(const usb_hub_dev_t *, size_t,
    usb_port_status_t *);
errno_t usb_hub_set_port_feature(const usb_hub_dev_t *, size_t,
    usb_hub_class_feature_t);
errno_t usb_hub_clear_port_feature(const usb_hub_dev_t *, size_t,
    usb_hub_class_feature_t);

bool hub_port_changes_callback(usb_device_t *, uint8_t *, size_t, void *);

errno_t usb_hub_reserve_default_address(usb_hub_dev_t *, async_exch_t *,
    usb_port_t *);
errno_t usb_hub_release_default_address(usb_hub_dev_t *, async_exch_t *);

#endif

/**
 * @}
 */
