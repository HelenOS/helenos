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

/** @addtogroup libusb
 * @{
 */
/** @file
 * @brief USB descriptor parser.
 */
#ifndef LIBUSB_DP_H_
#define LIBUSB_DP_H_

#include <sys/types.h>
#include <usb/usb.h>
#include <usb/descriptor.h>

typedef struct {
	int child;
	int parent;
} usb_dp_descriptor_nesting_t;

typedef struct {
	usb_dp_descriptor_nesting_t *nesting;
} usb_dp_parser_t;

typedef struct {
	uint8_t *data;
	size_t size;
	void *arg;
} usb_dp_parser_data_t;

uint8_t *usb_dp_get_nested_descriptor(usb_dp_parser_t *,
    usb_dp_parser_data_t *, uint8_t *);
uint8_t *usb_dp_get_sibling_descriptor(usb_dp_parser_t *,
    usb_dp_parser_data_t *, uint8_t *, uint8_t *);

#endif
/**
 * @}
 */
