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

/** @addtogroup libc
 * @{
 */

/** @file
 * Network interface layer module messages.
 */

#ifndef LIBC_NIL_MESSAGES_H_
#define LIBC_NIL_MESSAGES_H_

#include <ipc/net.h>

/** Network interface layer module messages. */
typedef enum {
	/** New device or update MTU message.
	 * @see nil_device_req()
	 */
	NET_NIL_DEVICE = NET_NIL_FIRST,
	/** New device state message.
	 * @see nil_device_state_msg()
	 */
	NET_NIL_DEVICE_STATE,
	/** Received packet queue message.
	 * @see nil_received_msg()
	 */
	NET_NIL_RECEIVED,
	/** Send packet queue message.
	 * @see nil_send_msg()
	 */
	NET_NIL_SEND,
	/** Packet size message.
	 * @see nil_packet_size_req()
	 */
	NET_NIL_PACKET_SPACE,
	/** Device local hardware address message.
	 * @see nil_get_addr()
	 */
	NET_NIL_ADDR,
	/** Device broadcast hardware address message.
	 * @see nil_get_broadcast_addr()
	 */
	NET_NIL_BROADCAST_ADDR,
	/** Device has changed address
	 * @see nil_addr_changed_msg()
	 */
	NET_NIL_ADDR_CHANGED
} nil_messages;

/** @name Network interface layer specific message parameters definitions */
/*@{*/

/** Return the protocol service message parameter. */
#define NIL_GET_PROTO(call)  ((services_t) IPC_GET_ARG2(call))

/*@}*/

#endif

/** @}
 */
