/*
 * Copyright (c) 2018 Jakub Jermar
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

/** @file VIRTIO PCI definitions
 */

#ifndef __VIRTIO_PCI_H__
#define __VIRTIO_PCI_H__

#include <ddf/driver.h>
#include <pci_dev_iface.h>
#include <ddi.h>

#define VIRTIO_PCI_CAP_TYPE(c)		((c) + 3)
#define VIRTIO_PCI_CAP_BAR(c)		((c) + 4)
#define VIRTIO_PCI_CAP_OFFSET(c)	((c) + 8)
#define VIRTIO_PCI_CAP_LENGTH(c)	((c) + 12)
#define VIRTIO_PCI_CAP_END(c)		((c) + 16)

#define VIRTIO_PCI_CAP_COMMON_CFG	1
#define VIRTIO_PCI_CAP_NOTIFY_CFG	2
#define VIRTIO_PCI_CAP_ISR_CFG		3
#define VIRTIO_PCI_CAP_DEVICE_CFG	4
#define VIRTIO_PCI_CAP_PCI_CFG		5

/** Common configuration structure layout according to VIRTIO v1. */
typedef struct virtio_pci_common_cfg {
	ioport32_t device_feature_select;
	const ioport32_t device_feature;
	ioport32_t driver_feature_select;
	ioport32_t driver_feature;
	ioport16_t msix_config;
	const ioport16_t num_queues;
	ioport8_t device_status;
	const ioport8_t config_generation;
	ioport16_t queue_select;
	ioport16_t queue_size;
	ioport16_t queue_msix_vector;
	ioport16_t queue_enable;
	const ioport16_t queue_notif_off;
	ioport64_t queue_desc;
	ioport64_t queue_avail;
	ioport64_t queue_used;
} virtio_pci_common_cfg_t;

typedef struct {
	struct {
		bool mapped;
		void *mapped_base;
	} bar[PCI_BAR_COUNT];

	/** Commong configuration structure */
	virtio_pci_common_cfg_t *common_cfg;

	/** Notification base address */
	ioport16_t *notify_base;
	/** Notification offset multiplier */
	uint32_t notify_off_multiplier;

	/** INT#x interrupt ISR register */
	ioport8_t *isr;

	/** Device-specific configuration */
	void *device_cfg;
} virtio_dev_t;

errno_t virtio_pci_dev_init(ddf_dev_t *, virtio_dev_t *);

#endif

/** @}
 */
