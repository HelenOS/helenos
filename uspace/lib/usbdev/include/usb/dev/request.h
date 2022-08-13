/*
 * SPDX-FileCopyrightText: 2011 Vojtech Horky
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup libusbdev
 * @{
 */
/** @file
 * Standard USB requests.
 */
#ifndef LIBUSBDEV_REQUEST_H_
#define LIBUSBDEV_REQUEST_H_

#include <stddef.h>
#include <stdint.h>
#include <l18n/langs.h>
#include <usb/usb.h>
#include <usb/dev/pipes.h>
#include <usb/descriptor.h>
#include <usb/request.h>

errno_t usb_control_request_set(usb_pipe_t *,
    usb_request_type_t, usb_request_recipient_t, uint8_t,
    uint16_t, uint16_t, const void *, size_t);

errno_t usb_control_request_get(usb_pipe_t *,
    usb_request_type_t, usb_request_recipient_t, uint8_t,
    uint16_t, uint16_t, void *, size_t, size_t *);

errno_t usb_request_get_status(usb_pipe_t *, usb_request_recipient_t,
    uint16_t, uint16_t *);
errno_t usb_request_clear_feature(usb_pipe_t *, usb_request_type_t,
    usb_request_recipient_t, uint16_t, uint16_t);
errno_t usb_request_set_feature(usb_pipe_t *, usb_request_type_t,
    usb_request_recipient_t, uint16_t, uint16_t);
errno_t usb_request_get_descriptor(usb_pipe_t *, usb_request_type_t,
    usb_request_recipient_t, uint8_t, uint8_t, uint16_t, void *, size_t,
    size_t *);
errno_t usb_request_get_descriptor_alloc(usb_pipe_t *, usb_request_type_t,
    usb_request_recipient_t, uint8_t, uint8_t, uint16_t, void **, size_t *);
errno_t usb_request_get_device_descriptor(usb_pipe_t *,
    usb_standard_device_descriptor_t *);
errno_t usb_request_get_bare_configuration_descriptor(usb_pipe_t *, int,
    usb_standard_configuration_descriptor_t *);
errno_t usb_request_get_full_configuration_descriptor(usb_pipe_t *, int,
    void *, size_t, size_t *);
errno_t usb_request_get_full_configuration_descriptor_alloc(usb_pipe_t *,
    int, void **, size_t *);
errno_t usb_request_set_descriptor(usb_pipe_t *, usb_request_type_t,
    usb_request_recipient_t, uint8_t, uint8_t, uint16_t, const void *, size_t);

errno_t usb_request_get_configuration(usb_pipe_t *, uint8_t *);
errno_t usb_request_set_configuration(usb_pipe_t *, uint8_t);

errno_t usb_request_get_interface(usb_pipe_t *, uint8_t, uint8_t *);
errno_t usb_request_set_interface(usb_pipe_t *, uint8_t, uint8_t);

errno_t usb_request_get_supported_languages(usb_pipe_t *,
    l18_win_locales_t **, size_t *);
errno_t usb_request_get_string(usb_pipe_t *, size_t, l18_win_locales_t,
    char **);

errno_t usb_pipe_clear_halt(usb_pipe_t *, usb_pipe_t *);
errno_t usb_request_get_endpoint_status(usb_pipe_t *, usb_pipe_t *, uint16_t *);

#endif
/**
 * @}
 */
