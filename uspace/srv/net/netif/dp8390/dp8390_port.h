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

/** Macro for difining functions.
 *  @param[in] function The function type and name definition.
 *  @param[in] params The function parameters definition.
 */
#define _PROTOTYPE( function, params ) function params

/** Success error code.
 */
#define OK	EOK

/** Type definition of the unsigned byte.
 */
typedef uint8_t u8_t;

/** Type definition of the unsigned short.
 */
typedef uint16_t u16_t;

/** Compares two memory blocks.
 *  @param[in] first The first memory block.
 *  @param[in] second The second memory block.
 *  @param[in] size The blocks size in bytes.
 *  @returns 0 if equeal.
 *  @returns -1 if the first is greater than the second.
 *  @returns 1 if the second is greater than the first.
 */
#define memcmp( first, second, size )	bcmp(( char * ) ( first ), ( char * ) ( second ), ( size ))

/** Reads 1 byte.
 *  @param[in] port The address to be read.
 *  @returns The read value.
 */
#define inb( port )	pio_read_8(( ioport8_t * ) ( port ))

/** Reads 1 word (2 bytes).
 *  @param[in] port The address to be read.
 *  @returns The read value.
 */
#define inw( port )	pio_read_16(( ioport16_t * ) ( port ))

/** Writes 1 byte.
 *  @param[in] port The address to be written.
 *  @param[in] value The value to be written.
 */
#define outb( port, value )	pio_write_8(( ioport8_t * ) ( port ), ( value ))

/** Writes 1 word (2 bytes).
 *  @param[in] port The address to be written.
 *  @param[in] value The value to be written.
 */
#define outw( port, value )	pio_write_16(( ioport16_t * ) ( port ), ( value ))

/** Prints out the driver critical error.
 *  Does not call the system panic().
 */
#define panic( ... )	printf( "%s%s%d", __VA_ARGS__ )

/** Copies a memory block.
 *  @param proc The source process. Ignored parameter.
 *  @param src_s Ignored parameter.
 *  @param[in] src The source address.
 *  @param me The current proces. Ignored parameter.
 *  @param dst_s Ignored parameter.
 *  @param[in] dst The destination address.
 *  @param[in] bytes The block size in bytes.
 *  @returns EOK.
 */
#define sys_vircopy( proc, src_s, src, me, dst_s, dst, bytes )	({ memcpy(( void * )( dst ), ( void * )( src ), ( bytes )); EOK; })

/** Reads a memory block byte by byte.
 *  @param[in] port The address to be written.
 *  @param proc The source process. Ignored parameter.
 *  @param[in] dst The destination address.
 *  @param[in] bytes The block size in bytes.
 */
#define do_vir_insb( port, proc, dst, bytes )	insb(( port ), ( void * )( dst ), ( bytes ))

/** Reads a memory block word by word (2 bytes).
 *  @param[in] port The address to be written.
 *  @param proc The source process. Ignored parameter.
 *  @param[in] dst The destination address.
 *  @param[in] bytes The block size in bytes.
 */
#define do_vir_insw( port, proc, dst, bytes )	insw(( port ), ( void * )( dst ), ( bytes ))

/** Writes a memory block byte by byte.
 *  @param[in] port The address to be written.
 *  @param proc The source process. Ignored parameter.
 *  @param[in] src The source address.
 *  @param[in] bytes The block size in bytes.
 */
#define do_vir_outsb( port, proc, src, bytes )	outsb(( port ), ( void * )( src ), ( bytes ))

/** Writes a memory block word by word (2 bytes).
 *  @param[in] port The address to be written.
 *  @param proc The source process. Ignored parameter.
 *  @param[in] src The source address.
 *  @param[in] bytes The block size in bytes.
 */
#define do_vir_outsw( port, proc, src, bytes )	outsw(( port ), ( void * )( src ), ( bytes ))

/* com.h */
/* Bits in 'DL_MODE' field of DL requests. */
#  define DL_NOMODE		0x0
#  define DL_PROMISC_REQ	0x2
#  define DL_MULTI_REQ		0x4
#  define DL_BROAD_REQ		0x8

/* const.h */
/** True value.
 */
#define TRUE               1	/* used for turning integers into Booleans */

/** False value.
 */
#define FALSE              0	/* used for turning integers into Booleans */

/** No number value.
 */
#define NO_NUM        0x8000	/* used as numerical argument to panic() */

/* devio.h */
//typedef u16_t port_t;
/** Type definition of a port.
 */
typedef int port_t;

/* dl_eth.h */
/** Ethernet statistics.
 */
typedef struct eth_stat
{
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

/* errno.h */
/** Generic error.
 */
#define EGENERIC     EINVAL

/* ether.h */
/** Minimum Ethernet packet size in bytes.
 */
#define ETH_MIN_PACK_SIZE		  60

/** Maximum Ethernet packet size in bytes.
 */
#define ETH_MAX_PACK_SIZE_TAGGED	1518

/** Ethernet address type definition.
 */
typedef struct ether_addr
{
	/** Address data.
	 */
	u8_t ea_addr[6];
} ether_addr_t;

/* type.h */
/** Type definition of the physical addresses and lengths in bytes.
 */
typedef unsigned long phys_bytes;

/** Type definition of the virtual addresses and lengths in bytes.
 */
typedef unsigned int vir_bytes;

/** Type definition of the input/output vector.
 */
typedef struct {
	/** Address of an I/O buffer.
	 */
	vir_bytes iov_addr;
	/** Sizeof an I/O buffer.
	 */
	vir_bytes iov_size;
} iovec_t;

#endif

/** @}
 */
