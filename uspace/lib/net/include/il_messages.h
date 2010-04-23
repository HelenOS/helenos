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

/** @addtogroup net_il
 * @{
 */

/** @file
 * Internetwork layer modules messages.
 * @see il_interface.h
 * @see ip_interface.h
 */

#ifndef __NET_IL_MESSAGES_H__
#define __NET_IL_MESSAGES_H__

#include <ipc/ipc.h>

/** Internet layer modules messages.
 */
typedef enum {
	/** New device message.
	 *  @see ip_device_req()
	 */
	NET_IL_DEVICE = NET_IL_FIRST,
	/** Device state changed message.
	 *  @see il_device_state_msg()
	 */
	NET_IL_DEVICE_STATE,
	/** Device MTU changed message.
	 *  @see il_mtu_changed_msg()
	 */
	NET_IL_MTU_CHANGED,
	/** Packet size message.
	 *  @see il_packet_size_req()
	 */
	NET_IL_PACKET_SPACE,
	/** Packet received message.
	 *  @see il_received_msg()
	 */
	NET_IL_RECEIVED,
	/** Packet send message.
	 *  @see il_send_msg()
	 */
	NET_IL_SEND
} il_messages;

/** @name Internetwork layer specific message parameters definitions
 *
 */
/*@{*/

/** Return the protocol number message parameter.
 * @param[in] call The message call structure.
 *
 */
#define IL_GET_PROTO(call)  (int) IPC_GET_ARG1(*call)

/** Return the registering service message parameter.
 * @param[in] call The message call structure.
 *
 */
#define IL_GET_SERVICE(call)  (services_t) IPC_GET_ARG2(*call)

/*@}*/

#endif

/** @}
 */
