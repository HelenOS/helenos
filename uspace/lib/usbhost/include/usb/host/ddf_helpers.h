/*
 * SPDX-FileCopyrightText: 2012 Jan Vesely
 * SPDX-FileCopyrightText: 2018 Ondrej Hlavaty
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup libusbhost
 * @{
 */
/** @file
 *
 */

#ifndef LIBUSBHOST_HOST_DDF_HELPERS_H
#define LIBUSBHOST_HOST_DDF_HELPERS_H

#include <ddf/driver.h>
#include <ddf/interrupt.h>
#include <usb/usb.h>

#include <usb/host/hcd.h>
#include <usb/descriptor.h>

extern errno_t hcd_ddf_setup_hc(ddf_dev_t *, size_t);
extern void hcd_ddf_clean_hc(hc_device_t *);

extern device_t *hcd_ddf_fun_create(hc_device_t *, usb_speed_t);
extern void hcd_ddf_fun_destroy(device_t *);

extern errno_t hcd_ddf_setup_match_ids(device_t *,
    usb_standard_device_descriptor_t *);

extern errno_t hcd_ddf_enable_interrupt(hc_device_t *, int);
extern errno_t hcd_ddf_get_registers(hc_device_t *, hw_res_list_parsed_t *);

#endif

/**
 * @}
 */
