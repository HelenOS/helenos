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
 * Network interface layer interface implementation for remote modules.
 * @see nil_remote.h
 */

#include <ipc/loc.h>
#include <nil_remote.h>
#include <generic.h>
#include <net/device.h>
#include <net/packet.h>
#include <packet_client.h>
#include <ipc/nil.h>

/** Notify the network interface layer about the device state change.
 *
 * @param[in] sess      Network interface layer session.
 * @param[in] device_id Device identifier.
 * @param[in] state     New device state.
 *
 * @return EOK on success.
 * @return Other error codes as defined for each specific module
 *         device state function.
 *
 */
int nil_device_state_msg(async_sess_t *sess, nic_device_id_t device_id,
    sysarg_t state)
{
	return generic_device_state_msg_remote(sess, NET_NIL_DEVICE_STATE,
	    device_id, state, 0);
}

/** Pass the packet queue to the network interface layer.
 *
 * Process and redistribute the received packet queue to the registered
 * upper layers.
 *
 * @param[in] sess      Network interface layer session.
 * @param[in] device_id Source device identifier.
 * @param[in] packet    Received packet or the received packet queue.
 * @param[in] target    Target service. Ignored parameter.
 *
 * @return EOK on success.
 * @return Other error codes as defined for each specific module
 *         received function.
 *
 */
int nil_received_msg(async_sess_t *sess, nic_device_id_t device_id,
    void *data, size_t size)
{
	async_exch_t *exch = async_exchange_begin(sess);

	ipc_call_t answer;
	aid_t req = async_send_1(exch, NET_NIL_RECEIVED, (sysarg_t) device_id,
	    &answer);
	sysarg_t retval = async_data_write_start(exch, data, size);

	async_exchange_end(exch);

	if (retval != EOK) {
		async_wait_for(req, NULL);
		return retval;
	}

	async_wait_for(req, &retval);
	return retval;
}

/** Notify upper layers that device address has changed
 *
 */
int nil_addr_changed_msg(async_sess_t *sess, nic_device_id_t device_id,
    const nic_address_t *address)
{
	assert(sess);
	
	async_exch_t *exch = async_exchange_begin(sess);
	
	aid_t message_id = async_send_1(exch, NET_NIL_ADDR_CHANGED,
	    (sysarg_t) device_id, NULL);
	int rc = async_data_write_start(exch, address, sizeof (nic_address_t));
	
	async_exchange_end(exch);
	
	sysarg_t res;
    async_wait_for(message_id, &res);
	
    if (rc != EOK)
		return rc;
	
    return (int) res;
}

int nil_device_req(async_sess_t *sess, nic_device_id_t device_id,
    service_id_t sid, size_t mtu)
{
	async_exch_t *exch = async_exchange_begin(sess);
	int rc = async_req_3_0(exch, NET_NIL_DEVICE, (sysarg_t) device_id,
	    (sysarg_t) sid, (sysarg_t) mtu);
	async_exchange_end(exch);
	return rc;
}

/** @}
 */
