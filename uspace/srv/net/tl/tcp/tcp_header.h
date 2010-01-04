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

/** @addtogroup tcp
 *  @{
 */

/** @file
 *  TCP header definition.
 *  Based on the RFC~793.
 */

#ifndef __NET_TCP_HEADER_H__
#define __NET_TCP_HEADER_H__

#include <sys/types.h>

/** TCP header size in bytes.
 */
#define TCP_HEADER_SIZE			sizeof( tcp_header_t )

/** Returns the actual TCP header length in bytes.
 *  @param[in] header The TCP packet header.
 */
#define TCP_HEADER_LENGTH( header )		(( header )->header_length * 4u )

/** Returns the TCP header length.
 *  @param[in] length The TCP header length in bytes.
 */
#define TCP_COMPUTE_HEADER_LENGTH( length )		(( uint8_t ) (( length ) / 4u ))

/** Type definition of the transmission datagram header.
 *  @see tcp_header
 */
typedef struct tcp_header	tcp_header_t;

/** Type definition of the transmission datagram header pointer.
 *  @see tcp_header
 */
typedef tcp_header_t *		tcp_header_ref;

/** Type definition of the transmission datagram header option.
 *  @see tcp_option
 */
typedef struct tcp_option	tcp_option_t;

/** Type definition of the transmission datagram header option pointer.
 *  @see tcp_option
 */
typedef tcp_option_t *		tcp_option_ref;

/** Type definition of the Maximum segment size TCP option.
 *  @see ...
 */
typedef struct tcp_max_segment_size_option	tcp_max_segment_size_option_t;

/** Type definition of the Maximum segment size TCP option pointer.
 *  @see tcp_max_segment_size_option
 */
typedef tcp_max_segment_size_option_t *		tcp_max_segment_size_option_ref;

/** Transmission datagram header.
 */
struct tcp_header{
	/** The source port number.
	 */
	uint16_t	source_port;
	/** The destination port number.
	 */
	uint16_t	destination_port;
	/** The sequence number of the first data octet in this segment (except when SYN is present).
	 *  If SYN is present the sequence number is the initial sequence number (ISN) and the first data octet is ISN+1.
	 */
	uint32_t	sequence_number;
	/** If the ACK control bit is set this field contains the value of the next sequence number the sender of the segment is expecting to receive.
	 *  Once a~connection is established this is always sent.
	 *  @see acknowledge
	 */
	uint32_t	acknowledgement_number;
#ifdef ARCH_IS_BIG_ENDIAN
	/** The number of 32~bit words in the TCP Header.
	 *  This indicates where the data begins.
	 *  The TCP header (even one including options) is an integral number of 32~bits long.
	 */
	uint8_t	header_length:4;
	/** Four bits reserved for future use.
	 *  Must be zero.
	 */
	uint8_t	reserved1:4;
#else
	/** Four bits reserved for future use.
	 *  Must be zero.
	 */
	uint8_t	reserved1:4;
	/** The number of 32~bit words in the TCP Header.
	 *  This indicates where the data begins.
	 *  The TCP header (even one including options) is an integral number of 32~bits long.
	 */
	uint8_t	header_length:4;
#endif
#ifdef ARCH_IS_BIG_ENDIAN
	/** Two bits reserved for future use.
	 *  Must be zero.
	 */
	uint8_t	reserved2:2;
	/** Urgent Pointer field significant.
	 *  @see tcp_header:urgent_pointer
	 */
	uint8_t	urgent:1;
	/** Acknowledgment field significant
	 *  @see tcp_header:acknowledgement_number
	 */
	uint8_t	acknowledge:1;
	/** Push function.
	 */
	uint8_t	push:1;
	/** Reset the connection.
	 */
	uint8_t	reset:1;
	/** Synchronize the sequence numbers.
	 */
	uint8_t	synchronize:1;
	/** No more data from the sender.
	 */
	uint8_t	finalize:1;
#else
	/** No more data from the sender.
	 */
	uint8_t	finalize:1;
	/** Synchronize the sequence numbers.
	 */
	uint8_t	synchronize:1;
	/** Reset the connection.
	 */
	uint8_t	reset:1;
	/** Push function.
	 */
	uint8_t	push:1;
	/** Acknowledgment field significant.
	 *  @see tcp_header:acknowledgement_number
	 */
	uint8_t	acknowledge:1;
	/** Urgent Pointer field significant.
	 *  @see tcp_header:urgent_pointer
	 */
	uint8_t	urgent:1;
	/** Two bits reserved for future use.
	 *  Must be zero.
	 */
	uint8_t	reserved2:2;
#endif
	/** The number of data octets beginning with the one indicated in the acknowledgment field which the sender of this segment is willing to accept.
	 *  @see tcp_header:acknowledge
	 */
	uint16_t	window;
	/** The checksum field is the 16~bit one's complement of the one's complement sum of all 16~bit words in the header and text.
	 *  If a~segment contains an odd number of header and text octets to be checksummed, the last octet is padded on the right with zeros to form a~16~bit word for checksum purposes.
	 *  The pad is not transmitted as part of the segment.
	 *  While computing the checksum, the checksum field itself is replaced with zeros.
	 *  The checksum also coves a~pseudo header conceptually.
	 *  The pseudo header conceptually prefixed to the TCP header contains the source address, the destination address, the protocol, and the TCP length.
	 *  This information gives protection against misrouted datagrams.
	 *  If the computed checksum is zero, it is transmitted as all ones (the equivalent in one's complement arithmetic).
	 */
	uint16_t	checksum;
	/** This field communicates the current value of the urgent pointer as a~positive offset from the sequence number in this segment.
	 *  The urgent pointer points to the sequence number of the octet following the urgent data.
	 *  This field is only be interpreted in segments with the URG control bit set.
	 *  @see tcp_header:urgent
	 */
	uint16_t	urgent_pointer;
} __attribute__ ((packed));

/** Transmission datagram header option.
 */
struct tcp_option{
	/** Option type.
	 */
	uint8_t		type;
	/** Option length.
	 */
	uint8_t		length;
};

/** Maximum segment size TCP option.
 */
struct tcp_max_segment_size_option{
	/** TCP option.
	 *  @see TCPOPT_MAX_SEGMENT_SIZE
	 *  @see TCPOPT_MAX_SEGMENT_SIZE_LENGTH
	 */
	tcp_option_t	option;
	/** Maximum segment size in bytes.
	 */
	uint16_t		max_segment_size;
} __attribute__ ((packed));

#endif

/** @}
 */
