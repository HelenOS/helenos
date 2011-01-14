/*
 * Copyright (c) 2010 Lubos Slovak
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
#ifndef USBHID_DESCDUMP_H_
#define USBHID_DESCDUMP_H_

#include <usb/classes/hid.h>

void dump_standard_configuration_descriptor(
    int index, const usb_standard_configuration_descriptor_t *d);

void dump_standard_interface_descriptor(
    const usb_standard_interface_descriptor_t *d);

void dump_standard_endpoint_descriptor(
    const usb_standard_endpoint_descriptor_t *d);

void dump_standard_hid_descriptor_header(
    const usb_standard_hid_descriptor_t *d);

void dump_standard_hid_class_descriptor_info(
    const usb_standard_hid_class_descriptor_info_t *d);

void dump_hid_class_descriptor(int index, uint8_t type, 
    const uint8_t *d, size_t size);

#endif /* USBHID_DESCDUMP_H_ */
