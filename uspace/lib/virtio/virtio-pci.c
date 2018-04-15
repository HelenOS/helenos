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
#include <pci_dev_iface.h>

static void virtio_pci_common_cfg(virtio_dev_t *vdev, hw_resource_list_t *res,
    uint8_t bar, uint32_t offset, uint32_t length)
{
	/* Proceed only if we don't have common config structure yet */
	if (vdev->common_cfg)
		return;

	/* We must ignore the capability if bar is greater than 5 */
	if (bar > 5)
		return;
}

errno_t virtio_pci_dev_init(ddf_dev_t *dev, virtio_dev_t *vdev)
{
	async_sess_t *pci_sess = ddf_dev_parent_sess_get(dev);
	if (!pci_sess)
		return ENOENT;

	errno_t rc;
	hw_resource_list_t hw_res;
	rc = hw_res_get_resource_list(pci_sess, &hw_res);
	if (rc != EOK)
		return rc;

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

			switch (type) {
			case VIRTIO_PCI_CAP_COMMON_CFG:
				virtio_pci_common_cfg(vdev, &hw_res, bar,
				    offset, length);
				break;
			case VIRTIO_PCI_CAP_NOTIFY_CFG:
				break;
			case VIRTIO_PCI_CAP_ISR_CFG:
				break;
			case VIRTIO_PCI_CAP_DEVICE_CFG:
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
