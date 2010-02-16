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
 *  @{
 */

/** @file
 *  Internetwork layer module interface for the underlying network interface layer.
 *  This interface is always called by the standalone remote modules.
 */

#ifndef __NET_IL_INTERFACE_H__
#define __NET_IL_INTERFACE_H__

#include <async.h>

#include <ipc/services.h>

#include "../messages.h"

#include "../include/device.h"

#include "../structures/packet/packet.h"
#include "../structures/packet/packet_client.h"

#include "../il/il_messages.h"

/** @name Internetwork layer module interface
 *  This interface is used by other modules.
 */
/*@{*/

/** Notifies the internetwork layer modules about the device state change.
 *  @param[in] il_phone The internetwork layer module phone used for (semi)remote calls.
 *  @param[in] device_id The device identifier.
 *  @param[in] state The new device state.
 *  @param[in] target The target internetwork module service to be delivered to.
 *  @returns EOK on success.
 */
static inline int	il_device_state_msg( int il_phone, device_id_t device_id, device_state_t state, services_t target ){
	return generic_device_state_msg( il_phone, NET_IL_DEVICE_STATE, device_id, state, target );
}

/** Notifies the internetwork layer modules about the received packet/s.
 *  @param[in] il_phone The internetwork layer module phone used for (semi)remote calls.
 *  @param[in] device_id The device identifier.
 *  @param[in] packet The received packet or the received packet queue.
 *  @param[in] target The target internetwork module service to be delivered to.
 *  @returns EOK on success.
 */
inline static int	il_received_msg( int il_phone, device_id_t device_id, packet_t packet, services_t target ){
	return generic_received_msg( il_phone, NET_IL_RECEIVED, device_id, packet_get_id( packet ), target, 0 );
}

/** Notifies the internetwork layer modules about the mtu change.
 *  @param[in] il_phone The internetwork layer module phone used for (semi)remote calls.
 *  @param[in] device_id The device identifier.
 *  @param[in] mtu The new mtu value.
 *  @param[in] target The target internetwork module service to be delivered to.
 *  @returns EOK on success.
 */
inline static int	il_mtu_changed_msg( int il_phone, device_id_t device_id, size_t mtu, services_t target ){
	return generic_device_state_msg( il_phone, NET_IL_MTU_CHANGED, device_id, ( int ) mtu, target );
}

/*@}*/

#endif

/** @}
 */
