/*
 * SPDX-FileCopyrightText: 2017 Petr Manek
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup drvusbdiag
 * @{
 */
/** @file
 * USB diagnostic device structures.
 */

#ifndef USBDIAG_DEVICE_H_
#define USBDIAG_DEVICE_H_

#include <usb/dev/device.h>
#include <usb/dev/driver.h>

#define USBDIAG_EP_BURST_INTR_IN    1
#define USBDIAG_EP_BURST_INTR_OUT   2
#define USBDIAG_EP_BURST_BULK_IN    3
#define USBDIAG_EP_BURST_BULK_OUT   4
#define USBDIAG_EP_BURST_ISOCH_IN   5
#define USBDIAG_EP_BURST_ISOCH_OUT  6

#define USBDIAG_EP_DATA_INTR_IN     7
#define USBDIAG_EP_DATA_INTR_OUT    8
#define USBDIAG_EP_DATA_BULK_IN     9
#define USBDIAG_EP_DATA_BULK_OUT   10
#define USBDIAG_EP_DATA_ISOCH_IN   11
#define USBDIAG_EP_DATA_ISOCH_OUT  12

/**
 * USB diagnostic device.
 */
typedef struct usbdiag_dev {
	usb_device_t *usb_dev;
	ddf_fun_t *fun;

	usb_pipe_t *burst_intr_in;
	usb_pipe_t *burst_intr_out;
	usb_pipe_t *burst_bulk_in;
	usb_pipe_t *burst_bulk_out;
	usb_pipe_t *burst_isoch_in;
	usb_pipe_t *burst_isoch_out;

	usb_pipe_t *data_intr_in;
	usb_pipe_t *data_intr_out;
	usb_pipe_t *data_bulk_in;
	usb_pipe_t *data_bulk_out;
	usb_pipe_t *data_isoch_in;
	usb_pipe_t *data_isoch_out;

} usbdiag_dev_t;

errno_t usbdiag_dev_create(usb_device_t *, usbdiag_dev_t **,
    const usb_endpoint_description_t **);
void usbdiag_dev_destroy(usbdiag_dev_t *);

static inline usbdiag_dev_t *usb_device_to_usbdiag_dev(usb_device_t *usb_dev)
{
	assert(usb_dev);
	return usb_device_data_get(usb_dev);
}

static inline usbdiag_dev_t *ddf_dev_to_usbdiag_dev(ddf_dev_t *ddf_dev)
{
	assert(ddf_dev);
	return usb_device_to_usbdiag_dev(usb_device_get(ddf_dev));
}

static inline usbdiag_dev_t *ddf_fun_to_usbdiag_dev(ddf_fun_t *ddf_fun)
{
	assert(ddf_fun);
	return ddf_dev_to_usbdiag_dev(ddf_fun_get_dev(ddf_fun));
}

#endif /* USBDIAG_USBDIAG_H_ */

/**
 * @}
 */
