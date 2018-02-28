/*
 * Copyright (c) 2011 Jan Vesely
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

#ifndef LIBUSBHOST_HOST_HCD_H
#define LIBUSBHOST_HOST_HCD_H

#include <ddf/driver.h>
#include <usb/request.h>

typedef struct hw_resource_list_parsed hw_res_list_parsed_t;
typedef struct bus bus_t;
typedef struct device device_t;

/* Treat this header as read-only in driver code.
 * It could be opaque, but why to complicate matters.
 */
typedef struct hc_device {
	/* Bus instance */
	bus_t *bus;

	/* Managed DDF device */
	ddf_dev_t *ddf_dev;

	/* Control function */
	ddf_fun_t *ctl_fun;

	/* Result of enabling HW IRQs */
	int irq_cap;

	/** Interrupt replacement fibril */
	fid_t polling_fibril;

	/* This structure is meant to be extended by driver code. */
} hc_device_t;

typedef struct hc_driver {
	const char *name;

	/** Size of the device data to be allocated, and passed as the
	 * hc_device_t. */
	size_t hc_device_size;

	/** Initialize device structures. */
	int (*hc_add)(hc_device_t *, const hw_res_list_parsed_t *);

	/** Generate IRQ code to handle interrupts. */
	int (*irq_code_gen)(irq_code_t *, hc_device_t *,
	    const hw_res_list_parsed_t *, int *);

	/** Claim device from BIOS. */
	int (*claim)(hc_device_t *);

	/** Start the host controller. */
	int (*start)(hc_device_t *);

	/** Setup the virtual roothub. */
	int (*setup_root_hub)(hc_device_t *);

	/** Stop the host controller (after start has been called) */
	int (*stop)(hc_device_t *);

	/** HC was asked to be removed (after hc_add has been called) */
	int (*hc_remove)(hc_device_t *);

	/** HC is gone. */
	int (*hc_gone)(hc_device_t *);
} hc_driver_t;

/* Drivers should call this before leaving hc_add */
static inline void hc_device_setup(hc_device_t *hcd, bus_t *bus)
{
	hcd->bus = bus;
}

static inline hc_device_t *dev_to_hcd(ddf_dev_t *dev)
{
	return ddf_dev_data_get(dev);
}

extern errno_t hc_driver_main(const hc_driver_t *);

#endif

/**
 * @}
 */
