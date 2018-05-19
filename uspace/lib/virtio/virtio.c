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

/** @file VIRTIO support
 */

#include "virtio-pci.h"

#include <as.h>
#include <align.h>
#include <macros.h>

#include <ddf/log.h>

errno_t virtio_virtq_setup(virtio_dev_t *vdev, uint16_t num, uint16_t size)
{
	virtq_t *q = &vdev->queues[num];
	virtio_pci_common_cfg_t *cfg = vdev->common_cfg;

	/* Program the queue of our interest */
	pio_write_16(&cfg->queue_select, num);

	/* Trim the size of the queue as needed */
	size = min(pio_read_16(&cfg->queue_size), size);
	pio_write_16(&cfg->queue_size, size);
	ddf_msg(LVL_NOTE, "Virtq %u: %u buffers", num, (unsigned) size);

	size_t avail_offset = 0;
	size_t used_offset = 0;

	/*
	 * Compute the size of the needed DMA memory and also the offsets of
	 * the individual components
	 */
	size_t mem_size = sizeof(virtq_desc_t[size]);
	mem_size = ALIGN_UP(mem_size, _Alignof(virtq_avail_t));
	avail_offset = mem_size;
	mem_size += sizeof(virtq_avail_t) + sizeof(ioport16_t[size]) +
	    sizeof(ioport16_t);
	mem_size = ALIGN_UP(mem_size, _Alignof(virtq_used_t));
	used_offset = mem_size;
	mem_size += sizeof(virtq_used_t) + sizeof(virtq_used_elem_t[size]) +
	    sizeof(ioport16_t);

	/*
	 * Allocate DMA memory for the virtqueues
	 */
	q->virt = AS_AREA_ANY;
	errno_t rc = dmamem_map_anonymous(mem_size, DMAMEM_4GiB,
	    AS_AREA_READ | AS_AREA_WRITE, 0, &q->phys, &q->virt);
	if (rc != EOK) {
		q->virt = NULL;
		return rc;
	}

	q->size = mem_size;
	q->queue_size = size;
	q->desc = q->virt;
	q->avail = q->virt + avail_offset;
	q->used = q->virt + used_offset;

	memset(q->virt, 0, q->size);

	/*
	 * Initialize the descriptor table
	 */
	for (unsigned i = 0; i < size; i++) {
		q->desc[i].addr = 0;
		q->desc[i].len = 0;
		q->desc[i].flags = 0;
	}

	/*
	 * Write the configured addresses to device's common config
	 */
	pio_write_64(&cfg->queue_desc, q->phys);
	pio_write_64(&cfg->queue_avail, q->phys + avail_offset);
	pio_write_64(&cfg->queue_used, q->phys + used_offset);

	ddf_msg(LVL_NOTE, "DMA memory for virtq %d: virt=%p, phys=%p, size=%zu",
	    num, q->virt, (void *) q->phys, q->size);

	/* Determine virtq's notification address */
	q->notify = vdev->notify_base +
	    pio_read_16(&cfg->queue_notif_off) * vdev->notify_off_multiplier;

	ddf_msg(LVL_NOTE, "notification register: %p", q->notify);

	return rc;
}

void virtio_virtq_teardown(virtio_dev_t *vdev, uint16_t num)
{
	virtq_t *q = &vdev->queues[num];
	if (q->size)
		dmamem_unmap_anonymous(q->virt);
}

/**
 * Perform device initialization as described in section 3.1.1 of the
 * specification, steps 1 - 6.
 */
errno_t virtio_device_setup_start(virtio_dev_t *vdev, uint32_t features)
{
	virtio_pci_common_cfg_t *cfg = vdev->common_cfg;

	/* 1. Reset the device */
	uint8_t status = VIRTIO_DEV_STATUS_RESET;
	pio_write_8(&cfg->device_status, status);

	/* 2. Acknowledge we found the device */
	status |= VIRTIO_DEV_STATUS_ACKNOWLEDGE;
	pio_write_8(&cfg->device_status, status);

	/* 3. We know how to drive the device */
	status |= VIRTIO_DEV_STATUS_DRIVER;
	pio_write_8(&cfg->device_status, status);

	/* 4. Read the offered feature flags */
	pio_write_32(&cfg->device_feature_select, VIRTIO_FEATURES_0_31);
	uint32_t device_features = pio_read_32(&cfg->device_feature);

	ddf_msg(LVL_NOTE, "offered features %x", device_features);
	features &= device_features;

	if (!features)
		return ENOTSUP;

	/* 4. Write the accepted feature flags */
	pio_write_32(&cfg->driver_feature_select, VIRTIO_FEATURES_0_31);
	pio_write_32(&cfg->driver_feature, features);

	ddf_msg(LVL_NOTE, "accepted features %x", features);

	/* 5. Set FEATURES_OK */
	status |= VIRTIO_DEV_STATUS_FEATURES_OK;
	pio_write_8(&cfg->device_status, status);

	/* 6. Test if the device supports our feature subset */
	status = pio_read_8(&cfg->device_status);
	if (!(status & VIRTIO_DEV_STATUS_FEATURES_OK))
		return ENOTSUP;

	return EOK;
}

/**
 * Perform device initialization as described in section 3.1.1 of the
 * specification, step 8 (go live).
 */
void virtio_device_setup_finalize(virtio_dev_t *vdev)
{
	virtio_pci_common_cfg_t *cfg = vdev->common_cfg;

	/* 8. Go live */
	uint8_t status = pio_read_8(&cfg->device_status);
	pio_write_8(&cfg->device_status, status | VIRTIO_DEV_STATUS_DRIVER_OK);
}

void virtio_device_setup_fail(virtio_dev_t *vdev)
{
	virtio_pci_common_cfg_t *cfg = vdev->common_cfg;

	uint8_t status = pio_read_8(&cfg->device_status);
	pio_write_8(&cfg->device_status, status | VIRTIO_DEV_STATUS_FAILED);
}

/** @}
 */
