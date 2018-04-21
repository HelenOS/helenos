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

static bool check_bar(virtio_dev_t *vdev, uint8_t bar)
{
	/* We must ignore the capability if bar is greater than 5 */
	if (bar >= PCI_BAR_COUNT)
		return false;

	/* This is not a mapped BAR */
	if (!vdev->bar[bar].mapped)
		return false;

	return true;
}

static void virtio_pci_common_cfg(virtio_dev_t *vdev, uint8_t bar,
    uint32_t offset, uint32_t length)
{
	if (vdev->common_cfg)
		return;

	if (!check_bar(vdev, bar))
		return;

	vdev->common_cfg = vdev->bar[bar].mapped_base + offset;

	ddf_msg(LVL_NOTE, "common_cfg=%p", vdev->common_cfg);
}

static void virtio_pci_notify_cfg(virtio_dev_t *vdev, uint8_t bar,
    uint32_t offset, uint32_t length, uint32_t multiplier)
{
	if (vdev->notify_base)
		return;

	if (!check_bar(vdev, bar))
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

	if (!check_bar(vdev, bar))
		return;

	vdev->isr = vdev->bar[bar].mapped_base + offset;

	ddf_msg(LVL_NOTE, "isr=%p", vdev->isr);
}

static void virtio_pci_device_cfg(virtio_dev_t *vdev, uint8_t bar,
    uint32_t offset, uint32_t length)
{
	if (vdev->device_cfg)
		return;

	if (!check_bar(vdev, bar))
		return;

	vdev->device_cfg = vdev->bar[bar].mapped_base + offset;

	ddf_msg(LVL_NOTE, "device_cfg=%p", vdev->device_cfg);
}

errno_t virtio_pci_dev_init(ddf_dev_t *dev, virtio_dev_t *vdev)
{
	memset(vdev, 0, sizeof(virtio_dev_t));

	async_sess_t *pci_sess = ddf_dev_parent_sess_get(dev);
	if (!pci_sess)
		return ENOENT;

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
		if (!bar)
			continue;

		rc = pio_enable_resource(&pio_window, &hw_res.resources[j],
		    &vdev->bar[i].mapped_base);
		if (rc == EOK)
			vdev->bar[i].mapped = true;
		j++;
	}

	/*
	 * Find the VIRTIO PCI Capabilities
	 */
	uint8_t c;
	uint8_t id;
	for (rc = pci_config_space_cap_first(pci_sess, &c, &id);
	    (rc == EOK) && c;
	    rc = pci_config_space_cap_next(pci_sess, &c, &id)) {
		if (id == PCI_CAP_VENDORSPECID) {
			uint8_t type;
			rc = pci_config_space_read_8(pci_sess,
			    VIRTIO_PCI_CAP_TYPE(c), &type);
			if (rc != EOK)
				return rc;

			uint8_t bar;
			rc = pci_config_space_read_8(pci_sess,
			    VIRTIO_PCI_CAP_BAR(c), &bar);
			if (rc != EOK)
				return rc;

			uint32_t offset;
			rc = pci_config_space_read_32(pci_sess,
			    VIRTIO_PCI_CAP_OFFSET(c), &offset);
			if (rc != EOK)
				return rc;

			uint32_t length;
			rc = pci_config_space_read_32(pci_sess,
			    VIRTIO_PCI_CAP_LENGTH(c), &length);
			if (rc != EOK)
				return rc;

			uint32_t multiplier;
			switch (type) {
			case VIRTIO_PCI_CAP_COMMON_CFG:
				virtio_pci_common_cfg(vdev, bar, offset,
				    length);
				break;
			case VIRTIO_PCI_CAP_NOTIFY_CFG:
				rc = pci_config_space_read_32(pci_sess,
				    VIRTIO_PCI_CAP_END(c), &multiplier);
				if (rc != EOK)
					return rc;
				virtio_pci_notify_cfg(vdev, bar, offset, length,
				    multiplier);
				break;
			case VIRTIO_PCI_CAP_ISR_CFG:
				virtio_pci_isr_cfg(vdev, bar, offset, length);
				break;
			case VIRTIO_PCI_CAP_DEVICE_CFG:
				virtio_pci_device_cfg(vdev, bar, offset,
				    length);
				break;
			case VIRTIO_PCI_CAP_PCI_CFG:
				break;
			default:
				break;
			}
		}
	}

	return rc;
}

/** @}
 */
