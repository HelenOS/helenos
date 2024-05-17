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

#include "virtio-net.h"

#include <stdio.h>
#include <stdint.h>

#include <as.h>
#include <ddf/driver.h>
#include <ddf/interrupt.h>
#include <ddf/log.h>
#include <ops/nic.h>
#include <pci_dev_iface.h>
#include <nic/nic.h>

#include <nic.h>

#include <virtio-pci.h>

#define NAME	"virtio-net"

#define VIRTIO_NET_NUM_QUEUES	3

#define RX_QUEUE_1	0
#define TX_QUEUE_1	1
#define CT_QUEUE_1	2

#define BUFFER_SIZE	2048
#define RX_BUF_SIZE	BUFFER_SIZE
#define TX_BUF_SIZE	BUFFER_SIZE
#define CT_BUF_SIZE	BUFFER_SIZE

static ddf_dev_ops_t virtio_net_dev_ops;

static errno_t virtio_net_dev_add(ddf_dev_t *dev);

static driver_ops_t virtio_net_driver_ops = {
	.dev_add = virtio_net_dev_add
};

static driver_t virtio_net_driver = {
	.name = NAME,
	.driver_ops = &virtio_net_driver_ops
};

/** VirtIO net IRQ handler.
 *
 * @param icall IRQ event notification
 * @param arg Argument (nic_t *)
 */
static void virtio_net_irq_handler(ipc_call_t *icall, void *arg)
{
	nic_t *nic = (nic_t *)arg;
	virtio_net_t *virtio_net = nic_get_specific(nic);
	virtio_dev_t *vdev = &virtio_net->virtio_dev;

	uint16_t descno;
	uint32_t len;
	while (virtio_virtq_consume_used(vdev, RX_QUEUE_1, &descno, &len)) {
		virtio_net_hdr_t *hdr =
		    (virtio_net_hdr_t *) virtio_net->rx_buf[descno];
		if (len <= sizeof(*hdr)) {
			ddf_msg(LVL_WARN,
			    "RX data length too short, packet dropped");
			virtio_virtq_produce_available(vdev, RX_QUEUE_1,
			    descno);
			continue;
		}

		nic_frame_t *frame = nic_alloc_frame(nic, len - sizeof(*hdr));
		if (frame) {
			memcpy(frame->data, &hdr[1], len - sizeof(*hdr));
			nic_received_frame(nic, frame);
		} else {
			ddf_msg(LVL_WARN,
			    "Cannot allocate RX frame, packet dropped");
		}

		virtio_virtq_produce_available(vdev, RX_QUEUE_1, descno);
	}

	while (virtio_virtq_consume_used(vdev, TX_QUEUE_1, &descno, &len)) {
		virtio_free_desc(vdev, TX_QUEUE_1, &virtio_net->tx_free_head,
		    descno);
	}
	while (virtio_virtq_consume_used(vdev, CT_QUEUE_1, &descno, &len)) {
		virtio_free_desc(vdev, CT_QUEUE_1, &virtio_net->ct_free_head,
		    descno);
	}
}

static errno_t virtio_net_register_interrupt(ddf_dev_t *dev)
{
	nic_t *nic = ddf_dev_data_get(dev);
	virtio_net_t *virtio_net = nic_get_specific(nic);
	virtio_dev_t *vdev = &virtio_net->virtio_dev;

	hw_res_list_parsed_t res;
	hw_res_list_parsed_init(&res);

	errno_t rc = nic_get_resources(nic, &res);
	if (rc != EOK)
		return rc;

	if (res.irqs.count < 1) {
		hw_res_list_parsed_clean(&res);
		rc = EINVAL;
		return rc;
	}

	virtio_net->irq = res.irqs.irqs[0];
	hw_res_list_parsed_clean(&res);

	irq_pio_range_t pio_ranges[] = {
		{
			.base = vdev->isr_phys,
			.size = sizeof(vdev->isr_phys),
		}
	};

	irq_cmd_t irq_commands[] = {
		{
			.cmd = CMD_PIO_READ_8,
			.addr = (void *) vdev->isr_phys,
			.dstarg = 2
		},
		{
			.cmd = CMD_PREDICATE,
			.value = 1,
			.srcarg = 2
		},
		{
			.cmd = CMD_ACCEPT
		}
	};

	irq_code_t irq_code = {
		.rangecount = sizeof(pio_ranges) / sizeof(irq_pio_range_t),
		.ranges = pio_ranges,
		.cmdcount = sizeof(irq_commands) / sizeof(irq_cmd_t),
		.cmds = irq_commands
	};

	return register_interrupt_handler(dev, virtio_net->irq,
	    virtio_net_irq_handler, (void *)nic, &irq_code,
	    &virtio_net->irq_handle);
}

static errno_t virtio_net_initialize(ddf_dev_t *dev)
{
	nic_t *nic = nic_create_and_bind(dev);
	if (!nic)
		return ENOMEM;

	virtio_net_t *virtio_net = calloc(1, sizeof(virtio_net_t));
	if (!virtio_net) {
		nic_unbind_and_destroy(dev);
		return ENOMEM;
	}

	nic_set_specific(nic, virtio_net);

	errno_t rc = virtio_pci_dev_initialize(dev, &virtio_net->virtio_dev);
	if (rc != EOK)
		return rc;

	virtio_dev_t *vdev = &virtio_net->virtio_dev;
	virtio_pci_common_cfg_t *cfg = virtio_net->virtio_dev.common_cfg;
	virtio_net_cfg_t *netcfg = virtio_net->virtio_dev.device_cfg;

	/*
	 * Register IRQ
	 */
	rc = virtio_net_register_interrupt(dev);
	if (rc != EOK)
		goto fail;

	/* Reset the device and negotiate the feature bits */
	rc = virtio_device_setup_start(vdev,
	    VIRTIO_NET_F_MAC | VIRTIO_NET_F_CTRL_VQ);
	if (rc != EOK)
		goto fail;

	/* Perform device-specific setup */

	/*
	 * Discover and configure the virtqueues
	 */
	uint16_t num_queues = pio_read_le16(&cfg->num_queues);
	if (num_queues != VIRTIO_NET_NUM_QUEUES) {
		ddf_msg(LVL_NOTE, "Unsupported number of virtqueues: %u",
		    num_queues);
		rc = ELIMIT;
		goto fail;
	}

	vdev->queues = calloc(sizeof(virtq_t), num_queues);
	if (!vdev->queues) {
		rc = ENOMEM;
		goto fail;
	}

	rc = virtio_virtq_setup(vdev, RX_QUEUE_1, RX_BUFFERS);
	if (rc != EOK)
		goto fail;
	rc = virtio_virtq_setup(vdev, TX_QUEUE_1, TX_BUFFERS);
	if (rc != EOK)
		goto fail;
	rc = virtio_virtq_setup(vdev, CT_QUEUE_1, CT_BUFFERS);
	if (rc != EOK)
		goto fail;

	/*
	 * Setup DMA buffers
	 */
	rc = virtio_setup_dma_bufs(RX_BUFFERS, RX_BUF_SIZE, false,
	    virtio_net->rx_buf, virtio_net->rx_buf_p);
	if (rc != EOK)
		goto fail;
	rc = virtio_setup_dma_bufs(TX_BUFFERS, TX_BUF_SIZE, true,
	    virtio_net->tx_buf, virtio_net->tx_buf_p);
	if (rc != EOK)
		goto fail;
	rc = virtio_setup_dma_bufs(CT_BUFFERS, CT_BUF_SIZE, true,
	    virtio_net->ct_buf, virtio_net->ct_buf_p);
	if (rc != EOK)
		goto fail;

	/*
	 * Give all RX buffers to the NIC
	 */
	for (unsigned i = 0; i < RX_BUFFERS; i++) {
		/*
		 * Associtate the buffer with the descriptor, set length and
		 * flags.
		 */
		virtio_virtq_desc_set(vdev, RX_QUEUE_1, i,
		    virtio_net->rx_buf_p[i], RX_BUF_SIZE, VIRTQ_DESC_F_WRITE,
		    0);
		/*
		 * Put the set descriptor into the available ring of the RX
		 * queue.
		 */
		virtio_virtq_produce_available(vdev, RX_QUEUE_1, i);
	}

	/*
	 * Put all TX and CT buffers on a free list
	 */
	virtio_create_desc_free_list(vdev, TX_QUEUE_1, TX_BUFFERS,
	    &virtio_net->tx_free_head);
	virtio_create_desc_free_list(vdev, CT_QUEUE_1, CT_BUFFERS,
	    &virtio_net->ct_free_head);

	/*
	 * Read the MAC address
	 */
	nic_address_t nic_addr;
	for (unsigned i = 0; i < ETH_ADDR; i++)
		nic_addr.address[i] = pio_read_8(&netcfg->mac[i]);
	rc = nic_report_address(nic, &nic_addr);
	if (rc != EOK)
		goto fail;

	ddf_msg(LVL_NOTE, "MAC address: " PRIMAC, ARGSMAC(nic_addr.address));

	/*
	 * Enable IRQ
	 */
	rc = hw_res_enable_interrupt(ddf_dev_parent_sess_get(dev),
	    virtio_net->irq);
	if (rc != EOK) {
		ddf_msg(LVL_NOTE, "Failed to enable interrupt");
		goto fail;
	}

	ddf_msg(LVL_NOTE, "Registered IRQ %d", virtio_net->irq);

	/* Go live */
	virtio_device_setup_finalize(vdev);

	return EOK;

fail:
	virtio_teardown_dma_bufs(virtio_net->rx_buf);
	virtio_teardown_dma_bufs(virtio_net->tx_buf);
	virtio_teardown_dma_bufs(virtio_net->ct_buf);

	virtio_device_setup_fail(vdev);
	virtio_pci_dev_cleanup(vdev);
	return rc;
}

static void virtio_net_uninitialize(ddf_dev_t *dev)
{
	nic_t *nic = ddf_dev_data_get(dev);
	virtio_net_t *virtio_net = (virtio_net_t *) nic_get_specific(nic);

	virtio_teardown_dma_bufs(virtio_net->rx_buf);
	virtio_teardown_dma_bufs(virtio_net->tx_buf);
	virtio_teardown_dma_bufs(virtio_net->ct_buf);

	virtio_device_setup_fail(&virtio_net->virtio_dev);
	virtio_pci_dev_cleanup(&virtio_net->virtio_dev);
}

static void virtio_net_send(nic_t *nic, void *data, size_t size)
{
	virtio_net_t *virtio_net = nic_get_specific(nic);
	virtio_dev_t *vdev = &virtio_net->virtio_dev;

	if (size > sizeof(virtio_net) + TX_BUF_SIZE) {
		ddf_msg(LVL_WARN, "TX data too big, frame dropped");
		return;
	}

	uint16_t descno = virtio_alloc_desc(vdev, TX_QUEUE_1,
	    &virtio_net->tx_free_head);
	if (descno == (uint16_t) -1U) {
		ddf_msg(LVL_WARN, "No TX buffers available, frame dropped");
		return;
	}
	assert(descno < TX_BUFFERS);

	/* Setup the packet header */
	virtio_net_hdr_t *hdr = (virtio_net_hdr_t *) virtio_net->tx_buf[descno];
	memset(hdr, 0, sizeof(virtio_net_hdr_t));
	hdr->gso_type = VIRTIO_NET_HDR_GSO_NONE;
	hdr->num_buffers = 0;

	/* Copy packet data into the buffer just past the header */
	memcpy(&hdr[1], data, size);

	/*
	 * Set the descriptor, put it into the virtqueue and notify the device
	 */
	virtio_virtq_desc_set(vdev, TX_QUEUE_1, descno,
	    virtio_net->tx_buf_p[descno], sizeof(virtio_net_hdr_t) + size, 0, 0);
	virtio_virtq_produce_available(vdev, TX_QUEUE_1, descno);
}

static errno_t virtio_net_on_multicast_mode_change(nic_t *nic,
    nic_multicast_mode_t new_mode, const nic_address_t *address_list,
    size_t address_count)
{
	switch (new_mode) {
	case NIC_MULTICAST_BLOCKED:
		nic_report_hw_filtering(nic, -1, 0, -1);
		return EOK;
	case NIC_MULTICAST_LIST:
		nic_report_hw_filtering(nic, -1, 0, -1);
		return EOK;
	case NIC_MULTICAST_PROMISC:
		nic_report_hw_filtering(nic, -1, 0, -1);
		return EOK;
	default:
		return ENOTSUP;
	}
	return EOK;
}

static errno_t virtio_net_on_broadcast_mode_change(nic_t *nic,
    nic_broadcast_mode_t new_mode)
{
	switch (new_mode) {
	case NIC_BROADCAST_BLOCKED:
		return ENOTSUP;
	case NIC_BROADCAST_ACCEPTED:
		return EOK;
	default:
		return ENOTSUP;
	}
}

static errno_t virtio_net_dev_add(ddf_dev_t *dev)
{
	ddf_msg(LVL_NOTE, "%s %s (handle = %zu)", __func__,
	    ddf_dev_get_name(dev), ddf_dev_get_handle(dev));

	errno_t rc = virtio_net_initialize(dev);
	if (rc != EOK)
		return rc;

	ddf_fun_t *fun = ddf_fun_create(dev, fun_exposed, "port0");
	if (fun == NULL) {
		rc = ENOMEM;
		goto uninitialize;
	}
	nic_t *nic = ddf_dev_data_get(dev);
	nic_set_ddf_fun(nic, fun);
	ddf_fun_set_ops(fun, &virtio_net_dev_ops);

	nic_set_send_frame_handler(nic, virtio_net_send);
	nic_set_filtering_change_handlers(nic, NULL,
	    virtio_net_on_multicast_mode_change,
	    virtio_net_on_broadcast_mode_change, NULL, NULL);

	rc = ddf_fun_bind(fun);
	if (rc != EOK) {
		ddf_msg(LVL_ERROR, "Failed binding device function");
		goto destroy;
	}

	rc = ddf_fun_add_to_category(fun, DEVICE_CATEGORY_NIC);
	if (rc != EOK) {
		ddf_msg(LVL_ERROR, "Failed adding function to category");
		goto unbind;
	}

	ddf_msg(LVL_NOTE, "The %s device has been successfully initialized.",
	    ddf_dev_get_name(dev));

	return EOK;

unbind:
	ddf_fun_unbind(fun);
destroy:
	ddf_fun_destroy(fun);
uninitialize:
	virtio_net_uninitialize(dev);
	return rc;
}

static errno_t virtio_net_get_device_info(ddf_fun_t *fun,
    nic_device_info_t *info)
{
	nic_t *nic = nic_get_from_ddf_fun(fun);
	if (!nic)
		return ENOENT;

	str_cpy(info->vendor_name, sizeof(info->vendor_name), "Red Hat, Inc.");
	str_cpy(info->model_name, sizeof(info->model_name),
	    "Virtio network device");

	return EOK;
}

static errno_t virtio_net_get_cable_state(ddf_fun_t *fun,
    nic_cable_state_t *state)
{
	*state = NIC_CS_PLUGGED;
	return EOK;
}

static errno_t virtio_net_get_operation_mode(ddf_fun_t *fun, int *speed,
    nic_channel_mode_t *duplex, nic_role_t *role)
{
	*speed = 1000;
	*duplex = NIC_CM_FULL_DUPLEX;
	*role = NIC_ROLE_UNKNOWN;
	return EOK;
}

static nic_iface_t virtio_net_nic_iface = {
	.get_device_info = virtio_net_get_device_info,
	.get_cable_state = virtio_net_get_cable_state,
	.get_operation_mode = virtio_net_get_operation_mode,
};

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
