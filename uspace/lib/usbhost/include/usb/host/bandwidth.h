/*
 * SPDX-FileCopyrightText: 2011 Jan Vesely
 * SPDX-FileCopyrightText: 2018 Ondrej Hlavaty
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
/**  @addtogroup libusbhost
 * @{
 */
/** @file
 *
 * Bandwidth calculation functions. Shared among uhci, ohci and ehci drivers.
 */

#ifndef LIBUSBHOST_HOST_BANDWIDTH_H
#define LIBUSBHOST_HOST_BANDWIDTH_H

#include <usb/usb.h>

#include <stddef.h>

typedef struct endpoint endpoint_t;

typedef size_t (*endpoint_count_bw_t)(endpoint_t *);

typedef struct {
	size_t available_bandwidth;
	endpoint_count_bw_t count_bw;
} bandwidth_accounting_t;

extern const bandwidth_accounting_t bandwidth_accounting_usb11;
extern const bandwidth_accounting_t bandwidth_accounting_usb2;

#endif
/**
 * @}
 */
