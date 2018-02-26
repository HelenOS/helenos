/*
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
