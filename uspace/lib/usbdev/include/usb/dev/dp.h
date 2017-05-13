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
 * USB descriptor parser.
 */
#ifndef LIBUSBDEV_DP_H_
#define LIBUSBDEV_DP_H_

#include <stddef.h>
#include <stdint.h>

/** USB descriptors nesting.
 * The nesting describes the logical tree USB descriptors form
 * (e.g. that endpoint descriptor belongs to interface or that
 * interface belongs to configuration).
 *
 * See usb_descriptor_type_t for descriptor constants.
 */
typedef struct {
	/** Child descriptor id. */
	int child;
	/** Parent descriptor id. */
	int parent;
} usb_dp_descriptor_nesting_t;

extern const usb_dp_descriptor_nesting_t usb_dp_standard_descriptor_nesting[];

/** Descriptor parser structure. */
typedef struct {
	/** Used descriptor nesting. */
	const usb_dp_descriptor_nesting_t *nesting;
} usb_dp_parser_t;

/** Descriptor parser data. */
typedef struct {
	/** Data to be parsed. */
	const uint8_t *data;
	/** Size of input data in bytes. */
	size_t size;
	/** Custom argument. */
	void *arg;
} usb_dp_parser_data_t;

typedef void (*walk_callback_t)(const uint8_t *, size_t, void *);

const uint8_t *usb_dp_get_nested_descriptor(const usb_dp_parser_t *,
    const usb_dp_parser_data_t *, const uint8_t *);
const uint8_t *usb_dp_get_sibling_descriptor(const usb_dp_parser_t *,
    const usb_dp_parser_data_t *, const uint8_t *, const uint8_t *);

void usb_dp_walk_simple(const uint8_t *, size_t,
    const usb_dp_descriptor_nesting_t *, walk_callback_t, void *);

#endif
/**
 * @}
 */
