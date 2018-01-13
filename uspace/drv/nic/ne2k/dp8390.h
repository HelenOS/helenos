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

/** @addtogroup drv_ne2k
 *  @{
 */

/** @file
 *  DP8390 network interface definitions.
 */

#ifndef __NET_NETIF_DP8390_H__
#define __NET_NETIF_DP8390_H__

#include <async.h>
#include <ddf/driver.h>
#include <fibril_synch.h>
#include <nic.h>
#include <ddf/interrupt.h>

/** Input/output size */
#define NE2K_IO_SIZE  0x0020

/* NE2000 implementation. */

/** NE2000 Data Register */
#define NE2K_DATA  0x0010

/** NE2000 Reset register */
#define NE2K_RESET  0x001f

/** NE2000 data start */
#define NE2K_START  0x4000

/** NE2000 data size */
#define NE2K_SIZE  0x4000

/** NE2000 retry count */
#define NE2K_RETRY  0x1000

/** NE2000 error messages rate limiting */
#define NE2K_ERL  10

/** Minimum Ethernet packet size in bytes */
#define ETH_MIN_PACK_SIZE  60

/** Maximum Ethernet packet size in bytes */
#define ETH_MAX_PACK_SIZE_TAGGED  1518

/* National Semiconductor DP8390 Network Interface Controller. */

/** Page 0, for reading */
#define DP_CR     0x00  /**< Command Register */
#define DP_CLDA0  0x01  /**< Current Local DMA Address 0 */
#define DP_CLDA1  0x02  /**< Current Local DMA Address 1 */
#define DP_BNRY   0x03  /**< Boundary Pointer */
#define DP_TSR    0x04  /**< Transmit Status Register */
#define DP_NCR    0x05  /**< Number of Collisions Register */
#define DP_FIFO   0x06  /**< FIFO */
#define DP_ISR    0x07  /**< Interrupt Status Register */
#define DP_CRDA0  0x08  /**< Current Remote DMA Address 0 */
#define DP_CRDA1  0x09  /**< Current Remote DMA Address 1 */
#define DP_RSR    0x0c  /**< Receive Status Register */
#define DP_CNTR0  0x0d  /**< Tally Counter 0 */
#define DP_CNTR1  0x0e  /**< Tally Counter 1 */
#define DP_CNTR2  0x0f  /**< Tally Counter 2 */

/** Page 0, for writing */
#define DP_PSTART  0x01  /**< Page Start Register*/
#define DP_PSTOP   0x02  /**< Page Stop Register */
#define DP_TPSR    0x04  /**< Transmit Page Start Register */
#define DP_TBCR0   0x05  /**< Transmit Byte Count Register 0 */
#define DP_TBCR1   0x06  /**< Transmit Byte Count Register 1 */
#define DP_RSAR0   0x08  /**< Remote Start Address Register 0 */
#define DP_RSAR1   0x09  /**< Remote Start Address Register 1 */
#define DP_RBCR0   0x0a  /**< Remote Byte Count Register 0 */
#define DP_RBCR1   0x0b  /**< Remote Byte Count Register 1 */
#define DP_RCR     0x0c  /**< Receive Configuration Register */
#define DP_TCR     0x0d  /**< Transmit Configuration Register */
#define DP_DCR     0x0e  /**< Data Configuration Register */
#define DP_IMR     0x0f  /**< Interrupt Mask Register */

/** Page 1, read/write */
#define DP_PAR0  0x01  /**< Physical Address Register 0 */
#define DP_PAR1  0x02  /**< Physical Address Register 1 */
#define DP_PAR2  0x03  /**< Physical Address Register 2 */
#define DP_PAR3  0x04  /**< Physical Address Register 3 */
#define DP_PAR4  0x05  /**< Physical Address Register 4 */
#define DP_PAR5  0x06  /**< Physical Address Register 5 */
#define DP_CURR  0x07  /**< Current Page Register */
#define DP_MAR0  0x08  /**< Multicast Address Register 0 */
#define DP_MAR1  0x09  /**< Multicast Address Register 1 */
#define DP_MAR2  0x0a  /**< Multicast Address Register 2 */
#define DP_MAR3  0x0b  /**< Multicast Address Register 3 */
#define DP_MAR4  0x0c  /**< Multicast Address Register 4 */
#define DP_MAR5  0x0d  /**< Multicast Address Register 5 */
#define DP_MAR6  0x0e  /**< Multicast Address Register 6 */
#define DP_MAR7  0x0f  /**< Multicast Address Register 7 */

/* Bits in Command Register */
#define CR_STP       0x01  /**< Stop (software reset) */
#define CR_STA       0x02  /**< Start (activate NIC) */
#define CR_TXP       0x04  /**< Transmit Packet */
#define CR_DMA       0x38  /**< Mask for DMA control */
#define CR_DM_NOP    0x00  /**< DMA: No Operation */
#define CR_DM_RR     0x08  /**< DMA: Remote Read */
#define CR_DM_RW     0x10  /**< DMA: Remote Write */
#define CR_DM_SP     0x18  /**< DMA: Send Packet */
#define CR_DM_ABORT  0x20  /**< DMA: Abort Remote DMA Operation */
#define CR_PS        0xc0  /**< Mask for Page Select */
#define CR_PS_P0     0x00  /**< Register Page 0 */
#define CR_PS_P1     0x40  /**< Register Page 1 */
#define CR_PS_P2     0x80  /**< Register Page 2 */
#define CR_PS_T1     0xc0  /**< Test Mode Register Map */

/* Bits in Interrupt State Register */
#define ISR_PRX  0x01  /**< Packet Received with no errors */
#define ISR_PTX  0x02  /**< Packet Transmitted with no errors */
#define ISR_RXE  0x04  /**< Receive Error */
#define ISR_TXE  0x08  /**< Transmit Error */
#define ISR_OVW  0x10  /**< Overwrite Warning */
#define ISR_CNT  0x20  /**< Counter Overflow */
#define ISR_RDC  0x40  /**< Remote DMA Complete */
#define ISR_RST  0x80  /**< Reset Status */

/* Bits in Interrupt Mask Register */
#define IMR_PRXE  0x01  /**< Packet Received Interrupt Enable */
#define IMR_PTXE  0x02  /**< Packet Transmitted Interrupt Enable */
#define IMR_RXEE  0x04  /**< Receive Error Interrupt Enable */
#define IMR_TXEE  0x08  /**< Transmit Error Interrupt Enable */
#define IMR_OVWE  0x10  /**< Overwrite Warning Interrupt Enable */
#define IMR_CNTE  0x20  /**< Counter Overflow Interrupt Enable */
#define IMR_RDCE  0x40  /**< DMA Complete Interrupt Enable */

/* Bits in Data Configuration Register */
#define DCR_WTS        0x01  /**< Word Transfer Select */
#define DCR_BYTEWIDE   0x00  /**< WTS: byte wide transfers */
#define DCR_WORDWIDE   0x01  /**< WTS: word wide transfers */
#define DCR_BOS        0x02  /**< Byte Order Select */
#define DCR_LTLENDIAN  0x00  /**< BOS: Little Endian */
#define DCR_BIGENDIAN  0x02  /**< BOS: Big Endian */
#define DCR_LAS        0x04  /**< Long Address Select */
#define DCR_BMS        0x08  /**< Burst Mode Select */
#define DCR_AR         0x10  /**< Autoinitialize Remote */
#define DCR_FTS        0x60  /**< Fifo Threshold Select */
#define DCR_2BYTES     0x00  /**< 2 bytes */
#define DCR_4BYTES     0x40  /**< 4 bytes */
#define DCR_8BYTES     0x20  /**< 8 bytes */
#define DCR_12BYTES    0x60  /**< 12 bytes */

/* Bits in Transmit Configuration Register */
#define TCR_CRC        0x01  /**< Inhibit CRC */
#define TCR_ELC        0x06  /**< Encoded Loopback Control */
#define TCR_NORMAL     0x00  /**< ELC: Normal Operation */
#define TCR_INTERNAL   0x02  /**< ELC: Internal Loopback */
#define TCR_0EXTERNAL  0x04  /**< ELC: External Loopback LPBK=0 */
#define TCR_1EXTERNAL  0x06  /**< ELC: External Loopback LPBK=1 */
#define TCR_ATD        0x08  /**< Auto Transmit Disable */
#define TCR_OFST       0x10  /**< Collision Offset Enable (be nice) */

/* Bits in Interrupt Status Register */
#define TSR_PTX  0x01  /**< Packet Transmitted (without error) */
#define TSR_DFR  0x02  /**< Transmit Deferred (reserved) */
#define TSR_COL  0x04  /**< Transmit Collided */
#define TSR_ABT  0x08  /**< Transmit Aborted */
#define TSR_CRS  0x10  /**< Carrier Sense Lost */
#define TSR_FU   0x20  /**< FIFO Underrun */
#define TSR_CDH  0x40  /**< CD Heartbeat */
#define TSR_OWC  0x80  /**< Out of Window Collision */

/* Bits in Receive Configuration Register */
#define RCR_SEP  0x01  /**< Save Errored Packets */
#define RCR_AR   0x02  /**< Accept Runt Packets */
#define RCR_AB   0x04  /**< Accept Broadcast */
#define RCR_AM   0x08  /**< Accept Multicast */
#define RCR_PRO  0x10  /**< Physical Promiscuous */
#define RCR_MON  0x20  /**< Monitor Mode */

/* Bits in Receive Status Register */
#define RSR_PRX  0x01  /**< Packet Received Intact */
#define RSR_CRC  0x02  /**< CRC Error */
#define RSR_FAE  0x04  /**< Frame Alignment Error */
#define RSR_FO   0x08  /**< FIFO Overrun */
#define RSR_MPA  0x10  /**< Missed Packet */
#define RSR_PHY  0x20  /**< Multicast Address Match */
#define RSR_DIS  0x40  /**< Receiver Disabled */
#define RSR_DFR  0x80  /**< In later manuals: Deferring */

typedef struct {
	/** DDF device */
	ddf_dev_t *dev;
	/** Parent session */
	async_sess_t *parent_sess;
	/* Device configuration */
	void *base_port; /**< Port assigned from ISA configuration **/
	void *port;
	void *data_port;
	int irq;
	nic_address_t mac;
	
	uint8_t start_page;  /**< Ring buffer start page */
	uint8_t stop_page;   /**< Ring buffer stop page */
	
	/* Send queue */
	struct {
		bool dirty;    /**< Buffer contains a packet */
		size_t size;   /**< Packet size */
		uint8_t page;  /**< Starting page of the buffer */
	} sq;
	fibril_mutex_t sq_mutex;
	fibril_condvar_t sq_cv;
	
	/* Driver run-time variables */
	bool probed;
	bool up;

	/* Irq code with assigned addresses for this device */
	irq_code_t code;

	/* Copy of the receive configuration register */
	uint8_t receive_configuration;

	/* Device statistics */
	// TODO: shouldn't be these directly in device.h - nic_device_stats?
	uint64_t misses;     /**< Receive frame misses */
	uint64_t underruns;  /**< FIFO underruns */
	uint64_t overruns;   /**< FIFO overruns */
} ne2k_t;

extern errno_t ne2k_probe(ne2k_t *);
extern errno_t ne2k_up(ne2k_t *);
extern void ne2k_down(ne2k_t *);
extern void ne2k_send(nic_t *, void *, size_t);
extern void ne2k_interrupt(nic_t *, uint8_t, uint8_t);

extern void ne2k_set_accept_mcast(ne2k_t *, int);
extern void ne2k_set_accept_bcast(ne2k_t *, int);
extern void ne2k_set_promisc_phys(ne2k_t *, int);
extern void ne2k_set_mcast_hash(ne2k_t *, uint64_t);
extern void ne2k_set_physical_address(ne2k_t *, const nic_address_t *address);

#endif

/** @}
 */
