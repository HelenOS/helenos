/*
 * Copyright (c) 2011 Matej Klonfar
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
 * USB HID report descriptor and report data parser
 */
#ifndef LIBUSB_HIDPATH_H_
#define LIBUSB_HIDPATH_H_

#include <usb/classes/hidparser.h>
#include <stdint.h>
#include <adt/list.h>

/**
 * Description of path of usage pages and usages in report descriptor
 */
#define USB_HID_PATH_COMPARE_STRICT				0
#define USB_HID_PATH_COMPARE_END				1
#define USB_HID_PATH_COMPARE_USAGE_PAGE_ONLY	4
#define USB_HID_PATH_COMPARE_COLLECTION_ONLY	2 /* porovnava jenom cestu z Kolekci */


/** Collection usage path structure */
typedef struct {
	/** */
	uint32_t usage_page;
	/** */	
	uint32_t usage;

	uint8_t flags;
	/** */
	link_t link;
} usb_hid_report_usage_path_t;

/** */
typedef struct {
	/** */	
	int depth;	
	uint8_t report_id;
	
	/** */	
	link_t link; /* list */

	link_t head; /* head of list of usage paths */

} usb_hid_report_path_t;

/** */
usb_hid_report_path_t *usb_hid_report_path(void);

/** */
void usb_hid_report_path_free(usb_hid_report_path_t *path);

/** */
int usb_hid_report_path_set_report_id(usb_hid_report_path_t *usage_path, 
                                      uint8_t report_id);

/** */
int usb_hid_report_path_append_item(usb_hid_report_path_t *usage_path, 
                                    int32_t usage_page, int32_t usage);

/** */
void usb_hid_report_remove_last_item(usb_hid_report_path_t *usage_path);

/** */
void usb_hid_report_null_last_item(usb_hid_report_path_t *usage_path);

/** */
void usb_hid_report_set_last_item(usb_hid_report_path_t *usage_path, 
                                  int32_t tag, int32_t data);

/** */
int usb_hid_report_compare_usage_path(usb_hid_report_path_t *report_path, 
                                      usb_hid_report_path_t *path, int flags);

/** */
usb_hid_report_path_t *usb_hid_report_path_clone(usb_hid_report_path_t *usage_path);

void usb_hid_print_usage_path(usb_hid_report_path_t *path);

#endif
/**
 * @}
 */
