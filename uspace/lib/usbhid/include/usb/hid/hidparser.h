/*
 * SPDX-FileCopyrightText: 2011 Matej Klonfar
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup libusbhid
 * @{
 */
/** @file
 * USB HID report descriptor and report data parser
 */
#ifndef LIBUSBHID_HIDPARSER_H_
#define LIBUSBHID_HIDPARSER_H_

#include <stdint.h>
#include <adt/list.h>
#include <usb/hid/hid_report_items.h>
#include <usb/hid/hidpath.h>
#include <usb/hid/hidtypes.h>
#include <usb/hid/hiddescriptor.h>

/*
 * Input report parser functions
 */
errno_t usb_hid_parse_report(const usb_hid_report_t *report, const uint8_t *data,
    size_t size, uint8_t *report_id);

/*
 * Output report parser functions
 */
uint8_t *usb_hid_report_output(usb_hid_report_t *report, size_t *size,
    uint8_t report_id);

void usb_hid_report_output_free(uint8_t *output);

size_t usb_hid_report_size(usb_hid_report_t *report, uint8_t report_id,
    usb_hid_report_type_t type);

size_t usb_hid_report_byte_size(usb_hid_report_t *report, uint8_t report_id,
    usb_hid_report_type_t type);

errno_t usb_hid_report_output_translate(usb_hid_report_t *report,
    uint8_t report_id, uint8_t *buffer, size_t size);

/*
 * Report descriptor structure observing functions
 */
usb_hid_report_field_t *usb_hid_report_get_sibling(usb_hid_report_t *report,
    usb_hid_report_field_t *field, usb_hid_report_path_t *path,
    int flags, usb_hid_report_type_t type);

uint8_t usb_hid_get_next_report_id(usb_hid_report_t *report,
    uint8_t report_id, usb_hid_report_type_t type);

#endif
/**
 * @}
 */
