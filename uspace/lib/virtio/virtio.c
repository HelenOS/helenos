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
#include <stdalign.h>

#include <ddf/log.h>
#include <barrier.h>

/** Allocate DMA buffers
 *
 * @param buffers[in]  Number of buffers to allocate.
 * @param size[in]     Size of each buffer.
 * @param write[in]    True if the buffers are writable by the driver, false
 *                     otherwise.
 * @param buf[out]     Output array holding virtual addresses of the allocated
 *                     buffers.
 * @param buf_p[out]   Output array holding physical addresses of the allocated
 *                     buffers.
 *
 * The buffers can be deallocated by virtio_teardown_dma_bufs().
 *
 * @return  EOK on success or error code.
 */
errno_t virtio_setup_dma_bufs(unsigned int buffers, size_t size,
    bool write, void *buf[], uintptr_t buf_p[])
{
	/*
	 * Allocate all buffers at once in one large chunk.
	 */
	void *virt = AS_AREA_ANY;
	uintptr_t phys;
	errno_t rc = dmamem_map_anonymous(buffers * size, 0,
	    write ? AS_AREA_WRITE | AS_AREA_READ : AS_AREA_READ, 0, &phys,
	    &virt);
	if (rc != EOK)
		return rc;

	ddf_msg(LVL_NOTE, "DMA buffers: %p-%p", virt, virt + buffers * size);

	/*
	 * Calculate addresses of the individual buffers for easy access.
	 */
	for (unsigned i = 0; i < buffers; i++) {
		buf[i] = virt + i * size;
		buf_p[i] = phys + i * size;
	}

	return EOK;
}

/** Deallocate DMA buffers
 *
 * @param buf[in]  Array holding the virtual addresses of the DMA buffers
 *                 previously allocated by virtio_setup_dma_bufs().
 */
void virtio_teardown_dma_bufs(void *buf[])
{
	if (buf[0]) {
		dmamem_unmap_anonymous(buf[0]);
		buf[0] = NULL;
	}
}

void virtio_virtq_desc_set(virtio_dev_t *vdev, uint16_t num, uint16_t descno,
    uint64_t addr, uint32_t len, uint16_t flags, uint16_t next)
{
	virtq_desc_t *d = &vdev->queues[num].desc[descno];
	pio_write_le64(&d->addr, addr);
	pio_write_le32(&d->len, len);
	pio_write_le16(&d->flags, flags);
	pio_write_le16(&d->next, next);
}

uint16_t virtio_virtq_desc_get_next(virtio_dev_t *vdev, uint16_t num,
    uint16_t descno)
{
	virtq_desc_t *d = &vdev->queues[num].desc[descno];
	if (!(pio_read_le16(&d->flags) & VIRTQ_DESC_F_NEXT))
		return (uint16_t) -1U;
	return pio_read_le16(&d->next);
}

/** Create free descriptor list from the unused VIRTIO descriptors
 *
 * @param vdev[in]   VIRTIO device for which the free list will be created.
 * @param num[in]    Index of the virtqueue for which the free list will be
 *                   created.
 * @param size[in]   Number of descriptors on the free list. The free list will
 *                   contain descriptors starting from 0 to \a size - 1.
 * @param head[out]  Variable that will hold the VIRTIO descriptor at the head
 *                   of the free list.
 */
void virtio_create_desc_free_list(virtio_dev_t *vdev, uint16_t num,
    uint16_t size, uint16_t *head)
{
	for (unsigned i = 0; i < size; i++) {
		virtio_virtq_desc_set(vdev, num, i, 0, 0,
		    VIRTQ_DESC_F_NEXT, (i + 1 == size) ? 0xffffu : i + 1);
	}
	*head = 0;
}

/** Allocate a descriptor from the free list
 *
 * @param vdev[in]      VIRTIO device with the free list.
 * @param num[in]       Index of the virtqueue with free list.
 * @param head[in,out]  Head of the free list.
 *
 * @return  Allocated descriptor or 0xFFFF if the list is empty.
 */
uint16_t virtio_alloc_desc(virtio_dev_t *vdev, uint16_t num, uint16_t *head)
{
	virtq_t *q = &vdev->queues[num];
	fibril_mutex_lock(&q->lock);
	uint16_t descno = *head;
	if (descno != (uint16_t) -1U)
		*head = virtio_virtq_desc_get_next(vdev, num, descno);
	fibril_mutex_unlock(&q->lock);
	return descno;
}

/** Free a descriptor into the free list
 *
 * @param vdev[in]      VIRTIO device with the free list.
 * @param num[in]       Index of the virtqueue with free list.
 * @param head[in,out]  Head of the free list.
 * @param descno[in]    The freed descriptor.
 */
void virtio_free_desc(virtio_dev_t *vdev, uint16_t num, uint16_t *head,
    uint16_t descno)
{
	virtq_t *q = &vdev->queues[num];
	fibril_mutex_lock(&q->lock);
	virtio_virtq_desc_set(vdev, num, descno, 0, 0, VIRTQ_DESC_F_NEXT,
	    *head);
	*head = descno;
	fibril_mutex_unlock(&q->lock);
}

void virtio_virtq_produce_available(virtio_dev_t *vdev, uint16_t num,
    uint16_t descno)
{
	virtq_t *q = &vdev->queues[num];

	fibril_mutex_lock(&q->lock);
	uint16_t idx = pio_read_le16(&q->avail->idx);
	pio_write_le16(&q->avail->ring[idx % q->queue_size], descno);
	write_barrier();
	pio_write_le16(&q->avail->idx, idx + 1);
	write_barrier();
	pio_write_le16(q->notify, num);
	fibril_mutex_unlock(&q->lock);
}

bool virtio_virtq_consume_used(virtio_dev_t *vdev, uint16_t num,
    uint16_t *descno, uint32_t *len)
{
	virtq_t *q = &vdev->queues[num];

	fibril_mutex_lock(&q->lock);
	uint16_t last_idx = q->used_last_idx % q->queue_size;
	if (last_idx == (pio_read_le16(&q->used->idx) % q->queue_size)) {
		fibril_mutex_unlock(&q->lock);
		return false;
	}

	*descno = (uint16_t) pio_read_le32(&q->used->ring[last_idx].id);
	*len = pio_read_le32(&q->used->ring[last_idx].len);

	q->used_last_idx++;
	fibril_mutex_unlock(&q->lock);

	return true;
}

errno_t virtio_virtq_setup(virtio_dev_t *vdev, uint16_t num, uint16_t size)
{
	virtq_t *q = &vdev->queues[num];
	virtio_pci_common_cfg_t *cfg = vdev->common_cfg;

	/* Program the queue of our interest */
	pio_write_le16(&cfg->queue_select, num);

	/* Trim the size of the queue as needed */
	if (size > pio_read_16(&cfg->queue_size)) {
		ddf_msg(LVL_ERROR, "Virtq %u: not enough descriptors", num);
		return ENOMEM;
	}
	pio_write_le16(&cfg->queue_size, size);
	ddf_msg(LVL_NOTE, "Virtq %u: %u descriptors", num, (unsigned) size);

	size_t avail_offset = 0;
	size_t used_offset = 0;

	/*
	 * Compute the size of the needed DMA memory and also the offsets of
	 * the individual components
	 */
	size_t mem_size = sizeof(virtq_desc_t[size]);
	mem_size = ALIGN_UP(mem_size, alignof(virtq_avail_t));
	avail_offset = mem_size;
	mem_size += sizeof(virtq_avail_t) + sizeof(ioport16_t[size]) +
	    sizeof(ioport16_t);
	mem_size = ALIGN_UP(mem_size, alignof(virtq_used_t));
	used_offset = mem_size;
	mem_size += sizeof(virtq_used_t) + sizeof(virtq_used_elem_t[size]) +
	    sizeof(ioport16_t);

	/*
	 * Allocate DMA memory for the virtqueues
	 */
	q->virt = AS_AREA_ANY;
	errno_t rc = dmamem_map_anonymous(mem_size, 0,
	    AS_AREA_READ | AS_AREA_WRITE, 0, &q->phys, &q->virt);
	if (rc != EOK) {
		q->virt = NULL;
		return rc;
	}

	fibril_mutex_initialize(&q->lock);

	q->size = mem_size;
	q->queue_size = size;
	q->desc = q->virt;
	q->avail = q->virt + avail_offset;
	q->used = q->virt + used_offset;
	q->used_last_idx = 0;

	memset(q->virt, 0, q->size);

	/*
	 * Write the configured addresses to device's common config
	 */
	pio_write_le64(&cfg->queue_desc, q->phys);
	pio_write_le64(&cfg->queue_avail, q->phys + avail_offset);
	pio_write_le64(&cfg->queue_used, q->phys + used_offset);

	ddf_msg(LVL_NOTE, "DMA memory for virtq %d: virt=%p, phys=%p, size=%zu",
	    num, q->virt, (void *) q->phys, q->size);

	/* Determine virtq's notification address */
	q->notify = vdev->notify_base +
	    pio_read_le16(&cfg->queue_notif_off) * vdev->notify_off_multiplier;

	ddf_msg(LVL_NOTE, "notification register: %p", q->notify);

	/* Enable the queue */
	pio_write_le16(&cfg->queue_enable, 1);
	ddf_msg(LVL_NOTE, "virtq %d set", num);

	return rc;
}

void virtio_virtq_teardown(virtio_dev_t *vdev, uint16_t num)
{
	virtio_pci_common_cfg_t *cfg = vdev->common_cfg;

	/* Disable the queue */
	pio_write_le16(&cfg->queue_enable, 0);

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
	pio_write_le32(&cfg->device_feature_select, VIRTIO_FEATURES_0_31);
	uint32_t device_features = pio_read_le32(&cfg->device_feature);

	uint32_t reserved_features = VIRTIO_F_VERSION_1;
	pio_write_le32(&cfg->device_feature_select, VIRTIO_FEATURES_32_63);
	uint32_t device_reserved_features = pio_read_le32(&cfg->device_feature);

	ddf_msg(LVL_NOTE, "offered features %x, reserved features %x",
	    device_features, device_reserved_features);

	if (features != (features & device_features))
		return ENOTSUP;
	features &= device_features;

	if (reserved_features != (reserved_features & device_reserved_features))
		return ENOTSUP;
	reserved_features &= device_reserved_features;

	/* 4. Write the accepted feature flags */
	pio_write_le32(&cfg->driver_feature_select, VIRTIO_FEATURES_0_31);
	pio_write_le32(&cfg->driver_feature, features);
	pio_write_le32(&cfg->driver_feature_select, VIRTIO_FEATURES_32_63);
	pio_write_le32(&cfg->driver_feature, reserved_features);

	ddf_msg(LVL_NOTE, "accepted features %x, reserved features %x",
	    features, reserved_features);

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
