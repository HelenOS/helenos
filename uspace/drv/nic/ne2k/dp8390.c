/*
 * Copyright (c) 2009 Lukas Mejdrech
 * Copyright (c) 2011 Martin Decky
 * Copyright (c) 2011 Radim Vansa
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

/*
 * This code is based upon the NE2000 driver for MINIX,
 * distributed according to a BSD-style license.
 *
 * Copyright (c) 1987, 1997, 2006 Vrije Universiteit
 * Copyright (c) 1992, 1994 Philip Homburg
 * Copyright (c) 1996 G. Falzoni
 *
 */

/**
 * @addtogroup drv_ne2k
 * @{
 */

/**
 * @file
 * @brief NE2000 driver core
 *
 * NE2000 (based on DP8390) network interface core implementation.
 * Only the basic NE2000 PIO (ISA) interface is supported, remote
 * DMA is completely absent from this code for simplicity.
 *
 */

#include <assert.h>
#include <async.h>
#include <byteorder.h>
#include <errno.h>
#include <stdio.h>
#include <ddi.h>
#include "dp8390.h"

/** Page size */
#define DP_PAGE  256

/** 6 * DP_PAGE >= 1514 bytes */
#define SQ_PAGES  6

/** Type definition of the receive header
 *
 */
typedef struct {
	/** Copy of RSR */
	uint8_t status;

	/** Pointer to next frame */
	uint8_t next;

	/** Receive Byte Count Low */
	uint8_t rbcl;

	/** Receive Byte Count High */
	uint8_t rbch;
} recv_header_t;

/** Read a memory block word by word.
 *
 * @param[in]  port Source address.
 * @param[out] buf  Destination buffer.
 * @param[in]  size Memory block size in bytes.
 *
 */
static void pio_read_buf_16(void *port, void *buf, size_t size)
{
	size_t i;

	for (i = 0; (i << 1) < size; i++)
		*((uint16_t *) buf + i) = pio_read_16((ioport16_t *) (port));
}

/** Write a memory block word by word.
 *
 * @param[in] port Destination address.
 * @param[in] buf  Source buffer.
 * @param[in] size Memory block size in bytes.
 *
 */
static void pio_write_buf_16(void *port, void *buf, size_t size)
{
	size_t i;

	for (i = 0; (i << 1) < size; i++)
		pio_write_16((ioport16_t *) port, *((uint16_t *) buf + i));
}

static void ne2k_download(ne2k_t *ne2k, void *buf, size_t addr, size_t size)
{
	size_t esize = size & ~1;

	pio_write_8(ne2k->port + DP_RBCR0, esize & 0xff);
	pio_write_8(ne2k->port + DP_RBCR1, (esize >> 8) & 0xff);
	pio_write_8(ne2k->port + DP_RSAR0, addr & 0xff);
	pio_write_8(ne2k->port + DP_RSAR1, (addr >> 8) & 0xff);
	pio_write_8(ne2k->port + DP_CR, CR_DM_RR | CR_PS_P0 | CR_STA);

	if (esize != 0) {
		pio_read_buf_16(ne2k->data_port, buf, esize);
		size -= esize;
		buf += esize;
	}

	if (size) {
		assert(size == 1);

		uint16_t word = pio_read_16(ne2k->data_port);
		memcpy(buf, &word, 1);
	}
}

static void ne2k_upload(ne2k_t *ne2k, void *buf, size_t addr, size_t size)
{
	size_t esize_ru = (size + 1) & ~1;
	size_t esize = size & ~1;

	pio_write_8(ne2k->port + DP_RBCR0, esize_ru & 0xff);
	pio_write_8(ne2k->port + DP_RBCR1, (esize_ru >> 8) & 0xff);
	pio_write_8(ne2k->port + DP_RSAR0, addr & 0xff);
	pio_write_8(ne2k->port + DP_RSAR1, (addr >> 8) & 0xff);
	pio_write_8(ne2k->port + DP_CR, CR_DM_RW | CR_PS_P0 | CR_STA);

	if (esize != 0) {
		pio_write_buf_16(ne2k->data_port, buf, esize);
		size -= esize;
		buf += esize;
	}

	if (size) {
		assert(size == 1);

		uint16_t word = 0;

		memcpy(&word, buf, 1);
		pio_write_16(ne2k->data_port, word);
	}
}

static void ne2k_init(ne2k_t *ne2k)
{
	unsigned int i;

	/* Reset the ethernet card */
	uint8_t val = pio_read_8(ne2k->port + NE2K_RESET);
	async_usleep(2000);
	pio_write_8(ne2k->port + NE2K_RESET, val);
	async_usleep(2000);

	/* Reset the DP8390 */
	pio_write_8(ne2k->port + DP_CR, CR_STP | CR_DM_ABORT);
	for (i = 0; i < NE2K_RETRY; i++) {
		if (pio_read_8(ne2k->port + DP_ISR) != 0)
			break;
	}
}

/** Probe and initialize the network interface.
 *
 * @param[in,out] ne2k Network interface structure.
 * @param[in]     port Device address.
 * @param[in]     irq  Device interrupt vector.
 *
 * @return EOK on success.
 * @return EXDEV if the network interface was not recognized.
 *
 */
errno_t ne2k_probe(ne2k_t *ne2k)
{
	unsigned int i;

	ne2k_init(ne2k);

	/* Check if the DP8390 is really there */
	uint8_t val = pio_read_8(ne2k->port + DP_CR);
	if ((val & (CR_STP | CR_TXP | CR_DM_ABORT)) != (CR_STP | CR_DM_ABORT))
		return EXDEV;

	/* Disable the receiver and init TCR and DCR */
	pio_write_8(ne2k->port + DP_RCR, RCR_MON);
	pio_write_8(ne2k->port + DP_TCR, TCR_NORMAL);
	pio_write_8(ne2k->port + DP_DCR, DCR_WORDWIDE | DCR_8BYTES | DCR_BMS);

	/* Setup a transfer to get the MAC address */
	pio_write_8(ne2k->port + DP_RBCR0, ETH_ADDR << 1);
	pio_write_8(ne2k->port + DP_RBCR1, 0);
	pio_write_8(ne2k->port + DP_RSAR0, 0);
	pio_write_8(ne2k->port + DP_RSAR1, 0);
	pio_write_8(ne2k->port + DP_CR, CR_DM_RR | CR_PS_P0 | CR_STA);

	for (i = 0; i < ETH_ADDR; i++)
		ne2k->mac.address[i] = pio_read_16(ne2k->data_port);

	return EOK;
}

void ne2k_set_physical_address(ne2k_t *ne2k, const nic_address_t *address)
{
	memcpy(&ne2k->mac, address, sizeof(nic_address_t));

	pio_write_8(ne2k->port + DP_CR, CR_PS_P0 | CR_DM_ABORT | CR_STP);

	pio_write_8(ne2k->port + DP_RBCR0, ETH_ADDR << 1);
	pio_write_8(ne2k->port + DP_RBCR1, 0);
	pio_write_8(ne2k->port + DP_RSAR0, 0);
	pio_write_8(ne2k->port + DP_RSAR1, 0);
	pio_write_8(ne2k->port + DP_CR, CR_DM_RW | CR_PS_P0 | CR_STA);

	size_t i;
	for (i = 0; i < ETH_ADDR; i++)
		pio_write_16(ne2k->data_port, ne2k->mac.address[i]);

	//pio_write_8(ne2k->port + DP_CR, CR_PS_P0 | CR_DM_ABORT | CR_STA);
}

/** Start the network interface.
 *
 * @param[in,out] ne2k Network interface structure.
 *
 * @return EOK on success.
 * @return EXDEV if the network interface is disabled.
 *
 */
errno_t ne2k_up(ne2k_t *ne2k)
{
	if (!ne2k->probed)
		return EXDEV;

	ne2k_init(ne2k);

	/*
	 * Setup send queue. Use the first
	 * SQ_PAGES of NE2000 memory for the send
	 * buffer.
	 */
	ne2k->sq.dirty = false;
	ne2k->sq.page = NE2K_START / DP_PAGE;
	fibril_mutex_initialize(&ne2k->sq_mutex);
	fibril_condvar_initialize(&ne2k->sq_cv);

	/*
	 * Setup receive ring buffer. Use all the rest
	 * of the NE2000 memory (except the first SQ_PAGES
	 * reserved for the send buffer) for the receive
	 * ring buffer.
	 */
	ne2k->start_page = ne2k->sq.page + SQ_PAGES;
	ne2k->stop_page = ne2k->sq.page + NE2K_SIZE / DP_PAGE;

	/*
	 * Initialization of the DP8390 following the mandatory procedure
	 * in reference manual ("DP8390D/NS32490D NIC Network Interface
	 * Controller", National Semiconductor, July 1995, Page 29).
	 */

	/* Step 1: */
	pio_write_8(ne2k->port + DP_CR, CR_PS_P0 | CR_STP | CR_DM_ABORT);

	/* Step 2: */
	pio_write_8(ne2k->port + DP_DCR, DCR_WORDWIDE | DCR_8BYTES | DCR_BMS);

	/* Step 3: */
	pio_write_8(ne2k->port + DP_RBCR0, 0);
	pio_write_8(ne2k->port + DP_RBCR1, 0);

	/* Step 4: */
	pio_write_8(ne2k->port + DP_RCR, ne2k->receive_configuration);

	/* Step 5: */
	pio_write_8(ne2k->port + DP_TCR, TCR_INTERNAL);

	/* Step 6: */
	pio_write_8(ne2k->port + DP_BNRY, ne2k->start_page);
	pio_write_8(ne2k->port + DP_PSTART, ne2k->start_page);
	pio_write_8(ne2k->port + DP_PSTOP, ne2k->stop_page);

	/* Step 7: */
	pio_write_8(ne2k->port + DP_ISR, 0xff);

	/* Step 8: */
	pio_write_8(ne2k->port + DP_IMR,
	    IMR_PRXE | IMR_PTXE | IMR_RXEE | IMR_TXEE | IMR_OVWE | IMR_CNTE);

	/* Step 9: */
	pio_write_8(ne2k->port + DP_CR, CR_PS_P1 | CR_DM_ABORT | CR_STP);

	pio_write_8(ne2k->port + DP_PAR0, ne2k->mac.address[0]);
	pio_write_8(ne2k->port + DP_PAR1, ne2k->mac.address[1]);
	pio_write_8(ne2k->port + DP_PAR2, ne2k->mac.address[2]);
	pio_write_8(ne2k->port + DP_PAR3, ne2k->mac.address[3]);
	pio_write_8(ne2k->port + DP_PAR4, ne2k->mac.address[4]);
	pio_write_8(ne2k->port + DP_PAR5, ne2k->mac.address[5]);

	pio_write_8(ne2k->port + DP_MAR0, 0);
	pio_write_8(ne2k->port + DP_MAR1, 0);
	pio_write_8(ne2k->port + DP_MAR2, 0);
	pio_write_8(ne2k->port + DP_MAR3, 0);
	pio_write_8(ne2k->port + DP_MAR4, 0);
	pio_write_8(ne2k->port + DP_MAR5, 0);
	pio_write_8(ne2k->port + DP_MAR6, 0);
	pio_write_8(ne2k->port + DP_MAR7, 0);

	pio_write_8(ne2k->port + DP_CURR, ne2k->start_page + 1);

	/* Step 10: */
	pio_write_8(ne2k->port + DP_CR, CR_PS_P0 | CR_DM_ABORT | CR_STA);

	/* Step 11: */
	pio_write_8(ne2k->port + DP_TCR, TCR_NORMAL);

	/* Reset counters by reading */
	pio_read_8(ne2k->port + DP_CNTR0);
	pio_read_8(ne2k->port + DP_CNTR1);
	pio_read_8(ne2k->port + DP_CNTR2);

	/* Finish the initialization */
	ne2k->up = true;
	return EOK;
}

/** Stop the network interface.
 *
 * @param[in,out] ne2k Network interface structure.
 *
 */
void ne2k_down(ne2k_t *ne2k)
{
	if ((ne2k->probed) && (ne2k->up)) {
		pio_write_8(ne2k->port + DP_CR, CR_STP | CR_DM_ABORT);
		ne2k_init(ne2k);
		ne2k->up = false;
	}
}

static void ne2k_reset(ne2k_t *ne2k)
{
	unsigned int i;

	fibril_mutex_lock(&ne2k->sq_mutex);

	/* Stop the chip */
	pio_write_8(ne2k->port + DP_CR, CR_STP | CR_DM_ABORT);
	pio_write_8(ne2k->port + DP_RBCR0, 0);
	pio_write_8(ne2k->port + DP_RBCR1, 0);

	for (i = 0; i < NE2K_RETRY; i++) {
		if ((pio_read_8(ne2k->port + DP_ISR) & ISR_RST) != 0)
			break;
	}

	pio_write_8(ne2k->port + DP_TCR, TCR_1EXTERNAL | TCR_OFST);
	pio_write_8(ne2k->port + DP_CR, CR_STA | CR_DM_ABORT);
	pio_write_8(ne2k->port + DP_TCR, TCR_NORMAL);

	/* Acknowledge the ISR_RDC (remote DMA) interrupt */
	for (i = 0; i < NE2K_RETRY; i++) {
		if ((pio_read_8(ne2k->port + DP_ISR) & ISR_RDC) != 0)
			break;
	}

	uint8_t val = pio_read_8(ne2k->port + DP_ISR);
	pio_write_8(ne2k->port + DP_ISR, val & ~ISR_RDC);

	/*
	 * Reset the transmit ring. If we were transmitting a frame,
	 * we pretend that the frame is processed. Higher layers will
	 * retransmit if the frame wasn't actually sent.
	 */
	ne2k->sq.dirty = false;

	fibril_mutex_unlock(&ne2k->sq_mutex);
}

/** Send a frame.
 *
 * @param[in,out] ne2k   Network interface structure.
 * @param[in]     data   Pointer to frame data
 * @param[in]     size   Frame size in bytes
 *
 */
void ne2k_send(nic_t *nic_data, void *data, size_t size)
{
	ne2k_t *ne2k = (ne2k_t *) nic_get_specific(nic_data);

	assert(ne2k->probed);
	assert(ne2k->up);

	fibril_mutex_lock(&ne2k->sq_mutex);

	while (ne2k->sq.dirty) {
		fibril_condvar_wait(&ne2k->sq_cv, &ne2k->sq_mutex);
	}

	if ((size < ETH_MIN_PACK_SIZE) || (size > ETH_MAX_PACK_SIZE_TAGGED)) {
		fibril_mutex_unlock(&ne2k->sq_mutex);
		return;
	}

	/* Upload the frame to the ethernet card */
	ne2k_upload(ne2k, data, ne2k->sq.page * DP_PAGE, size);
	ne2k->sq.dirty = true;
	ne2k->sq.size = size;

	/* Initialize the transfer */
	pio_write_8(ne2k->port + DP_TPSR, ne2k->sq.page);
	pio_write_8(ne2k->port + DP_TBCR0, size & 0xff);
	pio_write_8(ne2k->port + DP_TBCR1, (size >> 8) & 0xff);
	pio_write_8(ne2k->port + DP_CR, CR_TXP | CR_STA);
	fibril_mutex_unlock(&ne2k->sq_mutex);
}

static nic_frame_t *ne2k_receive_frame(nic_t *nic_data, uint8_t page,
    size_t length)
{
	ne2k_t *ne2k = (ne2k_t *) nic_get_specific(nic_data);

	nic_frame_t *frame = nic_alloc_frame(nic_data, length);
	if (frame == NULL)
		return NULL;

	memset(frame->data, 0, length);
	uint8_t last = page + length / DP_PAGE;

	if (last >= ne2k->stop_page) {
		size_t left = (ne2k->stop_page - page) * DP_PAGE -
		    sizeof(recv_header_t);
		ne2k_download(ne2k, frame->data, page * DP_PAGE + sizeof(recv_header_t),
		    left);
		ne2k_download(ne2k, frame->data + left, ne2k->start_page * DP_PAGE,
		    length - left);
	} else {
		ne2k_download(ne2k, frame->data, page * DP_PAGE + sizeof(recv_header_t),
		    length);
	}
	return frame;
}

static void ne2k_receive(nic_t *nic_data)
{
	ne2k_t *ne2k = (ne2k_t *) nic_get_specific(nic_data);
	/*
	 * Allocate memory for the list of received frames.
	 * If the allocation fails here we still receive the
	 * frames from the network, but they will be lost.
	 */
	nic_frame_list_t *frames = nic_alloc_frame_list();
	size_t frames_count = 0;

	/* We may block sending in this loop - after so many received frames there
	 * must be some interrupt pending (for the frames not yet downloaded) and
	 * we will continue in its handler. */
	while (frames_count < 16) {
		//TODO: isn't some locking necessary here?
		uint8_t boundary = pio_read_8(ne2k->port + DP_BNRY) + 1;

		if (boundary == ne2k->stop_page)
			boundary = ne2k->start_page;

		pio_write_8(ne2k->port + DP_CR, CR_PS_P1 | CR_STA);
		uint8_t current = pio_read_8(ne2k->port + DP_CURR);
		pio_write_8(ne2k->port + DP_CR, CR_PS_P0 | CR_STA);
		if (current == boundary)
			/* No more frames to process */
			break;

		recv_header_t header;
		size_t size = sizeof(header);
		size_t offset = boundary * DP_PAGE;

		/* Get the frame header */
		pio_write_8(ne2k->port + DP_RBCR0, size & 0xff);
		pio_write_8(ne2k->port + DP_RBCR1, (size >> 8) & 0xff);
		pio_write_8(ne2k->port + DP_RSAR0, offset & 0xff);
		pio_write_8(ne2k->port + DP_RSAR1, (offset >> 8) & 0xff);
		pio_write_8(ne2k->port + DP_CR, CR_DM_RR | CR_PS_P0 | CR_STA);

		pio_read_buf_16(ne2k->data_port, (void *) &header, size);

		size_t length =
		    (((size_t) header.rbcl) | (((size_t) header.rbch) << 8)) - size;
		uint8_t next = header.next;

		if ((length < ETH_MIN_PACK_SIZE) ||
		    (length > ETH_MAX_PACK_SIZE_TAGGED)) {
			next = current;
		} else if ((header.next < ne2k->start_page) ||
		    (header.next > ne2k->stop_page)) {
			next = current;
		} else if (header.status & RSR_FO) {
			/*
			 * This is very serious, so we issue a warning and
			 * reset the buffers.
			 */
			ne2k->overruns++;
			next = current;
		} else if ((header.status & RSR_PRX) && (ne2k->up)) {
			if (frames != NULL) {
				nic_frame_t *frame =
				    ne2k_receive_frame(nic_data, boundary, length);
				if (frame != NULL) {
					nic_frame_list_append(frames, frame);
					frames_count++;
				} else {
					break;
				}
			} else
				break;
		}

		/*
		 * Update the boundary pointer
		 * to the value of the page
		 * prior to the next frame to
		 * be processed.
		 */
		if (next == ne2k->start_page)
			next = ne2k->stop_page - 1;
		else
			next--;
		pio_write_8(ne2k->port + DP_BNRY, next);
	}
	nic_received_frame_list(nic_data, frames);
}

void ne2k_interrupt(nic_t *nic_data, uint8_t isr, uint8_t tsr)
{
	ne2k_t *ne2k = (ne2k_t *) nic_get_specific(nic_data);

	if (isr & (ISR_PTX | ISR_TXE)) {
		if (tsr & TSR_COL) {
			nic_report_collisions(nic_data,
			    pio_read_8(ne2k->port + DP_NCR) & 15);
		}

		if (tsr & TSR_PTX) {
			// TODO: fix number of sent bytes (but how?)
			nic_report_send_ok(nic_data, 1, 0);
		} else if (tsr & TSR_ABT) {
			nic_report_send_error(nic_data, NIC_SEC_ABORTED, 1);
		} else if (tsr & TSR_CRS) {
			nic_report_send_error(nic_data, NIC_SEC_CARRIER_LOST, 1);
		} else if (tsr & TSR_FU) {
			ne2k->underruns++;
			// if (ne2k->underruns < NE2K_ERL) {
			// }
		} else if (tsr & TSR_CDH) {
			nic_report_send_error(nic_data, NIC_SEC_HEARTBEAT, 1);
			// if (nic_data->stats.send_heartbeat_errors < NE2K_ERL) {
			// }
		} else if (tsr & TSR_OWC) {
			nic_report_send_error(nic_data, NIC_SEC_WINDOW_ERROR, 1);
		}

		fibril_mutex_lock(&ne2k->sq_mutex);
		if (ne2k->sq.dirty) {
			/* Prepare the buffer for next frame */
			ne2k->sq.dirty = false;
			ne2k->sq.size = 0;

			/* Signal a next frame to be sent */
			fibril_condvar_broadcast(&ne2k->sq_cv);
		} else {
			ne2k->misses++;
			// if (ne2k->misses < NE2K_ERL) {
			// }
		}
		fibril_mutex_unlock(&ne2k->sq_mutex);
	}

	if (isr & ISR_CNT) {
		unsigned int errors;
		for (errors = pio_read_8(ne2k->port + DP_CNTR0); errors > 0; --errors)
			nic_report_receive_error(nic_data, NIC_REC_CRC, 1);
		for (errors = pio_read_8(ne2k->port + DP_CNTR1); errors > 0; --errors)
			nic_report_receive_error(nic_data, NIC_REC_FRAME_ALIGNMENT, 1);
		for (errors = pio_read_8(ne2k->port + DP_CNTR2); errors > 0; --errors)
			nic_report_receive_error(nic_data, NIC_REC_MISSED, 1);
	}
	if (isr & ISR_PRX) {
		ne2k_receive(nic_data);
	}
	if (isr & ISR_RST) {
		/*
		 * The chip is stopped, and all arrived
		 * frames are delivered.
		 */
		ne2k_reset(ne2k);
	}

	/* Unmask interrupts to be processed in the next round */
	pio_write_8(ne2k->port + DP_IMR,
	    IMR_PRXE | IMR_PTXE | IMR_RXEE | IMR_TXEE | IMR_OVWE | IMR_CNTE);
}

void ne2k_set_accept_bcast(ne2k_t *ne2k, int accept)
{
	if (accept)
		ne2k->receive_configuration |= RCR_AB;
	else
		ne2k->receive_configuration &= ~RCR_AB;

	pio_write_8(ne2k->port + DP_RCR, ne2k->receive_configuration);
}

void ne2k_set_accept_mcast(ne2k_t *ne2k, int accept)
{
	if (accept)
		ne2k->receive_configuration |= RCR_AM;
	else
		ne2k->receive_configuration &= ~RCR_AM;

	pio_write_8(ne2k->port + DP_RCR, ne2k->receive_configuration);
}

void ne2k_set_promisc_phys(ne2k_t *ne2k, int promisc)
{
	if (promisc)
		ne2k->receive_configuration |= RCR_PRO;
	else
		ne2k->receive_configuration &= ~RCR_PRO;

	pio_write_8(ne2k->port + DP_RCR, ne2k->receive_configuration);
}

void ne2k_set_mcast_hash(ne2k_t *ne2k, uint64_t hash)
{
	/* Select Page 1 and stop all transfers */
	pio_write_8(ne2k->port + DP_CR, CR_PS_P1 | CR_DM_ABORT | CR_STP);

	pio_write_8(ne2k->port + DP_MAR0, (uint8_t) hash);
	pio_write_8(ne2k->port + DP_MAR1, (uint8_t) (hash >> 8));
	pio_write_8(ne2k->port + DP_MAR2, (uint8_t) (hash >> 16));
	pio_write_8(ne2k->port + DP_MAR3, (uint8_t) (hash >> 24));
	pio_write_8(ne2k->port + DP_MAR4, (uint8_t) (hash >> 32));
	pio_write_8(ne2k->port + DP_MAR5, (uint8_t) (hash >> 40));
	pio_write_8(ne2k->port + DP_MAR6, (uint8_t) (hash >> 48));
	pio_write_8(ne2k->port + DP_MAR7, (uint8_t) (hash >> 56));

	/* Select Page 0 and resume transfers */
	pio_write_8(ne2k->port + DP_CR, CR_PS_P0 | CR_DM_ABORT | CR_STA);
}

/** @}
 */
