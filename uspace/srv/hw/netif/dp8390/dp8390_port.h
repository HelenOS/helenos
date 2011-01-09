/*
 * Copyright (c) 2009 Lukas Mejdrech
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

/** @addtogroup dp8390
 *  @{
 */

/** @file
 *  DP8390 network interface types and structures ports.
 */

#ifndef __NET_NETIF_DP8390_PORT_H__
#define __NET_NETIF_DP8390_PORT_H__

#include <errno.h>
#include <mem.h>
#include <stdio.h>
#include <libarch/ddi.h>
#include <sys/types.h>

/** Reads 1 byte.
 *  @param[in] port The address to be read.
 *  @returns The read value.
 */
#define inb(port)  pio_read_8((ioport8_t *) (port))

/** Reads 1 word (2 bytes).
 *  @param[in] port The address to be read.
 *  @returns The read value.
 */
#define inw(port)  pio_read_16((ioport16_t *) (port))

/** Writes 1 byte.
 *  @param[in] port The address to be written.
 *  @param[in] value The value to be written.
 */
#define outb(port, value)  pio_write_8((ioport8_t *) (port), (value))

/** Writes 1 word (2 bytes).
 *  @param[in] port The address to be written.
 *  @param[in] value The value to be written.
 */
#define outw(port, value)  pio_write_16((ioport16_t *) (port), (value))

/** Type definition of a port.
 */
typedef long port_t;

/** Ethernet statistics.
 */
typedef struct eth_stat {
	/** Number of receive errors.
	 */
	unsigned long ets_recvErr;
	/** Number of send error.
	 */
	unsigned long ets_sendErr;
	/** Number of buffer overwrite warnings.
	 */
	unsigned long ets_OVW;
	/** Number of crc errors of read.
	 */
	unsigned long ets_CRCerr;
	/** Number of frames not alligned (number of bits % 8 != 0).
	 */
	unsigned long ets_frameAll;
	/** Number of packets missed due to slow processing.
	 */
	unsigned long ets_missedP;
	/** Number of packets received.
	 */
	unsigned long ets_packetR;
	/** Number of packets transmitted.
	 */
	unsigned long ets_packetT;
	/** Number of transmission defered (Tx was busy).
	 */
	unsigned long ets_transDef;
	/** Number of collissions.
	 */
	unsigned long ets_collision;
	/** Number of Tx aborted due to excess collisions.
	 */
	unsigned long ets_transAb;
	/** Number of carrier sense lost.
	 */
	unsigned long ets_carrSense;
	/** Number of FIFO underruns (processor too busy).
	 */
	unsigned long ets_fifoUnder;
	/** Number of FIFO overruns (processor too busy).
	 */
	unsigned long ets_fifoOver;
	/** Number of times unable to transmit collision sig.
	 */
	unsigned long ets_CDheartbeat;
	/** Number of times out of window collision.
	 */
	unsigned long ets_OWC;
} eth_stat_t;

/** Minimum Ethernet packet size in bytes.
 */
#define ETH_MIN_PACK_SIZE  60

/** Maximum Ethernet packet size in bytes.
 */
#define ETH_MAX_PACK_SIZE_TAGGED  1518

/** Ethernet address type definition.
 */
typedef struct ether_addr {
	/** Address data.
	 */
	uint8_t ea_addr[6];
} ether_addr_t;

#endif

/** @}
 */
