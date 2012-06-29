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
 * @{
 */

/** @file
 * TCP header definition.
 * Based on the RFC 793.
 */

#ifndef NET_TCP_HEADER_H_
#define NET_TCP_HEADER_H_

#include <sys/types.h>

/** TCP header size in bytes. */
#define TCP_HEADER_SIZE				sizeof(tcp_header_t)

/** Returns the actual TCP header length in bytes.
 * @param[in] header The TCP packet header.
 */
#define TCP_HEADER_LENGTH(header)		(GET_TCP_HEADER_LENGTH(header) * 4U)

/** Returns the TCP header length.
 * @param[in] length The TCP header length in bytes.
 */
#define TCP_COMPUTE_HEADER_LENGTH(length)	((uint8_t) ((length) / 4U))

/** Type definition of the transmission datagram header.
 * @see tcp_header
 */
typedef struct tcp_header tcp_header_t;

/** Type definition of the transmission datagram header option.
 * @see tcp_option
 */
typedef struct tcp_option tcp_option_t;

/** Type definition of the Maximum segment size TCP option. */
typedef struct tcp_max_segment_size_option tcp_max_segment_size_option_t;

/** Transmission datagram header. */
struct tcp_header {
	uint16_t source_port;
	uint16_t destination_port;
	uint32_t sequence_number;
	uint32_t acknowledgement_number;

	uint8_t hlr; /* header length, reserved1 */

#define GET_TCP_HEADER_LENGTH(header) \
	(((header)->hlr & 0xf0) >> 4)
#define SET_TCP_HEADER_LENGTH(header, length) \
	((header)->hlr = \
	 ((length & 0x0f) << 4) | ((header)->hlr & 0x0f))

#define GET_TCP_HEADER_RESERVED1(header) \
	((header)->hlr & 0x0f)
#define SET_TCP_HEADER_RESERVED1(header, reserved1) \
	((header)->hlr = \
	 (reserved1 & 0x0f) | ((header)->hlr & 0xf0))

	/* reserved2, urgent, acknowledge, push, reset, synchronize, finalize */
	uint8_t ruaprsf;  

#define GET_TCP_HEADER_RESERVED2(header) \
	(((header)->ruaprsf & 0xc0) >> 6)
#define SET_TCP_HEADER_RESERVED2(header, reserved2) \
	((header)->ruaprsf = \
	 ((reserved2 & 0x03) << 6) | ((header)->ruaprsf & 0x3f))

#define GET_TCP_HEADER_URGENT(header) \
	(((header)->ruaprsf & 0x20) >> 5)
#define SET_TCP_HEADER_URGENT(header, urgent) \
	((header)->ruaprsf = \
	 ((urgent & 0x01) << 5) | ((header)->ruaprsf & 0xdf))

#define GET_TCP_HEADER_ACKNOWLEDGE(header) \
	(((header)->ruaprsf & 0x10) >> 4)
#define SET_TCP_HEADER_ACKNOWLEDGE(header, acknowledge) \
	((header)->ruaprsf = \
	 ((acknowledge & 0x01) << 4) | ((header)->ruaprsf & 0xef))

#define GET_TCP_HEADER_PUSH(header) \
	(((header)->ruaprsf & 0x08) >> 3)
#define SET_TCP_HEADER_PUSH(header, push) \
	((header)->ruaprsf = \
	 ((push & 0x01) << 3) | ((header)->ruaprsf & 0xf7))

#define GET_TCP_HEADER_RESET(header) \
	(((header)->ruaprsf & 0x04) >> 2)
#define SET_TCP_HEADER_RESET(header, reset) \
	((header)->ruaprsf = \
	 ((reset & 0x01) << 2) | ((header)->ruaprsf & 0xfb))

#define GET_TCP_HEADER_SYNCHRONIZE(header) \
	(((header)->ruaprsf & 0x02) >> 1)
#define SET_TCP_HEADER_SYNCHRONIZE(header, synchronize) \
	((header)->ruaprsf = \
	 ((synchronize & 0x01) << 1) | ((header)->ruaprsf & 0xfd))

#define GET_TCP_HEADER_FINALIZE(header) \
	((header)->ruaprsf & 0x01)
#define SET_TCP_HEADER_FINALIZE(header, finalize) \
	((header)->ruaprsf = \
	 (finalize & 0x01) | ((header)->ruaprsf & 0xfe))

	uint16_t window;
	uint16_t checksum;
	uint16_t urgent_pointer;
} __attribute__ ((packed));

/** Transmission datagram header option. */
struct tcp_option {
	/** Option type. */
	uint8_t type;
	/** Option length. */
	uint8_t length;
};

/** Maximum segment size TCP option. */
struct tcp_max_segment_size_option {
	/** TCP option.
	 * @see TCPOPT_MAX_SEGMENT_SIZE
	 * @see TCPOPT_MAX_SEGMENT_SIZE_LENGTH
	 */
	tcp_option_t option;
	
	/** Maximum segment size in bytes. */
	uint16_t max_segment_size;
} __attribute__ ((packed));

#endif

/** @}
 */
