/*
 * SPDX-FileCopyrightText: 2018 Jakub Jermar
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef _VIRTIO_NET_H_
#define _VIRTIO_NET_H_

#include <virtio-pci.h>
#include <abi/cap.h>
#include <nic/nic.h>

#define RX_BUFFERS	8
#define TX_BUFFERS	8
#define CT_BUFFERS	4

/** Device handles packets with partial checksum. */
#define VIRTIO_NET_F_CSUM		(1U << 0)
/** Driver handles packets with partial checksum. */
#define VIRTIO_NET_F_GUEST_CSUM		(1U << 2)
/** Device has given MAC address. */
#define VIRTIO_NET_F_MAC		(1U << 5)
/** Control channel is available */
#define VIRTIO_NET_F_CTRL_VQ		(1U << 17)

#define VIRTIO_NET_HDR_GSO_NONE 0
typedef struct {
	uint8_t flags;
	uint8_t gso_type;
	uint16_t hdr_len;
	uint16_t gso_size;
	uint16_t csum_start;
	uint16_t csum_offset;
	uint16_t num_buffers;
} virtio_net_hdr_t;

typedef struct {
	uint8_t mac[ETH_ADDR];
} virtio_net_cfg_t;

typedef struct {
	virtio_dev_t virtio_dev;
	void *rx_buf[RX_BUFFERS];
	uintptr_t rx_buf_p[RX_BUFFERS];
	void *tx_buf[TX_BUFFERS];
	uintptr_t tx_buf_p[TX_BUFFERS];
	void *ct_buf[CT_BUFFERS];
	uintptr_t ct_buf_p[CT_BUFFERS];

	uint16_t tx_free_head;
	uint16_t ct_free_head;

	int irq;
	cap_irq_handle_t irq_handle;
} virtio_net_t;

#endif
