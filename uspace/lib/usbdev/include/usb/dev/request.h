/*
 * Copyright (c) 2011 Vojtech Horky
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
