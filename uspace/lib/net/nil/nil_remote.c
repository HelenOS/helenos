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

/** @addtogroup net_nil
 * @{
 */

/** @file
 * Network interface layer interface implementation for remote modules.
 * @see nil_interface.h
 */

#include <net_messages.h>
#include <net_device.h>
#include <nil_interface.h>
#include <packet/packet.h>
#include <packet/packet_client.h>
#include <nil_messages.h>
#include <nil_remote.h>

/** Notify the network interface layer about the device state change.
 *
 * @param[in] nil_phone The network interface layer phone.
 * @param[in] device_id The device identifier.
 * @param[in] state     The new device state.
 *
 * @return EOK on success.
 * @return Other error codes as defined for each specific module device
 *         state function.
 *
 */
int nil_device_state_msg_remote(int nil_phone, device_id_t device_id, int state)
{
	return generic_device_state_msg_remote(nil_phone, NET_NIL_DEVICE_STATE,
	    device_id, state, 0);
}

/** Pass the packet queue to the network interface layer.
 *
 * Process and redistribute the received packet queue to the registered
 * upper layers.
 *
 * @param[in] nil_phone The network interface layer phone.
 * @param[in] device_id The source device identifier.
 * @param[in] packet    The received packet or the received packet queue.
 * @param     target    The target service. Ignored parameter.
 *
 * @return EOK on success.
 * @return Other error codes as defined for each specific module
 *         received function.
 *
 */
int nil_received_msg_remote(int nil_phone, device_id_t device_id,
    packet_t packet, services_t target)
{
	return generic_received_msg_remote(nil_phone, NET_NIL_RECEIVED, device_id,
	    packet_get_id(packet), target, 0);
}

/** @}
 */
