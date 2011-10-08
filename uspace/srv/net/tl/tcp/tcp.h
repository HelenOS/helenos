/*
 * Copyright (c) 2008 Lukas Mejdrech
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
 * TCP module.
 */

#ifndef NET_TCP_H_
#define NET_TCP_H_

#include <async.h>
#include <fibril_synch.h>
#include <net/packet.h>
#include <net/device.h>
#include <socket_core.h>
#include <tl_common.h>

/** Type definition of the TCP global data.
 * @see tcp_globals
 */
typedef struct tcp_globals tcp_globals_t;

/** Type definition of the TCP socket specific data.
 * @see tcp_socket_data
 */
typedef struct tcp_socket_data tcp_socket_data_t;

/** Type definition of the TCP operation data.
 * @see tcp_operation
 */
typedef struct tcp_operation tcp_operation_t;

/** TCP socket state type definition.
 * @see tcp_socket_state
 */
typedef enum tcp_socket_state tcp_socket_state_t;

/** TCP socket state. */
enum tcp_socket_state {
	/** Initial.
	 *
	 * Not connected or bound.
	 */
	TCP_SOCKET_INITIAL,
	
	/** Listening.
	 *
	 * Awaiting a connection request from another TCP layer.
	 * When SYN is received a new bound socket in the
	 * TCP_SOCKET_SYN_RECEIVED state should be created.
	 */
	TCP_SOCKET_LISTEN,
	
	/** Connecting issued.
	 *
	 * A SYN has been sent, and TCP is awaiting the response SYN.
	 * Should continue to the TCP_SOCKET_ESTABLISHED state.
	 */
	TCP_SOCKET_SYN_SENT,
	
	/** Connecting received.
	 *
	 * A SYN has been received, a SYN has been sent, and TCP is awaiting an
	 * ACK. Should continue to the TCP_SOCKET_ESTABLISHED state.
	 */
	TCP_SOCKET_SYN_RECEIVED,
	
	/** Connected.
	 *
	 * The three-way handshake has been completed.
	 */
	TCP_SOCKET_ESTABLISHED,
	
	/** Closing started.
	 *
	 * The local application has issued a CLOSE.
	 * TCP has sent a FIN, and is awaiting an ACK or a FIN.
	 * Should continue to the TCP_SOCKET_FIN_WAIT_2 state when an ACK is
	 * received.
	 * Should continue to the TCP_SOCKET_CLOSING state when a FIN is
	 * received.
	 */
	TCP_SOCKET_FIN_WAIT_1,
	
	/** Closing confirmed.
	 *
	 * A FIN has been sent, and an ACK received.
	 * TCP is awaiting a~FIN from the remote TCP layer.
	 * Should continue to the TCP_SOCKET_CLOSING state.
	 */
	TCP_SOCKET_FIN_WAIT_2,
	
	/** Closing.
	 *
	 * A FIN has been sent, a FIN has been received, and an ACK has been
	 * sent.
	 * TCP is awaiting an ACK for the FIN that was sent.
	 * Should continue to the TCP_SOCKET_TIME_WAIT state.
	 */
	TCP_SOCKET_CLOSING,
	
	/** Closing received.
	 *
	 * TCP has received a FIN, and has sent an ACK.
	 * It is awaiting a close request from the local application before
	 * sending a FIN.
	 * Should continue to the TCP_SOCKET_SOCKET_LAST_ACK state.
	 */
	TCP_SOCKET_CLOSE_WAIT,
	
	/**
	 * A FIN has been received, and an ACK and a FIN have been sent.
	 * TCP is awaiting an ACK.
	 * Should continue to the TCP_SOCKET_TIME_WAIT state.
	 */
	TCP_SOCKET_LAST_ACK,
	
	/** Closing finished.
	 *
	 * FINs have been received and ACK’d, and TCP is waiting two MSLs to
	 * remove the connection from the table.
	 */
	TCP_SOCKET_TIME_WAIT,
	
	/** Closed.
	 *
	 * Imaginary, this indicates that a connection has been removed from
	 * the connection table.
	 */
	TCP_SOCKET_CLOSED
};

/** TCP operation data. */
struct tcp_operation {
	/** Operation result. */
	int result;
	/** Safety lock. */
	fibril_mutex_t mutex;
	/** Operation result signaling. */
	fibril_condvar_t condvar;
};

/** TCP socket specific data. */
struct tcp_socket_data {
	/** TCP socket state. */
	tcp_socket_state_t state;
	
	/**
	 * Data fragment size.
	 * Sending optimalization.
	 */
	size_t data_fragment_size;
	
	/** Device identifier. */
	nic_device_id_t device_id;
	
	/**
	 * Listening backlog.
	 * The maximal number of connected but not yet accepted sockets.
	 */
	int backlog;
	
	/**
	 * Parent listening socket identifier.
	 * Set if this socket is an accepted one.
	 */
	int listening_socket_id;
	
	/** Treshold size in bytes. */
	size_t treshold;
	/** Window size in bytes. */
	size_t window;
	/** Acknowledgement timeout. */
	suseconds_t timeout;
	/** Last acknowledged byte. */
	uint32_t acknowledged;
	/** Next incoming sequence number. */
	uint32_t next_incoming;
	/** Incoming FIN. */
	uint32_t fin_incoming;
	/** Next outgoing sequence number. */
	uint32_t next_outgoing;
	/** Last outgoing sequence number. */
	uint32_t last_outgoing;
	/** Outgoing FIN. */
	uint32_t fin_outgoing;
	
	/**
	 * Expected sequence number by the remote host.
	 * The sequence number the other host expects.
	 * The notification is sent only upon a packet reecival.
	 */
	uint32_t expected;
	
	/**
	 * Expected sequence number counter.
	 * Counts the number of received notifications for the same sequence
	 * number.
	 */
	int expected_count;
	
	/** Incoming packet queue.
	 *
	 * Packets are buffered until received in the right order.
	 * The packets are excluded after successfully read.
	 * Packets are sorted by their starting byte.
	 * Packets metric is set as their data length.
	 */
	packet_t *incoming;
	
	/** Outgoing packet queue.
	 *
	 * Packets are buffered until acknowledged by the remote host in the
	 * right order.
	 * The packets are excluded after acknowledged.
	 * Packets are sorted by their starting byte.
	 * Packets metric is set as their data length.
	 */
	packet_t *outgoing;
	
	/** IP pseudo header. */
	void *pseudo_header;
	/** IP pseudo header length. */
	size_t headerlen;
	/** Remote host address. */
	struct sockaddr *addr;
	/** Remote host address length. */
	socklen_t addrlen;
	/** Remote host port. */
	uint16_t dest_port;
	/** Parent local sockets. */
	socket_cores_t *local_sockets;
	
	/** Local sockets safety lock.
	 *
	 * May be locked for writing while holding the global lock for reading
	 * when changing the local sockets only.
	 * The global lock may be locked only before locking the local lock.
	 * The global lock may be locked more weakly than the local lock.
	 * The global lock may be released before releasing the local lock.
	 * @see tcp_globals:lock
	 */
	fibril_rwlock_t *local_lock;
	
	/** Pending operation data. */
	tcp_operation_t operation;
	
	/**
	 * Timeouts in a row counter.
	 * If TCP_MAX_TIMEOUTS is reached, the connection is lost.
	 */
	int timeout_count;
};

/** TCP global data. */
struct tcp_globals {
	/** Networking module session. */
	async_sess_t *net_sess;
	/** IP module session. */
	async_sess_t *ip_sess;
	/** ICMP module session. */
	async_sess_t *icmp_sess;
	/** Last used free port. */
	int last_used_port;
	/** Active sockets. */
	socket_ports_t sockets;
	/** Device packet dimensions. */
	packet_dimensions_t dimensions;
	
	/**
	 * Safety lock.
	 * Write lock is used only for adding or removing socket ports.
	 */
	fibril_rwlock_t lock;
};

#endif

/** @}
 */
