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
 *  DP8390 network interface definitions.
 */

#ifndef __NET_NETIF_DP8390_H__
#define __NET_NETIF_DP8390_H__

#include <net/packet.h>
#include "dp8390_port.h"

/** Input/output size */
#define DP8390_IO_SIZE  0x0020

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

/* Bits in dp_cr */
#define CR_STP		0x01	/* Stop: software reset              */
#define CR_STA		0x02	/* Start: activate NIC               */
#define CR_TXP		0x04	/* Transmit Packet                   */
#define CR_DMA		0x38	/* Mask for DMA control              */
#define CR_DM_NOP	0x00	/* DMA: No Operation                 */
#define CR_DM_RR	0x08	/* DMA: Remote Read                  */
#define CR_DM_RW	0x10	/* DMA: Remote Write                 */
#define CR_DM_SP	0x18	/* DMA: Send Packet                  */
#define CR_DM_ABORT	0x20	/* DMA: Abort Remote DMA Operation   */
#define CR_PS		0xC0	/* Mask for Page Select              */
#define CR_PS_P0	0x00	/* Register Page 0                   */
#define CR_PS_P1	0x40	/* Register Page 1                   */
#define CR_PS_P2	0x80	/* Register Page 2                   */
#define CR_PS_T1	0xC0	/* Test Mode Register Map            */

/* Bits in dp_isr */
#define ISR_PRX		0x01	/* Packet Received with no errors    */
#define ISR_PTX		0x02	/* Packet Transmitted with no errors */
#define ISR_RXE		0x04	/* Receive Error                     */
#define ISR_TXE		0x08	/* Transmit Error                    */
#define ISR_OVW		0x10	/* Overwrite Warning                 */
#define ISR_CNT		0x20	/* Counter Overflow                  */
#define ISR_RDC		0x40	/* Remote DMA Complete               */
#define ISR_RST		0x80	/* Reset Status                      */

/* Bits in dp_imr */
#define IMR_PRXE	0x01	/* Packet Received iEnable           */
#define IMR_PTXE	0x02	/* Packet Transmitted iEnable        */
#define IMR_RXEE	0x04	/* Receive Error iEnable             */
#define IMR_TXEE	0x08	/* Transmit Error iEnable            */
#define IMR_OVWE	0x10	/* Overwrite Warning iEnable         */
#define IMR_CNTE	0x20	/* Counter Overflow iEnable          */
#define IMR_RDCE	0x40	/* DMA Complete iEnable              */

/* Bits in dp_dcr */
#define DCR_WTS		0x01	/* Word Transfer Select              */
#define DCR_BYTEWIDE	0x00	/* WTS: byte wide transfers          */
#define DCR_WORDWIDE	0x01	/* WTS: word wide transfers          */
#define DCR_BOS		0x02	/* Byte Order Select                 */
#define DCR_LTLENDIAN	0x00	/* BOS: Little Endian                */
#define DCR_BIGENDIAN	0x02	/* BOS: Big Endian                   */
#define DCR_LAS		0x04	/* Long Address Select               */
#define DCR_BMS		0x08	/* Burst Mode Select
				 * Called Loopback Select (LS) in 
				 * later manuals. Should be set.     */
#define DCR_AR		0x10	/* Autoinitialize Remote             */
#define DCR_FTS		0x60	/* Fifo Threshold Select             */
#define DCR_2BYTES	0x00	/* 2 bytes                           */
#define DCR_4BYTES	0x40	/* 4 bytes                           */
#define DCR_8BYTES	0x20	/* 8 bytes                           */
#define DCR_12BYTES	0x60	/* 12 bytes                          */

/* Bits in dp_tcr */
#define TCR_CRC		0x01	/* Inhibit CRC                       */
#define TCR_ELC		0x06	/* Encoded Loopback Control          */
#define TCR_NORMAL	0x00	/* ELC: Normal Operation             */
#define TCR_INTERNAL	0x02	/* ELC: Internal Loopback            */
#define TCR_0EXTERNAL	0x04	/* ELC: External Loopback LPBK=0     */
#define TCR_1EXTERNAL	0x06	/* ELC: External Loopback LPBK=1     */
#define TCR_ATD		0x08	/* Auto Transmit Disable             */
#define TCR_OFST	0x10	/* Collision Offset Enable (be nice) */

/* Bits in dp_tsr */
#define TSR_PTX		0x01	/* Packet Transmitted (without error)*/
#define TSR_DFR		0x02	/* Transmit Deferred, reserved in
				 * later manuals.		     */
#define TSR_COL		0x04	/* Transmit Collided                 */
#define TSR_ABT		0x08	/* Transmit Aborted                  */
#define TSR_CRS		0x10	/* Carrier Sense Lost                */
#define TSR_FU		0x20	/* FIFO Underrun                     */
#define TSR_CDH		0x40	/* CD Heartbeat                      */
#define TSR_OWC		0x80	/* Out of Window Collision           */

/* Bits in tp_rcr */
#define RCR_SEP		0x01	/* Save Errored Packets              */
#define RCR_AR		0x02	/* Accept Runt Packets               */
#define RCR_AB		0x04	/* Accept Broadcast                  */
#define RCR_AM		0x08	/* Accept Multicast                  */
#define RCR_PRO		0x10	/* Physical Promiscuous              */
#define RCR_MON		0x20	/* Monitor Mode                      */

/* Bits in dp_rsr */
#define RSR_PRX		0x01	/* Packet Received Intact            */
#define RSR_CRC		0x02	/* CRC Error                         */
#define RSR_FAE		0x04	/* Frame Alignment Error             */
#define RSR_FO		0x08	/* FIFO Overrun                      */
#define RSR_MPA		0x10	/* Missed Packet                     */
#define RSR_PHY		0x20	/* Multicast Address Match           */
#define RSR_DIS		0x40	/* Receiver Disabled                 */
#define RSR_DFR		0x80	/* In later manuals: Deferring       */

/** Type definition of the receive header
 *
 */
typedef struct dp_rcvhdr {
	/** Copy of rsr */
	uint8_t dr_status;
	
	/** Pointer to next packet */
	uint8_t dr_next;
	
	/** Receive Byte Count Low */
	uint8_t dr_rbcl;
	
	/** Receive Byte Count High */
	uint8_t dr_rbch;
} dp_rcvhdr_t;

/** Page size */
#define DP_PAGESIZE  256

/** Read 1 byte from the zero page register.
 *  @param[in] dep The network interface structure.
 *  @param[in] reg The register offset.
 *  @returns The read value.
 */
#define inb_reg0(dep, reg)  (inb(dep->de_dp8390_port + reg))

/** Write 1 byte zero page register.
 *  @param[in] dep The network interface structure.
 *  @param[in] reg The register offset.
 *  @param[in] data The value to be written.
 */
#define outb_reg0(dep, reg, data)  (outb(dep->de_dp8390_port + reg, data))

/** Read 1 byte from the first page register.
 *  @param[in] dep The network interface structure.
 *  @param[in] reg The register offset.
 *  @returns The read value.
 */
#define inb_reg1(dep, reg)  (inb(dep->de_dp8390_port + reg))

/** Write 1 byte first page register.
 *  @param[in] dep The network interface structure.
 *  @param[in] reg The register offset.
 *  @param[in] data The value to be written.
 */
#define outb_reg1(dep, reg, data)  (outb(dep->de_dp8390_port + reg, data))

/* Software interface to the dp8390 driver */

struct dpeth;

typedef void (*dp_initf_t)(struct dpeth *dep);
typedef void (*dp_stopf_t)(struct dpeth *dep);
typedef void (*dp_user2nicf_t)(struct dpeth *dep, void *buf, size_t offset, int nic_addr, size_t size);
typedef void (*dp_nic2userf_t)(struct dpeth *dep, int nic_addr, void *buf, size_t offset, size_t size);
typedef void (*dp_getblock_t)(struct dpeth *dep, int page, size_t offset, size_t size, void *dst);

#define SENDQ_NR     2  /* Maximum size of the send queue */
#define SENDQ_PAGES  6  /* 6 * DP_PAGESIZE >= 1514 bytes */

typedef struct dpeth {
	/*
	 * The de_base_port field is the starting point of the probe.
	 * The conf routine also fills de_irq. If the probe
	 * routine knows the irq and/or memory address because they are
	 * hardwired in the board, the probe should modify these fields.
	 * Futhermore, the probe routine should also fill in de_initf and
	 * de_stopf fields with the appropriate function pointers.
	 */
	port_t de_base_port;
	int de_irq;
	dp_initf_t de_initf;
	dp_stopf_t de_stopf;
	
	/*
	 * The initf function fills the following fields. Only cards that do
	 * programmed I/O fill in the de_pata_port field.
	 * In addition, the init routine has to fill in the sendq data
	 * structures.
	 */
	ether_addr_t de_address;
	port_t de_dp8390_port;
	port_t de_data_port;
	int de_16bit;
	long de_ramsize;
	int de_offset_page;
	int de_startpage;
	int de_stoppage;
	
	/* Do it yourself send queue */
	struct sendq {
		int sq_filled;    /* this buffer contains a packet */
		int sq_size;      /* with this size */
		int sq_sendpage;  /* starting page of the buffer */
	} de_sendq[SENDQ_NR];
	
	int de_sendq_nr;
	int de_sendq_head;  /* Enqueue at the head */
	int de_sendq_tail;  /* Dequeue at the tail */
	
	/* Fields for internal use by the dp8390 driver. */
	eth_stat_t de_stat;
	dp_user2nicf_t de_user2nicf;
	dp_nic2userf_t de_nic2userf;
	dp_getblock_t de_getblockf;
	
	/* Driver flags */
	bool up;
	bool enabled;
	bool stopped;
	bool sending;
	bool send_avail;
} dpeth_t;

#endif

/** @}
 */
