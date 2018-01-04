/*
 * Copyright (c) 2015 Jan Kolarik
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
	int input_ctrl_pipe_number;
	int output_ctrl_pipe_number;
	int input_data_pipe_number;
	int output_data_pipe_number;
	
	/** Pointer to connected USB device. */
	usb_device_t *usb_device;
} ath_usb_t;

typedef struct {
	uint16_t length;  /**< Little Endian value! */
	uint16_t tag;     /**< Little Endian value! */
} ath_usb_data_header_t;

extern errno_t ath_usb_init(ath_t *, usb_device_t *);

#endif  /* ATHEROS_ATH_USB_H */
