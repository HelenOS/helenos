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

/** @addtogroup libnet
 * @{
 */

/** @file
 * Internetwork layer module interface for the underlying network interface
 * layer.
 */

#include <il_remote.h>
#include <generic.h>
#include <packet_client.h>
#include <ipc/services.h>
#include <ipc/il.h>
#include <net/device.h>
#include <net/packet.h>

/** Notify the internetwork layer modules about the device state change.
 *
 * @param[in] sess      Internetwork layer module session.
 * @param[in] device_id Device identifier.
 * @param[in] state     New device state.
 * @param[in] target    Target internetwork module service to be
 *                      delivered to.
 *
 * @return EOK on success.
 *
 */
int il_device_state_msg(async_sess_t *sess, nic_device_id_t device_id,
    nic_device_state_t state, services_t target)
{
	return generic_device_state_msg_remote(sess, NET_IL_DEVICE_STATE,
	    device_id, state, target);
}

/** Notify the internetwork layer modules about the received packet/s.
 *
 * @param[in] sess      Internetwork layer module session.
 * @param[in] device_id Device identifier.
 * @param[in] packet    Received packet or the received packet queue.
 * @param[in] target    Target internetwork module service to be
 *                      delivered to.
 *
 * @return EOK on success.
 *
 */
int il_received_msg(async_sess_t *sess, nic_device_id_t device_id,
    packet_t *packet, services_t target)
{
	return generic_received_msg_remote(sess, NET_IL_RECEIVED, device_id,
	    packet_get_id(packet), target, 0);
}

/** Notify the internetwork layer modules about the mtu change.
 *
 * @param[in] sess      Internetwork layer module session.
 * @param[in] device_id Device identifier.
 * @param[in] mtu       New MTU value.
 * @param[in] target    Target internetwork module service to be
 *                      delivered to.
 *
 * @return EOK on success.
 *
 */
int il_mtu_changed_msg(async_sess_t *sess, nic_device_id_t device_id, size_t mtu,
    services_t target)
{
	return generic_device_state_msg_remote(sess, NET_IL_MTU_CHANGED,
	    device_id, mtu, target);
}

/** Notify IL layer modules about address change (implemented by ARP)
 *
 */
int il_addr_changed_msg(async_sess_t *sess, nic_device_id_t device_id,
    size_t addr_len, const uint8_t *address)
{
	async_exch_t *exch = async_exchange_begin(sess);
	
	aid_t message_id = async_send_1(exch, NET_IL_ADDR_CHANGED,
			(sysarg_t) device_id, NULL);
	int rc = async_data_write_start(exch, address, addr_len);
	
	async_exchange_end(exch);
	
	sysarg_t res;
    async_wait_for(message_id, &res);
    if (rc != EOK)
		return rc;
	
    return (int) res;
}

/** @}
 */
