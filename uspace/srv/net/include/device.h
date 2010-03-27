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

/** @addtogroup netif
 *  @{
 */

/** @file
 *  Device identifier, state and usage statistics.
 */

#ifndef __NET_DEVICE_ID_TYPE_H__
#define __NET_DEVICE_ID_TYPE_H__

#include "../structures/int_map.h"

/** Device identifier to generic type map declaration.
 */
#define DEVICE_MAP_DECLARE		INT_MAP_DECLARE

/** Device identifier to generic type map implementation.
 */
#define DEVICE_MAP_IMPLEMENT	INT_MAP_IMPLEMENT

/** Invalid device identifier.
 */
#define DEVICE_INVALID_ID		(-1)

/** Device identifier type.
 */
typedef int	device_id_t;

/** Device state type.
 */
typedef enum device_state	device_state_t;

/** Type definition of the device usage statistics.
 *  @see device_stats
 */
typedef struct device_stats	device_stats_t;

/** Type definition of the device usage statistics pointer.
 *  @see device_stats
 */
typedef device_stats_t *	device_stats_ref;

/** Device state.
 */
enum	device_state{
	/** Device not present or not initialized.
	 */
	NETIF_NULL = 0,
	/** Device present and stopped.
	 */
	NETIF_STOPPED,
	/** Device present and active.
	 */
	NETIF_ACTIVE,
	/** Device present but unable to transmit.
	 */
	NETIF_CARRIER_LOST
};

/** Device usage statistics.
 */
struct	device_stats{
	/** Total packets received.
	 */
	unsigned long receive_packets;
	/** Total packets transmitted.
	 */
	unsigned long send_packets;
	/** Total bytes received.
	 */
	unsigned long receive_bytes;
	/** Total bytes transmitted.
	 */
	unsigned long send_bytes;
	/** Bad packets received counter.
	 */
	unsigned long receive_errors;
	/** Packet transmition problems counter.
	 */
	unsigned long send_errors;
	/** No space in buffers counter.
	 */
	unsigned long receive_dropped;
	/** No space available counter.
	 */
	unsigned long send_dropped;
	/** Total multicast packets received.
	 */
	unsigned long multicast;
	/** The number of collisions due to congestion on the medium.
	 */
	unsigned long collisions;

	/* detailed receive_errors: */
	/** Received packet length error counter.
	 */
	unsigned long receive_length_errors;
	/** Receiver buffer overflow counter.
	 */
	unsigned long receive_over_errors;
	/** Received packet with crc error counter.
	 */
	unsigned long receive_crc_errors;
	/** Received frame alignment error counter.
	 */
	unsigned long receive_frame_errors;
	/** Receiver fifo overrun counter.
	 */
	unsigned long receive_fifo_errors;
	/** Receiver missed packet counter.
	 */
	unsigned long receive_missed_errors;

	/* detailed send_errors */
	/** Transmitter aborted counter.
	 */
	unsigned long send_aborted_errors;
	/** Transmitter carrier errors counter.
	 */
	unsigned long send_carrier_errors;
	/** Transmitter fifo overrun counter.
	 */
	unsigned long send_fifo_errors;
	/** Transmitter carrier errors counter.
	 */
	unsigned long send_heartbeat_errors;
	/** Transmitter window errors counter.
	 */
	unsigned long send_window_errors;

	/* for cslip etc */
	/** Total compressed packets received.
	 */
	unsigned long receive_compressed;
	/** Total compressed packet transmitted.
	 */
	unsigned long send_compressed;
};

#endif

/** @}
 */

