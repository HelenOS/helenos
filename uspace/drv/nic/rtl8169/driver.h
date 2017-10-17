/*
 * Copyright (c) 2011 Jiri Michalec
 * Copyright (c) 2014 Agnieszka Tabaka
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

#ifndef RTL8169_DRIVER_H_
#define RTL8169_DRIVER_H_

#include <stddef.h>
#include <stdint.h>
#include "defs.h"

/** The driver name */
#define NAME  "rtl8169"

#define	TX_BUFFERS_COUNT	16
#define	RX_BUFFERS_COUNT	16
#define	BUFFER_SIZE		2048

#define	TX_RING_SIZE		(sizeof(rtl8169_descr_t) * TX_BUFFERS_COUNT)
#define	RX_RING_SIZE		(sizeof(rtl8169_descr_t) * RX_BUFFERS_COUNT)
#define	TX_BUFFERS_SIZE		(BUFFER_SIZE * TX_BUFFERS_COUNT)
#define	RX_BUFFERS_SIZE		(BUFFER_SIZE * RX_BUFFERS_COUNT)

/** RTL8139 device data */
typedef struct rtl8169_data {
	/** DDF device */
	ddf_dev_t *dev;
	/** Parent session */
	async_sess_t *parent_sess;
	/** I/O address of the device */
	void *regs_phys;
	/** Mapped I/O port */
	void *regs;
	/** The irq assigned */
	int irq;
	/** PCI Vendor and Product ids */
	uint16_t pci_vid;
	uint16_t pci_pid;
	/** Mask of the turned interupts (IMR value) */
	uint16_t int_mask;
	/** TX ring */
	uintptr_t tx_ring_phys;
	rtl8169_descr_t *tx_ring;
	unsigned int tx_head;
	unsigned int tx_tail;
	/** RX ring */
	uintptr_t rx_ring_phys;
	rtl8169_descr_t *rx_ring;
	unsigned int rx_head;
	unsigned int rx_tail;
	/** TX buffers */
	uintptr_t tx_buff_phys;
	void *tx_buff;
	/** RX buffers */
	uintptr_t rx_buff_phys;
	void *rx_buff;
	/** The nubmer of the next buffer to use, index = tx_next % TX_BUFF_COUNT */
	size_t tx_next;
	/** The number of the first used buffer in the row
	 *
	 *  tx_used is in the interval tx_next - TX_BUFF_COUNT and tx_next:
	 *  	tx_next - TX_BUFF_COUNT: there is no useable Tx descriptor
	 *  	tx_next: all Tx descriptors are can be used
	 */
	size_t tx_used;

	/** Receive Control Register masks */
	uint32_t rcr_ucast;
	uint32_t rcr_mcast;

	/** Lock for receiver */
	fibril_mutex_t rx_lock;
	/** Lock for transmitter */
	fibril_mutex_t tx_lock;

	/** Backward pointer to nic_data */
	nic_t *nic_data;

} rtl8169_t;

#endif
