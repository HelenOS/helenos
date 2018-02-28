/*
 * Copyright (c) 2018 Ondrej Hlavaty
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
