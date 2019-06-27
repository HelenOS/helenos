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

#ifndef _VIRTIO_PCI_H_
#define _VIRTIO_PCI_H_

#include <ddf/driver.h>
#include <pci_dev_iface.h>
#include <ddi.h>
#include <fibril_synch.h>

#define VIRTIO_PCI_CAP_CAP_LEN(c)	((c) + 2)
#define VIRTIO_PCI_CAP_CFG_TYPE(c)	((c) + 3)
#define VIRTIO_PCI_CAP_BAR(c)		((c) + 4)
#define VIRTIO_PCI_CAP_OFFSET(c)	((c) + 8)
#define VIRTIO_PCI_CAP_LENGTH(c)	((c) + 12)
#define VIRTIO_PCI_CAP_END(c)		((c) + 16)

#define VIRTIO_PCI_CAP_COMMON_CFG	1
#define VIRTIO_PCI_CAP_NOTIFY_CFG	2
#define VIRTIO_PCI_CAP_ISR_CFG		3
#define VIRTIO_PCI_CAP_DEVICE_CFG	4
#define VIRTIO_PCI_CAP_PCI_CFG		5

#define VIRTIO_DEV_STATUS_RESET			0
#define VIRTIO_DEV_STATUS_ACKNOWLEDGE		1
#define VIRTIO_DEV_STATUS_DRIVER		2
#define VIRTIO_DEV_STATUS_DRIVER_OK		4
#define VIRTIO_DEV_STATUS_FEATURES_OK		8
#define VIRTIO_DEV_STATUS_DEVICE_NEEDS_RESET	64
#define VIRTIO_DEV_STATUS_FAILED		128

#define VIRTIO_FEATURES_0_31	0
#define VIRTIO_FEATURES_32_63	1

#define VIRTIO_F_VERSION_1	1

/** Common configuration structure layout according to VIRTIO version 1.0 */
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

/** The buffer continues in the next descriptor */
#define VIRTQ_DESC_F_NEXT	1
/** Device write-only buffer */
#define VIRTQ_DESC_F_WRITE	2
/** Buffer contains a list of buffer descriptors */
#define VIRTQ_DESC_F_INDIRECT	4

/** Virtqueue Descriptor structure as per VIRTIO version 1.0 */
typedef struct virtq_desc {
	ioport64_t addr;	/**< Buffer physical address */
	ioport32_t len;		/**< Buffer length */
	ioport16_t flags;	/**< Buffer flags */
	ioport16_t next;	/**< Continuation descriptor */
} virtq_desc_t;

#define VIRTQ_AVAIL_F_NO_INTERRUPT	1

/** Virtqueue Available Ring as per VIRTIO version 1.0 */
typedef struct virtq_avail {
	ioport16_t flags;
	ioport16_t idx;
	ioport16_t ring[];
	/*
	 * We do not define the optional used_event member here in order to be
	 * able to define ring as a variable-length array.
	 */
} virtq_avail_t;

typedef struct virtq_used_elem {
	ioport32_t id;
	ioport32_t len;
} virtq_used_elem_t;

#define VIRTQ_USED_F_NO_NOTIFY	1

/** Virtqueue Used Ring as per VIRTIO version 1.0 */
typedef struct virtq_used {
	ioport16_t flags;
	ioport16_t idx;
	virtq_used_elem_t ring[];
	/*
	 * We do not define the optional avail_event member here in order to be
	 * able to define ring as a variable-length array.
	 */
} virtq_used_t;

typedef struct {
	void *virt;
	uintptr_t phys;
	size_t size;

	/** Mutex protecting access to this virtqueue */
	fibril_mutex_t lock;

	/**
	 * Size of the queue which determines the number of descriptors and
	 * DMA buffers.
	 */
	size_t queue_size;

	/** Virtual address of queue size virtq descriptors */
	virtq_desc_t *desc;
	/** Virtual address of the available ring */
	virtq_avail_t *avail;
	/** Virtual address of the used ring */
	virtq_used_t *used;
	uint16_t used_last_idx;

	/** Address of the queue's notification register */
	ioport16_t *notify;
} virtq_t;

/** VIRTIO-device specific data associated with the NIC framework nic_t */
typedef struct {
	struct {
		bool mapped;
		uintptr_t phys_base;
		void *mapped_base;
		size_t mapped_size;
	} bar[PCI_BAR_COUNT];

	/** Commong configuration structure */
	virtio_pci_common_cfg_t *common_cfg;

	/** Notification base address */
	void *notify_base;
	/** Notification offset multiplier */
	uint32_t notify_off_multiplier;

	/** INT#x interrupt ISR register */
	ioport8_t *isr;
	uintptr_t isr_phys;

	/** Device-specific configuration */
	void *device_cfg;

	/** Virtqueues */
	virtq_t *queues;
} virtio_dev_t;

extern errno_t virtio_setup_dma_bufs(unsigned int, size_t, bool, void *[],
    uintptr_t []);
extern void virtio_teardown_dma_bufs(void *[]);

extern void virtio_virtq_desc_set(virtio_dev_t *vdev, uint16_t, uint16_t,
    uint64_t, uint32_t, uint16_t, uint16_t);
extern uint16_t virtio_virtq_desc_get_next(virtio_dev_t *vdev, uint16_t,
    uint16_t);

extern void virtio_create_desc_free_list(virtio_dev_t *, uint16_t, uint16_t,
    uint16_t *);
extern uint16_t virtio_alloc_desc(virtio_dev_t *, uint16_t, uint16_t *);
extern void virtio_free_desc(virtio_dev_t *, uint16_t, uint16_t *, uint16_t);

extern void virtio_virtq_produce_available(virtio_dev_t *, uint16_t, uint16_t);
extern bool virtio_virtq_consume_used(virtio_dev_t *, uint16_t, uint16_t *,
    uint32_t *);

extern errno_t virtio_virtq_setup(virtio_dev_t *, uint16_t, uint16_t);
extern void virtio_virtq_teardown(virtio_dev_t *, uint16_t);

extern errno_t virtio_device_setup_start(virtio_dev_t *, uint32_t);
extern void virtio_device_setup_fail(virtio_dev_t *);
extern void virtio_device_setup_finalize(virtio_dev_t *);

extern errno_t virtio_pci_dev_initialize(ddf_dev_t *, virtio_dev_t *);
extern errno_t virtio_pci_dev_cleanup(virtio_dev_t *);

#endif

/** @}
 */
