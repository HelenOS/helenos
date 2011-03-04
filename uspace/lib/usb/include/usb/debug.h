/*
 * Copyright (c) 2010-2011 Vojtech Horky
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
 * @brief Debugging related functions.
 */
#ifndef LIBUSB_DEBUG_H_
#define LIBUSB_DEBUG_H_
#include <stdio.h>
#include <usb/usb.h>
#include <assert.h>

void usb_dprintf(const char *tag, int level, const char *format, ...);
void usb_dprintf_enable(const char *tag, int level);

void usb_dump_standard_descriptor(FILE *, const char *, const char *,
    const uint8_t *, size_t);

/** Logging level. */
typedef enum {
	USB_LOG_LEVEL_FATAL,
	USB_LOG_LEVEL_ERROR,
	USB_LOG_LEVEL_WARNING,
	USB_LOG_LEVEL_INFO,
	USB_LOG_LEVEL_DEBUG,
	USB_LOG_LEVEL_DEBUG2,
	USB_LOG_LEVEL_MAX
} usb_log_level_t;


void usb_log_enable(usb_log_level_t, const char *);

void usb_log_printf(usb_log_level_t, const char *, ...);

#define usb_log_fatal(format, ...) \
	usb_log_printf(USB_LOG_LEVEL_FATAL, format, ##__VA_ARGS__)

#define usb_log_error(format, ...) \
	usb_log_printf(USB_LOG_LEVEL_ERROR, format, ##__VA_ARGS__)

#define usb_log_warning(format, ...) \
	usb_log_printf(USB_LOG_LEVEL_WARNING, format, ##__VA_ARGS__)

#define usb_log_info(format, ...) \
	usb_log_printf(USB_LOG_LEVEL_INFO, format, ##__VA_ARGS__)

#define usb_log_debug(format, ...) \
	usb_log_printf(USB_LOG_LEVEL_DEBUG, format, ##__VA_ARGS__)

#define usb_log_debug2(format, ...) \
	usb_log_printf(USB_LOG_LEVEL_DEBUG2, format, ##__VA_ARGS__)

const char *usb_debug_str_buffer(uint8_t *, size_t, size_t);


#endif
/**
 * @}
 */

