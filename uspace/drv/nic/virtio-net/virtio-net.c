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

#include <stdio.h>
#include <stdint.h>

#include <ddf/driver.h>
#include <ddf/log.h>
#include <ops/nic.h>
#include <pci_dev_iface.h>

#include <nic.h>

#define NAME	"virtio-net"

#define VIRTIO_PCI_CAP_TYPE(c)		((c) + 3)
#define VIRTIO_PCI_CAP_BAR(c)		((c) + 4)
#define VIRTIO_PCI_CAP_OFFSET(c)	((c) + 8)
#define VIRTIO_PCI_CAP_LENGTH(c)	((c) + 12)

#define VIRTIO_PCI_CAP_COMMON_CFG	1
#define VIRTIO_PCI_CAP_NOTIFY_CFG	2
#define VIRTIO_PCI_CAP_ISR_CFG		3
#define VIRTIO_PCI_CAP_DEVICE_CFG	4
#define VIRTIO_PCI_CAP_PCI_CFG		5

static errno_t virtio_net_dev_add(ddf_dev_t *dev)
{
	ddf_msg(LVL_NOTE, "%s %s (handle = %zu)", __func__,
	    ddf_dev_get_name(dev), ddf_dev_get_handle(dev));

	async_sess_t *pci_sess = ddf_dev_parent_sess_get(dev);
	if (!pci_sess)
		return ENOENT;

	/*
	 * Find the VIRTIO PCI Capabilities
	 */
	errno_t rc;
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

			switch (type) {
			case VIRTIO_PCI_CAP_COMMON_CFG:
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
	if (rc != EOK)
		return rc;

	return ENOTSUP;
}

static ddf_dev_ops_t virtio_net_dev_ops;

static driver_ops_t virtio_net_driver_ops = {
	.dev_add = virtio_net_dev_add
};

static driver_t virtio_net_driver = {
	.name = NAME,
	.driver_ops = &virtio_net_driver_ops
};

static nic_iface_t virtio_net_nic_iface;

int main(void)
{
	printf("%s: HelenOS virtio-net driver\n", NAME);

	if (nic_driver_init(NAME) != EOK)
		return 1;

	nic_driver_implement(&virtio_net_driver_ops, &virtio_net_dev_ops,
	    &virtio_net_nic_iface);

	(void) ddf_log_init(NAME);
	return ddf_driver_main(&virtio_net_driver);
}
