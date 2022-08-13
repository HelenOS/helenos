/*
 * SPDX-FileCopyrightText: 2011 Vojtech Horky
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup libusbdev
 * @{
 */
/** @file
 * USB device driver framework.
 */

#ifndef LIBUSBDEV_DEVICE_H_
#define LIBUSBDEV_DEVICE_H_

#include <ddf/driver.h>
#include <usb/usb.h>
#include <usb/descriptor.h>
#include <usb/dev/alternate_ifaces.h>
#include <usb/dev/pipes.h>
#include <usbhc_iface.h>

#include <assert.h>
#include <async.h>

/** Some useful descriptors for USB device. */
typedef struct {
	/** Standard device descriptor. */
	usb_standard_device_descriptor_t device;
	/** Full configuration descriptor of current configuration. */
	void *full_config;
	size_t full_config_size;
} usb_device_descriptors_t;

typedef struct usb_device usb_device_t;

/* DDF parts */
errno_t usb_device_create_ddf(ddf_dev_t *,
    const usb_endpoint_description_t **, const char **);
void usb_device_destroy_ddf(ddf_dev_t *);

static inline usb_device_t *usb_device_get(ddf_dev_t *dev)
{
	assert(dev);
	return ddf_dev_data_get(dev);
}

usb_device_t *usb_device_create(devman_handle_t);
void usb_device_destroy(usb_device_t *);

const char *usb_device_get_name(usb_device_t *);
ddf_fun_t *usb_device_ddf_fun_create(usb_device_t *, fun_type_t, const char *);

async_exch_t *usb_device_bus_exchange_begin(usb_device_t *);
void usb_device_bus_exchange_end(async_exch_t *);

errno_t usb_device_select_interface(usb_device_t *, uint8_t,
    const usb_endpoint_description_t **);

errno_t usb_device_create_pipes(usb_device_t *usb_dev,
    const usb_endpoint_description_t **endpoints);
void usb_device_destroy_pipes(usb_device_t *);

usb_pipe_t *usb_device_get_default_pipe(usb_device_t *);
usb_endpoint_mapping_t *usb_device_get_mapped_ep_desc(usb_device_t *,
    const usb_endpoint_description_t *);
int usb_device_unmap_ep(usb_endpoint_mapping_t *);

usb_address_t usb_device_get_address(const usb_device_t *);
usb_speed_t usb_device_get_depth(const usb_device_t *);
usb_speed_t usb_device_get_speed(const usb_device_t *);
int usb_device_get_iface_number(const usb_device_t *);
devman_handle_t usb_device_get_devman_handle(const usb_device_t *);

const usb_device_descriptors_t *usb_device_descriptors(usb_device_t *);

const usb_alternate_interfaces_t *usb_device_get_alternative_ifaces(
    usb_device_t *);

void *usb_device_data_alloc(usb_device_t *, size_t);
void *usb_device_data_get(usb_device_t *);

#endif
/**
 * @}
 */
