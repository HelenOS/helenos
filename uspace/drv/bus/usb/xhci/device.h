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
/** @addtogroup libusbhost
 * @{
 */
/** @file
 *
 */
#ifndef XHCI_DEVICE_H
#define XHCI_DEVICE_H

#include <usb/host/bus.h>
#include <usb/dma_buffer.h>

typedef struct xhci_slot_ctx xhci_slot_ctx_t;

typedef struct xhci_device {
	device_t base;		/**< Inheritance. Keep this first. */

	/** Slot ID assigned to the device by xHC. */
	uint32_t slot_id;

	/** Corresponding port on RH */
	uint8_t rh_port;

	/** Route string */
	uint32_t route_str;

	/** Place to store the allocated context */
	dma_buffer_t dev_ctx;

	/** Hub specific information. Valid only if the device is_hub. */
	bool is_hub;
	uint8_t num_ports;
	uint8_t tt_think_time;
} xhci_device_t;

#define XHCI_DEV_FMT  "(%s, slot %d)"
#define XHCI_DEV_ARGS(dev)	ddf_fun_get_name((dev).base.fun), (dev).slot_id

/* Bus callbacks */
errno_t xhci_device_enumerate(device_t *);
void xhci_device_offline(device_t *);
errno_t xhci_device_online(device_t *);
void xhci_device_gone(device_t *);

void xhci_setup_slot_context(xhci_device_t *, xhci_slot_ctx_t *);

static inline xhci_device_t *xhci_device_get(device_t *dev)
{
	assert(dev);
	return (xhci_device_t *) dev;
}

#endif
/**
 * @}
 */
