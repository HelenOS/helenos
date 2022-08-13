/*
 * SPDX-FileCopyrightText: 2011 Vojtech Horky
 *
 * SPDX-License-Identifier: BSD-3-Clause
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
