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

#include <ddf/driver.h>
#include <ddf/log.h>
#include <pci_dev_iface.h>

static bool check_bar(virtio_dev_t *vdev, uint8_t bar, uint32_t offset,
    uint32_t length)
{
	/* We must ignore the capability if bar is greater than 5 */
	if (bar >= PCI_BAR_COUNT)
		return false;

	/* This is not a mapped BAR */
	if (!vdev->bar[bar].mapped)
		return false;

	uintptr_t start = (uintptr_t) vdev->bar[bar].mapped_base;
	if (start + offset < start)
		return false;
	if (start + offset > start + vdev->bar[bar].mapped_size)
		return false;
	if (start + offset + length < start + offset)
		return false;
	if (start + offset + length > start + vdev->bar[bar].mapped_size)
		return false;

	return true;
}

static void virtio_pci_common_cfg(virtio_dev_t *vdev, uint8_t bar,
    uint32_t offset, uint32_t length)
{
	if (vdev->common_cfg)
		return;

	if (!check_bar(vdev, bar, offset, length))
		return;

	vdev->common_cfg = vdev->bar[bar].mapped_base + offset;

	ddf_msg(LVL_NOTE, "common_cfg=%p", vdev->common_cfg);
}

static void virtio_pci_notify_cfg(virtio_dev_t *vdev, uint8_t bar,
    uint32_t offset, uint32_t length, uint32_t multiplier)
{
	if (vdev->notify_base)
		return;

	if (!check_bar(vdev, bar, offset, length))
		return;

	vdev->notify_base = vdev->bar[bar].mapped_base + offset;
	vdev->notify_off_multiplier = multiplier;

	ddf_msg(LVL_NOTE, "notify_base=%p, off_multiplier=%u",
	    vdev->notify_base, vdev->notify_off_multiplier);
}

static void virtio_pci_isr_cfg(virtio_dev_t *vdev, uint8_t bar, uint32_t offset,
    uint32_t length)
{
	if (vdev->isr)
		return;

	if (!check_bar(vdev, bar, offset, length))
		return;

	vdev->isr = vdev->bar[bar].mapped_base + offset;
	vdev->isr_phys = vdev->bar[bar].phys_base + offset;

	ddf_msg(LVL_NOTE, "isr=%p (phys=%#" PRIxn ")", vdev->isr,
	    vdev->isr_phys);
}

static void virtio_pci_device_cfg(virtio_dev_t *vdev, uint8_t bar,
    uint32_t offset, uint32_t length)
{
	if (vdev->device_cfg)
		return;

	if (!check_bar(vdev, bar, offset, length))
		return;

	vdev->device_cfg = vdev->bar[bar].mapped_base + offset;

	ddf_msg(LVL_NOTE, "device_cfg=%p", vdev->device_cfg);
}

static errno_t enable_resources(async_sess_t *pci_sess, virtio_dev_t *vdev)
{
	pio_window_t pio_window;
	errno_t rc = pio_window_get(pci_sess, &pio_window);
	if (rc != EOK)
		return rc;

	hw_resource_list_t hw_res;
	rc = hw_res_get_resource_list(pci_sess, &hw_res);
	if (rc != EOK)
		return rc;

	/*
	 * Enable resources and reconstruct the mapping between BAR and resource
	 * indices. We are going to need this later when the VIRTIO PCI
	 * capabilities refer to specific BARs.
	 *
	 * XXX: The mapping should probably be provided by the PCI driver
	 *      itself.
	 */
	for (unsigned i = 0, j = 0; i < PCI_BAR_COUNT && j < hw_res.count;
	    i++) {
		/* Detect and skip unused BARs */
		uint32_t bar;
		rc = pci_config_space_read_32(pci_sess,
		    PCI_BAR0 + i * sizeof(uint32_t), &bar);
		if (rc != EOK)
			return rc;
		if (!bar)
			continue;

		hw_resource_t *res = &hw_res.resources[j];
		rc = pio_enable_resource(&pio_window, res,
		    &vdev->bar[i].mapped_base, &vdev->bar[i].phys_base,
		    &vdev->bar[i].mapped_size);
		if (rc == EOK)
			vdev->bar[i].mapped = true;
		j++;
	}

	return rc;
}

static errno_t disable_resources(virtio_dev_t *vdev)
{
	for (unsigned i = 0; i < PCI_BAR_COUNT; i++) {
		if (vdev->bar[i].mapped) {
			errno_t rc = pio_disable(vdev->bar[i].mapped_base,
			    vdev->bar[i].mapped_size);
			if (rc != EOK)
				return rc;
			vdev->bar[i].mapped = false;
		}
	}

	return EOK;
}

errno_t virtio_pci_dev_initialize(ddf_dev_t *dev, virtio_dev_t *vdev)
{
	memset(vdev, 0, sizeof(virtio_dev_t));

	async_sess_t *pci_sess = ddf_dev_parent_sess_get(dev);
	if (!pci_sess)
		return ENOENT;

	errno_t rc = enable_resources(pci_sess, vdev);
	if (rc != EOK)
		goto error;

	/*
	 * Find the VIRTIO PCI Capabilities
	 */
	uint8_t c;
	uint8_t cap_vndr;
	for (rc = pci_config_space_cap_first(pci_sess, &c, &cap_vndr);
	    (rc == EOK) && c;
	    rc = pci_config_space_cap_next(pci_sess, &c, &cap_vndr)) {
		if (cap_vndr != PCI_CAP_VENDORSPECID)
			continue;

		uint8_t cap_len;
		rc = pci_config_space_read_8(pci_sess,
		    VIRTIO_PCI_CAP_CAP_LEN(c), &cap_len);
		if (rc != EOK)
			goto error;

		if (cap_len < VIRTIO_PCI_CAP_END(0)) {
			rc = EINVAL;
			goto error;
		}

		uint8_t cfg_type;
		rc = pci_config_space_read_8(pci_sess,
		    VIRTIO_PCI_CAP_CFG_TYPE(c), &cfg_type);
		if (rc != EOK)
			goto error;

		uint8_t bar;
		rc = pci_config_space_read_8(pci_sess, VIRTIO_PCI_CAP_BAR(c),
		    &bar);
		if (rc != EOK)
			goto error;

		uint32_t offset;
		rc = pci_config_space_read_32(pci_sess,
		    VIRTIO_PCI_CAP_OFFSET(c), &offset);
		if (rc != EOK)
			goto error;

		uint32_t length;
		rc = pci_config_space_read_32(pci_sess,
		    VIRTIO_PCI_CAP_LENGTH(c), &length);
		if (rc != EOK)
			goto error;

		uint32_t multiplier;
		switch (cfg_type) {
		case VIRTIO_PCI_CAP_COMMON_CFG:
			virtio_pci_common_cfg(vdev, bar, offset, length);
			break;
		case VIRTIO_PCI_CAP_NOTIFY_CFG:
			if (cap_len < VIRTIO_PCI_CAP_END(sizeof(uint32_t))) {
				rc = EINVAL;
				goto error;
			}
			rc = pci_config_space_read_32(pci_sess,
			    VIRTIO_PCI_CAP_END(c), &multiplier);
			if (rc != EOK)
				goto error;
			virtio_pci_notify_cfg(vdev, bar, offset, length,
			    multiplier);
			break;
		case VIRTIO_PCI_CAP_ISR_CFG:
			virtio_pci_isr_cfg(vdev, bar, offset, length);
			break;
		case VIRTIO_PCI_CAP_DEVICE_CFG:
			virtio_pci_device_cfg(vdev, bar, offset, length);
			break;
		case VIRTIO_PCI_CAP_PCI_CFG:
			break;
		default:
			break;
		}
	}

	/* Check that the configuration is complete */
	if (!vdev->common_cfg || !vdev->notify_base || !vdev->isr ||
	    !vdev->device_cfg) {
		rc = EINVAL;
		goto error;
	}

	return rc;

error:
	(void) disable_resources(vdev);
	return rc;
}

errno_t virtio_pci_dev_cleanup(virtio_dev_t *vdev)
{
	if (vdev->queues) {
		for (unsigned i = 0;
		    i < pio_read_le16(&vdev->common_cfg->num_queues); i++)
			virtio_virtq_teardown(vdev, i);
		free(vdev->queues);
	}
	return disable_resources(vdev);
}

/** @}
 */
