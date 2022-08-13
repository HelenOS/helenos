/*
 * SPDX-FileCopyrightText: 2010-2011 Vojtech Horky
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup libusb
 * @{
 */
/** @file
 * Debugging related functions.
 */
#ifndef LIBUSB_DEBUG_H_
#define LIBUSB_DEBUG_H_
#include <stdio.h>
#include <stddef.h>
#include <stdint.h>
#include <io/log.h>

void usb_dump_standard_descriptor(FILE *, const char *, const char *,
    const uint8_t *, size_t);

#define USB_LOG_LEVEL_FATAL LVL_FATAL
#define USB_LOG_LEVEL_ERROR LVL_ERROR
#define USB_LOG_LEVEL_WARNING LVL_WARN
#define USB_LOG_LEVEL_INFO LVL_NOTE
#define USB_LOG_LEVEL_DEBUG LVL_DEBUG
#define USB_LOG_LEVEL_DEBUG2 LVL_DEBUG2

#define usb_log_printf(level, format, ...) \
	log_msg(LOG_DEFAULT, level, format, ##__VA_ARGS__)

/** Log fatal error. */
#define usb_log_fatal(format, ...) \
	usb_log_printf(USB_LOG_LEVEL_FATAL, format, ##__VA_ARGS__)

/** Log normal (recoverable) error. */
#define usb_log_error(format, ...) \
	usb_log_printf(USB_LOG_LEVEL_ERROR, format, ##__VA_ARGS__)

/** Log warning. */
#define usb_log_warning(format, ...) \
	usb_log_printf(USB_LOG_LEVEL_WARNING, format, ##__VA_ARGS__)

/** Log informational message. */
#define usb_log_info(format, ...) \
	usb_log_printf(USB_LOG_LEVEL_INFO, format, ##__VA_ARGS__)

/** Log debugging message. */
#define usb_log_debug(format, ...) \
	usb_log_printf(USB_LOG_LEVEL_DEBUG, format, ##__VA_ARGS__)

/** Log verbose debugging message. */
#define usb_log_debug2(format, ...) \
	usb_log_printf(USB_LOG_LEVEL_DEBUG2, format, ##__VA_ARGS__)

const char *usb_debug_str_buffer(const uint8_t *, size_t, size_t);

#endif
/**
 * @}
 */
