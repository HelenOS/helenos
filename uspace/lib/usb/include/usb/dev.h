/*
 * SPDX-FileCopyrightText: 2011 Jan Vesely
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup libusb
 * @{
 */

/** @file
 * Common USB types and functions.
 */

#ifndef LIBUSB_DEV_H_
#define LIBUSB_DEV_H_

#include <devman.h>

extern errno_t usb_resolve_device_handle(const char *, devman_handle_t *);

#endif

/**
 * @}
 */
