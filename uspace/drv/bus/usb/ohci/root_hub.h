/*
 * Copyright (c) 2011 Jan Vesely
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
/** @addtogroup drvusbohci
 * @{
 */
/** @file
 * @brief OHCI driver
 */
#ifndef DRV_OHCI_ROOT_HUB_H
#define DRV_OHCI_ROOT_HUB_H

#include <usb/usb.h>
#include <usb/dev/driver.h>
#include <usb/host/usb_transfer_batch.h>

#include "ohci_regs.h"

#define HUB_DESCRIPTOR_MAX_SIZE (7 + 2 + 2)

/**
 * ohci root hub representation
 */
typedef struct rh {
	fibril_mutex_t guard;
	/** pointer to ohci driver registers */
	ohci_regs_t *registers;
	/** usb address of the root hub */
	usb_address_t address;
	/** hub port count */
	size_t port_count;
	/** interrupt transfer waiting for an actual interrupt to occur */
	usb_transfer_batch_t *unfinished_interrupt_transfer;
	/** size of interrupt buffer */
	size_t interrupt_mask_size;
	/** Descriptors */
	struct {
		usb_standard_configuration_descriptor_t configuration;
		usb_standard_interface_descriptor_t interface;
		usb_standard_endpoint_descriptor_t endpoint;
		uint8_t hub[HUB_DESCRIPTOR_MAX_SIZE];
	} __attribute__ ((packed)) descriptors;
	/** size of hub descriptor */
	size_t hub_descriptor_size;
} rh_t;

void rh_init(rh_t *instance, ohci_regs_t *regs);

void rh_request(rh_t *instance, usb_transfer_batch_t *request);

void rh_interrupt(rh_t *instance);
#endif
/**
 * @}
 */
