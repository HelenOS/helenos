/*
 * SPDX-FileCopyrightText: 2018 Ondrej Hlavaty
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
/** @addtogroup drvusbehci
 * @{
 */
/** @file
 * @brief USB host controller library: Utility functions.
 *
 * A mix of various functions that makes life of USB HC driver developers
 * easier.
 */
#ifndef LIB_USBHOST_UTILITY
#define LIB_USBHOST_UTILITY

#include <usb/host/bus.h>
#include <usb/host/usb_transfer_batch.h>
#include <usb/descriptor.h>
#include <usb/classes/hub.h>
#include <usb/request.h>

typedef void (*endpoint_reset_toggle_t)(endpoint_t *);

uint16_t hc_get_ep0_initial_mps(usb_speed_t);
int hc_get_ep0_max_packet_size(uint16_t *, device_t *);
void hc_reset_toggles(const usb_transfer_batch_t *batch, endpoint_reset_toggle_t);
int hc_setup_virtual_root_hub(hc_device_t *, usb_speed_t);
int hc_get_device_desc(device_t *, usb_standard_device_descriptor_t *);
int hc_get_hub_desc(device_t *, usb_hub_descriptor_header_t *);
int hc_device_explore(device_t *);

/** Joinable fibril */

typedef int (*fibril_worker_t)(void *);
typedef struct joinable_fibril joinable_fibril_t;

joinable_fibril_t *joinable_fibril_create(fibril_worker_t, void *);
void joinable_fibril_start(joinable_fibril_t *);
void joinable_fibril_join(joinable_fibril_t *);
void joinable_fibril_destroy(joinable_fibril_t *);
errno_t joinable_fibril_recreate(joinable_fibril_t *);

#endif
/**
 * @}
 */
