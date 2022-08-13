/*
 * SPDX-FileCopyrightText: 2011 Vojtech Horky
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup libusbdev
 * @{
 */
/** @file
 * USB device recognition.
 */

#ifndef LIBUSBDEV_RECOGNISE_H_
#define LIBUSBDEV_RECOGNISE_H_

#include <usb/descriptor.h>
#include <usb/dev/pipes.h>

#include <devman.h>

extern errno_t usb_device_create_match_ids_from_device_descriptor(
    const usb_standard_device_descriptor_t *, match_id_list_t *);

extern errno_t usb_device_create_match_ids_from_interface(
    const usb_standard_device_descriptor_t *,
    const usb_standard_interface_descriptor_t *, match_id_list_t *);

extern errno_t usb_device_create_match_ids(usb_pipe_t *, match_id_list_t *);

#endif

/**
 * @}
 */
