/*
 * SPDX-FileCopyrightText: 2015 Jan Kolarik
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @file ath_usb.h
 *
 * Definitions of Atheros USB wifi device functions.
 *
 */

#ifndef ATHEROS_ATH_USB_H
#define ATHEROS_ATH_USB_H

#include <usb/dev/driver.h>
#include "ath.h"

#define RX_TAG  0x4e00
#define TX_TAG  0x697e

/** Atheros USB wifi device structure */
typedef struct {
	/** USB pipes indexes */
	usb_pipe_t *input_ctrl_pipe;
	usb_pipe_t *output_ctrl_pipe;
	usb_pipe_t *input_data_pipe;
	usb_pipe_t *output_data_pipe;

	/** Pointer to connected USB device. */
	usb_device_t *usb_device;
} ath_usb_t;

typedef struct {
	uint16_t length;  /**< Little Endian value! */
	uint16_t tag;     /**< Little Endian value! */
} ath_usb_data_header_t;

extern errno_t ath_usb_init(ath_t *, usb_device_t *, const usb_endpoint_description_t **endpoints);

#endif  /* ATHEROS_ATH_USB_H */
