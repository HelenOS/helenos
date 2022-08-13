/*
 * SPDX-FileCopyrightText: 2011 Matej Klonfar
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup libusb
 * @{
 */
/** @file
 * USB HID report descriptor and report data parser
 */
#ifndef LIBUSB_HIDDESCRIPTOR_H_
#define LIBUSB_HIDDESCRIPTOR_H_

#include <errno.h>
#include <stdint.h>
#include <adt/list.h>
#include <usb/hid/hid_report_items.h>
#include <usb/hid/hidpath.h>
#include <usb/hid/hidtypes.h>

errno_t usb_hid_parse_report_descriptor(usb_hid_report_t *report,
    const uint8_t *data, size_t size);

void usb_hid_descriptor_print(usb_hid_report_t *report);

errno_t usb_hid_report_init(usb_hid_report_t *report);

void usb_hid_report_deinit(usb_hid_report_t *report);

errno_t usb_hid_report_append_fields(usb_hid_report_t *report,
    usb_hid_report_item_t *report_item);

usb_hid_report_description_t *usb_hid_report_find_description(
    const usb_hid_report_t *report, uint8_t report_id,
    usb_hid_report_type_t type);

int usb_hid_report_parse_tag(uint8_t tag, uint8_t class, const uint8_t *data,
    size_t item_size, usb_hid_report_item_t *report_item,
    usb_hid_report_path_t *usage_path);

int usb_hid_report_parse_main_tag(uint8_t tag, const uint8_t *data,
    size_t item_size, usb_hid_report_item_t *report_item,
    usb_hid_report_path_t *usage_path);

int usb_hid_report_parse_global_tag(uint8_t tag, const uint8_t *data,
    size_t item_size, usb_hid_report_item_t *report_item,
    usb_hid_report_path_t *usage_path);

int usb_hid_report_parse_local_tag(uint8_t tag, const uint8_t *data,
    size_t item_size, usb_hid_report_item_t *report_item,
    usb_hid_report_path_t *usage_path);

void usb_hid_descriptor_print_list(list_t *list);

void usb_hid_report_reset_local_items(usb_hid_report_item_t *report_item);

usb_hid_report_item_t *usb_hid_report_item_clone(
    const usb_hid_report_item_t *item);

uint32_t usb_hid_report_tag_data_uint32(const uint8_t *data, size_t size);

usb_hid_report_path_t *usb_hid_report_path_try_insert(usb_hid_report_t *report,
    usb_hid_report_path_t *cmp_path);

#endif
/**
 * @}
 */
