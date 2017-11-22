/*
 * Copyright (c) 2010 Vojtech Horky
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

/** @addtogroup libusb
 * @{
 */
/** @file
 * Common USB functions.
 */
#include <usb/usb.h>
#include <usb/request.h>

#include <assert.h>
#include <byteorder.h>
#include <macros.h>

static const char *str_speed[] = {
	[USB_SPEED_LOW] = "low",
	[USB_SPEED_FULL] = "full",
	[USB_SPEED_HIGH] = "high",
	[USB_SPEED_SUPER] = "super",
};

static const char *str_transfer_type[] = {
	[USB_TRANSFER_CONTROL] = "control",
	[USB_TRANSFER_ISOCHRONOUS] = "isochronous",
	[USB_TRANSFER_BULK] = "bulk",
	[USB_TRANSFER_INTERRUPT] = "interrupt",
};

static const char *str_transfer_type_short[] = {
	[USB_TRANSFER_CONTROL] = "ctrl",
	[USB_TRANSFER_ISOCHRONOUS] = "iso",
	[USB_TRANSFER_BULK] = "bulk",
	[USB_TRANSFER_INTERRUPT] = "intr",
};

static const char *str_direction[] = {
	[USB_DIRECTION_IN] = "in",
	[USB_DIRECTION_OUT] = "out",
	[USB_DIRECTION_BOTH] = "both",
};

/** String representation for USB transfer type.
 *
 * @param t Transfer type.
 * @return Transfer type as a string (in English).
 */
const char *usb_str_transfer_type(usb_transfer_type_t t)
{
	if ((size_t)t >= ARRAY_SIZE(str_transfer_type)) {
		return "invalid";
	}
	return str_transfer_type[t];
}

/** String representation for USB transfer type (short version).
 *
 * @param t Transfer type.
 * @return Transfer type as a short string for debugging messages.
 */
const char *usb_str_transfer_type_short(usb_transfer_type_t t)
{
	if ((size_t)t >= ARRAY_SIZE(str_transfer_type_short)) {
		return "invl";
	}
	return str_transfer_type_short[t];
}

/** String representation of USB direction.
 *
 * @param d The direction.
 * @return Direction as a string (in English).
 */
const char *usb_str_direction(usb_direction_t d)
{
	if (d >= ARRAY_SIZE(str_direction)) {
		return "invalid";
	}
	return str_direction[d];
}

/** String representation of USB speed.
 *
 * @param s The speed.
 * @return USB speed as a string (in English).
 */
const char *usb_str_speed(usb_speed_t s)
{
	if (s >= ARRAY_SIZE(str_speed)) {
		return "invalid";
	}
	return str_speed[s];
}

/**
 * @}
 */
