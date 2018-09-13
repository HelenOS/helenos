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
