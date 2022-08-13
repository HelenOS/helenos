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
#ifndef LIBUSB_HIDPATH_H_
#define LIBUSB_HIDPATH_H_

#include <errno.h>
#include <usb/hid/hidparser.h>
#include <stdint.h>
#include <adt/list.h>

/*
 * Flags of usage paths comparison modes.
 *
 */
/** Wanted usage path must be exactly the same as the searched one.  This
 * option cannot be combined with the others.
 */
#define USB_HID_PATH_COMPARE_STRICT		0

/**
 * Wanted usage path must be the suffix in the searched one.
 */
#define USB_HID_PATH_COMPARE_END		1

/**
 * Only usage page are compared along the usage path.  This option can be
 * combined with others.
 */
#define USB_HID_PATH_COMPARE_USAGE_PAGE_ONLY	2

/**
 * Searched usage page must be prefix of the other one.
 */
#define USB_HID_PATH_COMPARE_BEGIN		4

/**
 * Searched couple of usage page and usage can be anywhere in usage path.
 * This option is deprecated.
 */
#define USB_HID_PATH_COMPARE_ANYWHERE		8

/**
 * Item of usage path structure. Last item of linked list describes one item
 * in report, the others describe superior Collection tags. Usage and Usage
 * page of report item can be changed due to data in report.
 */
typedef struct {
	/** Usage page of report item. Zero when usage page can be changed. */
	uint32_t usage_page;
	/** Usage of report item. Zero when usage can be changed. */
	uint32_t usage;

	/** Attribute of Collection tag in report descriptor */
	uint8_t flags;

	/** Link to usb_hid_report_path_t.items list */
	link_t rpath_items_link;
} usb_hid_report_usage_path_t;

/**
 * USB HID usage path structure.
 */
typedef struct {
	/** Length of usage path */
	int depth;

	/** Report id. Zero is reserved and means that report id is not used.
	 */
	uint8_t report_id;

	/** Link to usb_hid_report_path_t.collection_paths list. */
	link_t cpath_link;

	/** List of usage path items. */
	list_t items;	/* of usb_hid_report_usage_path_t */
} usb_hid_report_path_t;

usb_hid_report_path_t *usb_hid_report_path(void);

void usb_hid_report_path_free(usb_hid_report_path_t *path);

errno_t usb_hid_report_path_set_report_id(usb_hid_report_path_t *usage_path,
    uint8_t report_id);

errno_t usb_hid_report_path_append_item(usb_hid_report_path_t *usage_path,
    int32_t usage_page, int32_t usage);

void usb_hid_report_remove_last_item(usb_hid_report_path_t *usage_path);

void usb_hid_report_null_last_item(usb_hid_report_path_t *usage_path);

void usb_hid_report_set_last_item(usb_hid_report_path_t *usage_path,
    int32_t tag, int32_t data);

int usb_hid_report_compare_usage_path(usb_hid_report_path_t *report_path,
    usb_hid_report_path_t *path, int flags);

usb_hid_report_path_t *usb_hid_report_path_clone(
    usb_hid_report_path_t *usage_path);

void usb_hid_print_usage_path(usb_hid_report_path_t *path);

#endif
/**
 * @}
 */
