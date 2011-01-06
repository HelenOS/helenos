/*
 * Copyright (c) 2009 Lukas Mejdrech
 * Copyright (c) 2011 Martin Decky
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

/** @addtogroup dp8390
 *  @{
 */

/** @file
 *  DP8390 network interface core implementation.
 */

#include <assert.h>
#include <byteorder.h>
#include <errno.h>
#include <netif_local.h>
#include <net/packet.h>
#include <nil_interface.h>
#include <packet_client.h>
#include "dp8390_drv.h"
#include "dp8390_port.h"
#include "dp8390.h"
#include "ne2000.h"

/** Read a memory block byte by byte.
 *
 *  @param[in] port The source address.
 *  @param[out] buf The destination buffer.
 *  @param[in] size The memory block size in bytes.
 *
 */
static void outsb(port_t port, void * buf, size_t size);

/** Read a memory block word by word.
 *
 *  @param[in] port The source address.
 *  @param[out] buf The destination buffer.
 *  @param[in] size The memory block size in bytes.
 *
 */
static void outsw(port_t port, void * buf, size_t size);

/*
 * Some clones of the dp8390 and the PC emulator 'Bochs' require the CR_STA
 * on writes to the CR register. Additional CR_STAs do not appear to hurt
 * genuine dp8390s.
 */
#define CR_EXTRA  CR_STA

static void dp_init(dpeth_t *dep);
static void dp_reinit(dpeth_t *dep);
static void dp_reset(dpeth_t *dep);
static void dp_recv(int nil_phone, device_id_t device_id, dpeth_t *dep);
static void dp_pio8_getblock(dpeth_t *dep, int page, size_t offset, size_t size, void *dst);
static void dp_pio16_getblock(dpeth_t *dep, int page, size_t offset, size_t size, void *dst);
static int dp_pkt2user(int nil_phone, device_id_t device_id, dpeth_t *dep, int page, int length);
static void dp_pio8_user2nic(dpeth_t *dep, void *buf, size_t offset, int nic_addr, size_t size);
static void dp_pio16_user2nic(dpeth_t *dep, void *buf, size_t offset, int nic_addr, size_t size);
static void dp_pio8_nic2user(dpeth_t *dep, int nic_addr, void *buf, size_t offset, size_t size);
static void dp_pio16_nic2user(dpeth_t *dep, int nic_addr, void *buf, size_t offset, size_t size);
static void conf_hw(dpeth_t *dep);
static void insb(port_t port, void *buf, size_t size);
static void insw(port_t port, void *buf, size_t size);

int do_probe(dpeth_t *dep)
{
	/* This is the default, try to (re)locate the device. */
	conf_hw(dep);
	if (dep->de_mode == DEM_DISABLED)
		/* Probe failed, or the device is configured off. */
		return EXDEV;
	
	if (dep->de_mode == DEM_ENABLED)
		dp_init(dep);
	
	return EOK;
}

int do_init(dpeth_t *dep, int mode)
{
	if (dep->de_mode == DEM_DISABLED)
		/* FIXME: Perhaps call do_probe()? */
		return EXDEV;
	
	assert(dep->de_mode == DEM_ENABLED);
	assert(dep->de_flags & DEF_ENABLED);
	
	dep->de_flags &= ~(DEF_PROMISC | DEF_MULTI | DEF_BROAD);
	
	if (mode &DL_PROMISC_REQ)
		dep->de_flags |= DEF_PROMISC | DEF_MULTI | DEF_BROAD;
	
	if (mode &DL_MULTI_REQ)
		dep->de_flags |= DEF_MULTI;
	
	if (mode &DL_BROAD_REQ)
		dep->de_flags |= DEF_BROAD;
	
	dp_reinit(dep);
	return EOK;
}

void do_stop(dpeth_t *dep)
{
	if ((dep->de_mode == DEM_ENABLED)
	    && (dep->de_flags & DEF_ENABLED)) {
		outb_reg0(dep, DP_CR, CR_STP | CR_DM_ABORT);
		(dep->de_stopf)(dep);
		dep->de_flags = DEF_EMPTY;
	}
}

int do_pwrite(dpeth_t *dep, packet_t *packet, int from_int)
{
	int size;
	int sendq_head;
	
	assert(dep->de_mode == DEM_ENABLED);
	assert(dep->de_flags & DEF_ENABLED);
	
	if (dep->de_flags & DEF_SEND_AVAIL) {
		fprintf(stderr, "dp8390: send already in progress\n");
		return EBUSY;
	}
	
	sendq_head = dep->de_sendq_head;
	if (dep->de_sendq[sendq_head].sq_filled) {
		if (from_int)
			fprintf(stderr, "dp8390: should not be sending\n");
		dep->de_flags |= DEF_SEND_AVAIL;
		dep->de_flags &= ~DEF_PACK_SEND;
		
		return EBUSY;
	}
	
	assert(!(dep->de_flags & DEF_PACK_SEND));
	
	void *buf = packet_get_data(packet);
	size = packet_get_data_length(packet);
	
	if (size < ETH_MIN_PACK_SIZE || size > ETH_MAX_PACK_SIZE_TAGGED) {
		fprintf(stderr, "dp8390: invalid packet size\n");
		return EINVAL;
	}
	
	(dep->de_user2nicf)(dep, buf, 0,
	    dep->de_sendq[sendq_head].sq_sendpage * DP_PAGESIZE, size);
	dep->de_sendq[sendq_head].sq_filled = true;
	
	if (dep->de_sendq_tail == sendq_head) {
		outb_reg0(dep, DP_TPSR, dep->de_sendq[sendq_head].sq_sendpage);
		outb_reg0(dep, DP_TBCR1, size >> 8);
		outb_reg0(dep, DP_TBCR0, size & 0xff);
		outb_reg0(dep, DP_CR, CR_TXP | CR_EXTRA);  /* there it goes .. */
	} else
		dep->de_sendq[sendq_head].sq_size = size;
	
	if (++sendq_head == dep->de_sendq_nr)
		sendq_head = 0;
	
	assert(sendq_head < SENDQ_NR);
	dep->de_sendq_head = sendq_head;
	
	dep->de_flags |= DEF_PACK_SEND;
	
	if (from_int)
		return EOK;
	
	dep->de_flags &= ~DEF_PACK_SEND;
	
	return EOK;
}

void dp_init(dpeth_t *dep)
{
	int dp_rcr_reg;
	int i;
	
	/* General initialization */
	dep->de_flags = DEF_EMPTY;
	(*dep->de_initf)(dep);
	
	printf("%s: Ethernet address ", dep->de_name);
	for (i = 0; i < 6; i++)
		printf("%x%c", dep->de_address.ea_addr[i], i < 5 ? ':' : '\n');
	
	/*
	 * Initialization of the dp8390 following the mandatory procedure
	 * in reference manual ("DP8390D/NS32490D NIC Network Interface
	 * Controller", National Semiconductor, July 1995, Page 29).
	 */
	
	/* Step 1: */
	outb_reg0(dep, DP_CR, CR_PS_P0 | CR_STP | CR_DM_ABORT);
	
	/* Step 2: */
	if (dep->de_16bit)
		outb_reg0(dep, DP_DCR, DCR_WORDWIDE | DCR_8BYTES | DCR_BMS);
	else
		outb_reg0(dep, DP_DCR, DCR_BYTEWIDE | DCR_8BYTES | DCR_BMS);
	
	/* Step 3: */
	outb_reg0(dep, DP_RBCR0, 0);
	outb_reg0(dep, DP_RBCR1, 0);
	
	/* Step 4: */
	dp_rcr_reg = 0;
	
	if (dep->de_flags & DEF_PROMISC)
		dp_rcr_reg |= RCR_AB | RCR_PRO | RCR_AM;
	
	if (dep->de_flags & DEF_BROAD)
		dp_rcr_reg |= RCR_AB;
	
	if (dep->de_flags & DEF_MULTI)
		dp_rcr_reg |= RCR_AM;
	
	outb_reg0(dep, DP_RCR, dp_rcr_reg);
	
	/* Step 5: */
	outb_reg0(dep, DP_TCR, TCR_INTERNAL);
	
	/* Step 6: */
	outb_reg0(dep, DP_BNRY, dep->de_startpage);
	outb_reg0(dep, DP_PSTART, dep->de_startpage);
	outb_reg0(dep, DP_PSTOP, dep->de_stoppage);
	
	/* Step 7: */
	outb_reg0(dep, DP_ISR, 0xFF);
	
	/* Step 8: */
	outb_reg0(dep, DP_IMR, IMR_PRXE | IMR_PTXE | IMR_RXEE | IMR_TXEE |
	    IMR_OVWE | IMR_CNTE);
	
	/* Step 9: */
	outb_reg0(dep, DP_CR, CR_PS_P1 | CR_DM_ABORT | CR_STP);
	
	outb_reg1(dep, DP_PAR0, dep->de_address.ea_addr[0]);
	outb_reg1(dep, DP_PAR1, dep->de_address.ea_addr[1]);
	outb_reg1(dep, DP_PAR2, dep->de_address.ea_addr[2]);
	outb_reg1(dep, DP_PAR3, dep->de_address.ea_addr[3]);
	outb_reg1(dep, DP_PAR4, dep->de_address.ea_addr[4]);
	outb_reg1(dep, DP_PAR5, dep->de_address.ea_addr[5]);
	
	outb_reg1(dep, DP_MAR0, 0xff);
	outb_reg1(dep, DP_MAR1, 0xff);
	outb_reg1(dep, DP_MAR2, 0xff);
	outb_reg1(dep, DP_MAR3, 0xff);
	outb_reg1(dep, DP_MAR4, 0xff);
	outb_reg1(dep, DP_MAR5, 0xff);
	outb_reg1(dep, DP_MAR6, 0xff);
	outb_reg1(dep, DP_MAR7, 0xff);
	
	outb_reg1(dep, DP_CURR, dep->de_startpage + 1);
	
	/* Step 10: */
	outb_reg0(dep, DP_CR, CR_DM_ABORT | CR_STA);
	
	/* Step 11: */
	outb_reg0(dep, DP_TCR, TCR_NORMAL);
	
	inb_reg0(dep, DP_CNTR0);  /* Reset counters by reading */
	inb_reg0(dep, DP_CNTR1);
	inb_reg0(dep, DP_CNTR2);
	
	/* Finish the initialization. */
	dep->de_flags |= DEF_ENABLED;
	for (i = 0; i < dep->de_sendq_nr; i++)
		dep->de_sendq[i].sq_filled= 0;
	
	dep->de_sendq_head = 0;
	dep->de_sendq_tail = 0;
	
	if (dep->de_16bit) {
		dep->de_user2nicf= dp_pio16_user2nic;
		dep->de_nic2userf= dp_pio16_nic2user;
		dep->de_getblockf= dp_pio16_getblock;
	} else {
		dep->de_user2nicf= dp_pio8_user2nic;
		dep->de_nic2userf= dp_pio8_nic2user;
		dep->de_getblockf= dp_pio8_getblock;
	}
}

static void dp_reinit(dpeth_t *dep)
{
	int dp_rcr_reg;
	
	outb_reg0(dep, DP_CR, CR_PS_P0 | CR_EXTRA);
	
	dp_rcr_reg = 0;
	
	if (dep->de_flags & DEF_PROMISC)
		dp_rcr_reg |= RCR_AB | RCR_PRO | RCR_AM;
	
	if (dep->de_flags & DEF_BROAD)
		dp_rcr_reg |= RCR_AB;
	
	if (dep->de_flags & DEF_MULTI)
		dp_rcr_reg |= RCR_AM;
	
	outb_reg0(dep, DP_RCR, dp_rcr_reg);
}

static void dp_reset(dpeth_t *dep)
{
	int i;
	
	/* Stop chip */
	outb_reg0(dep, DP_CR, CR_STP | CR_DM_ABORT);
	outb_reg0(dep, DP_RBCR0, 0);
	outb_reg0(dep, DP_RBCR1, 0);
	
	for (i = 0; i < 0x1000 && ((inb_reg0(dep, DP_ISR) & ISR_RST) == 0); i++)
		; /* Do nothing */
	
	outb_reg0(dep, DP_TCR, TCR_1EXTERNAL | TCR_OFST);
	outb_reg0(dep, DP_CR, CR_STA | CR_DM_ABORT);
	outb_reg0(dep, DP_TCR, TCR_NORMAL);
	
	/* Acknowledge the ISR_RDC (remote DMA) interrupt. */
	for (i = 0; i < 0x1000 && ((inb_reg0(dep, DP_ISR) &ISR_RDC) == 0); i++)
		; /* Do nothing */
	
	outb_reg0(dep, DP_ISR, inb_reg0(dep, DP_ISR) & ~ISR_RDC);
	
	/*
	 * Reset the transmit ring. If we were transmitting a packet, we
	 * pretend that the packet is processed. Higher layers will
	 * retransmit if the packet wasn't actually sent.
	 */
	dep->de_sendq_head = 0;
	dep->de_sendq_tail = 0;
	
	for (i = 0; i < dep->de_sendq_nr; i++)
		dep->de_sendq[i].sq_filled = 0;
	
	dep->de_flags &= ~DEF_SEND_AVAIL;
	dep->de_flags &= ~DEF_STOPPED;
}

static uint8_t isr_acknowledge(dpeth_t *dep)
{
	uint8_t isr = inb_reg0(dep, DP_ISR) & 0x7f;
	if (isr != 0)
		outb_reg0(dep, DP_ISR, isr);
	
	return isr;
}

void dp_check_ints(int nil_phone, device_id_t device_id, dpeth_t *dep, uint8_t isr)
{
	int tsr;
	int size, sendq_tail;
	
	if (!(dep->de_flags & DEF_ENABLED))
		fprintf(stderr, "dp8390: got premature interrupt\n");
	
	for (; isr != 0; isr = isr_acknowledge(dep)) {
		if (isr & (ISR_PTX | ISR_TXE)) {
			if (isr & ISR_TXE)
				dep->de_stat.ets_sendErr++;
			else {
				tsr = inb_reg0(dep, DP_TSR);
				
				if (tsr & TSR_PTX)
					dep->de_stat.ets_packetT++;
				
				if (tsr & TSR_COL)
					dep->de_stat.ets_collision++;
				
				if (tsr & TSR_ABT)
					dep->de_stat.ets_transAb++;
				
				if (tsr & TSR_CRS)
					dep->de_stat.ets_carrSense++;
				
				if ((tsr & TSR_FU) && (++dep->de_stat.ets_fifoUnder <= 10))
					printf("%s: fifo underrun\n", dep->de_name);
				
				if ((tsr & TSR_CDH) && (++dep->de_stat.ets_CDheartbeat <= 10))
					printf("%s: CD heart beat failure\n", dep->de_name);
				
				if (tsr & TSR_OWC)
					dep->de_stat.ets_OWC++;
			}
			
			sendq_tail = dep->de_sendq_tail;
			
			if (!(dep->de_sendq[sendq_tail].sq_filled)) {
				/* Or hardware bug? */
				printf("%s: transmit interrupt, but not sending\n", dep->de_name);
				continue;
			}
			
			dep->de_sendq[sendq_tail].sq_filled = 0;
			
			if (++sendq_tail == dep->de_sendq_nr)
				sendq_tail = 0;
			
			dep->de_sendq_tail = sendq_tail;
			
			if (dep->de_sendq[sendq_tail].sq_filled) {
				size = dep->de_sendq[sendq_tail].sq_size;
				outb_reg0(dep, DP_TPSR,
				    dep->de_sendq[sendq_tail].sq_sendpage);
				outb_reg0(dep, DP_TBCR1, size >> 8);
				outb_reg0(dep, DP_TBCR0, size & 0xff);
				outb_reg0(dep, DP_CR, CR_TXP | CR_EXTRA);
			}
			
			dep->de_flags &= ~DEF_SEND_AVAIL;
		}
		
		if (isr & ISR_PRX)
			dp_recv(nil_phone, device_id, dep);
		
		if (isr & ISR_RXE)
			dep->de_stat.ets_recvErr++;
		
		if (isr & ISR_CNT) {
			dep->de_stat.ets_CRCerr += inb_reg0(dep, DP_CNTR0);
			dep->de_stat.ets_frameAll += inb_reg0(dep, DP_CNTR1);
			dep->de_stat.ets_missedP += inb_reg0(dep, DP_CNTR2);
		}
		
		if (isr & ISR_OVW)
			dep->de_stat.ets_OVW++;
		
		if (isr & ISR_RDC) {
			/* Nothing to do */
		}
		
		if (isr & ISR_RST) {
			/*
			 * This means we got an interrupt but the ethernet 
			 * chip is shutdown. We set the flag DEF_STOPPED,
			 * and continue processing arrived packets. When the
			 * receive buffer is empty, we reset the dp8390.
			 */
			dep->de_flags |= DEF_STOPPED;
			break;
		}
	}
	
	if ((dep->de_flags & DEF_STOPPED) == DEF_STOPPED) {
		/*
		 * The chip is stopped, and all arrived packets
		 * are delivered.
		 */
		dp_reset(dep);
	}
	
	dep->de_flags &= ~DEF_PACK_SEND;
}

static void dp_recv(int nil_phone, device_id_t device_id, dpeth_t *dep)
{
	dp_rcvhdr_t header;
	int pageno, curr, next;
	size_t length;
	int packet_processed, r;
	uint16_t eth_type;
	
	packet_processed = false;
	pageno = inb_reg0(dep, DP_BNRY) + 1;
	if (pageno == dep->de_stoppage)
		pageno = dep->de_startpage;
	
	do {
		outb_reg0(dep, DP_CR, CR_PS_P1 | CR_EXTRA);
		curr = inb_reg1(dep, DP_CURR);
		outb_reg0(dep, DP_CR, CR_PS_P0 | CR_EXTRA);
		
		if (curr == pageno)
			break;
		
		(dep->de_getblockf)(dep, pageno, (size_t) 0, sizeof(header), &header);
		(dep->de_getblockf)(dep, pageno, sizeof(header) + 2 * sizeof(ether_addr_t), sizeof(eth_type), &eth_type);
		
		length = (header.dr_rbcl | (header.dr_rbch << 8)) - sizeof(dp_rcvhdr_t);
		next = header.dr_next;
		if ((length < ETH_MIN_PACK_SIZE) || (length > ETH_MAX_PACK_SIZE_TAGGED)) {
			printf("%s: packet with strange length arrived: %d\n", dep->de_name, (int) length);
			next= curr;
		} else if ((next < dep->de_startpage) || (next >= dep->de_stoppage)) {
			printf("%s: strange next page\n", dep->de_name);
			next= curr;
		} else if (header.dr_status & RSR_FO) {
			/*
			 * This is very serious, so we issue a warning and
			 * reset the buffers
			 */
			printf("%s: fifo overrun, resetting receive buffer\n", dep->de_name);
			dep->de_stat.ets_fifoOver++;
			next = curr;
		} else if ((header.dr_status & RSR_PRX) && (dep->de_flags & DEF_ENABLED)) {
			r = dp_pkt2user(nil_phone, device_id, dep, pageno, length);
			if (r != EOK)
				return;
			
			packet_processed = true;
			dep->de_stat.ets_packetR++;
		}
		
		if (next == dep->de_startpage)
			outb_reg0(dep, DP_BNRY, dep->de_stoppage - 1);
		else
			outb_reg0(dep, DP_BNRY, next - 1);
		
		pageno = next;
	} while (!packet_processed);
}

static void dp_pio8_getblock(dpeth_t *dep, int page, size_t offset, size_t size, void *dst)
{
	offset = page * DP_PAGESIZE + offset;
	outb_reg0(dep, DP_RBCR0, size &0xFF);
	outb_reg0(dep, DP_RBCR1, size >> 8);
	outb_reg0(dep, DP_RSAR0, offset &0xFF);
	outb_reg0(dep, DP_RSAR1, offset >> 8);
	outb_reg0(dep, DP_CR, CR_DM_RR | CR_PS_P0 | CR_STA);
	
	insb(dep->de_data_port, dst, size);
}

static void dp_pio16_getblock(dpeth_t *dep, int page, size_t offset, size_t size, void *dst)
{
	offset = page * DP_PAGESIZE + offset;
	outb_reg0(dep, DP_RBCR0, size &0xFF);
	outb_reg0(dep, DP_RBCR1, size >> 8);
	outb_reg0(dep, DP_RSAR0, offset &0xFF);
	outb_reg0(dep, DP_RSAR1, offset >> 8);
	outb_reg0(dep, DP_CR, CR_DM_RR | CR_PS_P0 | CR_STA);
	
	assert(!(size & 1));
	
	insw(dep->de_data_port, dst, size);
}

static int dp_pkt2user(int nil_phone, device_id_t device_id, dpeth_t *dep, int page, int length)
{
	int last, count;
	packet_t *packet;
	
	packet = netif_packet_get_1(length);
	if (!packet)
		return ENOMEM;
	
	void *buf = packet_suffix(packet, length);
	
	last = page + (length - 1) / DP_PAGESIZE;
	if (last >= dep->de_stoppage) {
		count = (dep->de_stoppage - page) * DP_PAGESIZE - sizeof(dp_rcvhdr_t);
		
		(dep->de_nic2userf)(dep, page * DP_PAGESIZE +
		    sizeof(dp_rcvhdr_t), buf, 0, count);
		(dep->de_nic2userf)(dep, dep->de_startpage * DP_PAGESIZE,
		    buf, count, length - count);
	} else {
		(dep->de_nic2userf)(dep, page * DP_PAGESIZE +
		    sizeof(dp_rcvhdr_t), buf, 0, length);
	}
	
	nil_received_msg(nil_phone, device_id, packet, SERVICE_NONE);
	
	return EOK;
}

static void dp_pio8_user2nic(dpeth_t *dep, void *buf, size_t offset, int nic_addr, size_t size)
{
	outb_reg0(dep, DP_ISR, ISR_RDC);
	
	outb_reg0(dep, DP_RBCR0, size & 0xFF);
	outb_reg0(dep, DP_RBCR1, size >> 8);
	outb_reg0(dep, DP_RSAR0, nic_addr & 0xFF);
	outb_reg0(dep, DP_RSAR1, nic_addr >> 8);
	outb_reg0(dep, DP_CR, CR_DM_RW | CR_PS_P0 | CR_STA);
	
	outsb(dep->de_data_port, buf + offset, size);
	
	unsigned int i;
	for (i = 0; i < 100; i++) {
		if (inb_reg0(dep, DP_ISR) & ISR_RDC)
			break;
	}
	
	if (i == 100)
		fprintf(stderr, "dp8390: remote DMA failed to complete\n");
}

static void dp_pio16_user2nic(dpeth_t *dep, void *buf, size_t offset, int nic_addr, size_t size)
{
	void *vir_user;
	size_t ecount;
	uint16_t two_bytes;
	
	ecount = size & ~1;
	
	outb_reg0(dep, DP_ISR, ISR_RDC);
	outb_reg0(dep, DP_RBCR0, ecount & 0xFF);
	outb_reg0(dep, DP_RBCR1, ecount >> 8);
	outb_reg0(dep, DP_RSAR0, nic_addr & 0xFF);
	outb_reg0(dep, DP_RSAR1, nic_addr >> 8);
	outb_reg0(dep, DP_CR, CR_DM_RW | CR_PS_P0 | CR_STA);
	
	vir_user = buf + offset;
	if (ecount != 0) {
		outsw(dep->de_data_port, vir_user, ecount);
		size -= ecount;
		offset += ecount;
		vir_user += ecount;
	}
	
	if (size) {
		assert(size == 1);
		
		memcpy(&(((uint8_t *) &two_bytes)[0]), vir_user, 1);
		outw(dep->de_data_port, two_bytes);
	}
	
	unsigned int i;
	for (i = 0; i < 100; i++) {
		if (inb_reg0(dep, DP_ISR) & ISR_RDC)
			break;
	}
	
	if (i == 100)
		fprintf(stderr, "dp8390: remote dma failed to complete\n");
}

static void dp_pio8_nic2user(dpeth_t *dep, int nic_addr, void *buf, size_t offset, size_t size)
{
	outb_reg0(dep, DP_RBCR0, size & 0xFF);
	outb_reg0(dep, DP_RBCR1, size >> 8);
	outb_reg0(dep, DP_RSAR0, nic_addr & 0xFF);
	outb_reg0(dep, DP_RSAR1, nic_addr >> 8);
	outb_reg0(dep, DP_CR, CR_DM_RR | CR_PS_P0 | CR_STA);
	
	insb(dep->de_data_port, buf + offset, size);
}

static void dp_pio16_nic2user(dpeth_t *dep, int nic_addr, void *buf, size_t offset, size_t size)
{
	void *vir_user;
	size_t ecount;
	uint16_t two_bytes;
	
	ecount = size & ~1;
	
	outb_reg0(dep, DP_RBCR0, ecount & 0xFF);
	outb_reg0(dep, DP_RBCR1, ecount >> 8);
	outb_reg0(dep, DP_RSAR0, nic_addr & 0xFF);
	outb_reg0(dep, DP_RSAR1, nic_addr >> 8);
	outb_reg0(dep, DP_CR, CR_DM_RR | CR_PS_P0 | CR_STA);
	
	vir_user = buf + offset;
	if (ecount != 0) {
		insw(dep->de_data_port, vir_user, ecount);
		size -= ecount;
		offset += ecount;
		vir_user += ecount;
	}
	
	if (size) {
		assert(size == 1);
		
		two_bytes = inw(dep->de_data_port);
		memcpy(vir_user, &(((uint8_t *) &two_bytes)[0]), 1);
	}
}

static void conf_hw(dpeth_t *dep)
{
	if (!ne_probe(dep)) {
		printf("%s: No ethernet card found at %#lx\n",
		    dep->de_name, dep->de_base_port);
		dep->de_mode= DEM_DISABLED;
		return;
	}
	
	dep->de_mode = DEM_ENABLED;
	dep->de_flags = DEF_EMPTY;
}

static void insb(port_t port, void *buf, size_t size)
{
	size_t i;
	
	for (i = 0; i < size; i++)
		*((uint8_t *) buf + i) = inb(port);
}

static void insw(port_t port, void *buf, size_t size)
{
	size_t i;
	
	for (i = 0; i * 2 < size; i++)
		*((uint16_t *) buf + i) = inw(port);
}

static void outsb(port_t port, void *buf, size_t size)
{
	size_t i;
	
	for (i = 0; i < size; i++)
		outb(port, *((uint8_t *) buf + i));
}

static void outsw(port_t port, void *buf, size_t size)
{
	size_t i;
	
	for (i = 0; i * 2 < size; i++)
		outw(port, *((uint16_t *) buf + i));
}

/** @}
 */
