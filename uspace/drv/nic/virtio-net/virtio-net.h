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
