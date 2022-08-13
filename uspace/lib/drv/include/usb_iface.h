/*
 * SPDX-FileCopyrightText: 2010 Vojtech Horky
 * SPDX-FileCopyrightText: 2018 Ondrej Hlavaty, Michal Staruch
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup libdrv
 * @addtogroup usb
 * @{
 */
/** @file
 * @brief USB device interface definition.
 */

#ifndef LIBDRV_USB_IFACE_H_
#define LIBDRV_USB_IFACE_H_

#include "ddf/driver.h"
#include <async.h>
#include <usbhc_iface.h>

typedef async_sess_t usb_dev_session_t;

typedef struct {
	usb_address_t address;	/**< Current USB address */
	uint8_t depth;		/**< Depth in the hub hiearchy */
	usb_speed_t speed;	/**< Speed of the device */
	devman_handle_t handle;	/**< Handle to DDF function of the HC driver */
	/** Interface set by multi interface driver,  -1 if none */
	int iface;
} usb_device_desc_t;

extern usb_dev_session_t *usb_dev_connect(devman_handle_t);
extern usb_dev_session_t *usb_dev_connect_to_self(ddf_dev_t *);
extern void usb_dev_disconnect(usb_dev_session_t *);

extern errno_t usb_get_my_description(async_exch_t *, usb_device_desc_t *);

/** USB device communication interface. */
typedef struct {
	errno_t (*get_my_description)(ddf_fun_t *, usb_device_desc_t *);
} usb_iface_t;

#endif
/**
 * @}
 */
