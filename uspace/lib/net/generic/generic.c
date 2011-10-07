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
 * Generic communication interfaces for networking.
 */

#include <generic.h>
#include <async.h>
#include <ipc/services.h>
#include <net/device.h>
#include <adt/measured_strings.h>
#include <net/packet.h>

/** Notify the module about the device state change.
 *
 * @param[in] sess      Service module session.
 * @param[in] message   Service specific message.
 * @param[in] device_id Device identifier.
 * @param[in] state     New device state.
 * @param[in] target    Target module service.
 *
 * @return EOK on success.
 *
 */
int generic_device_state_msg_remote(async_sess_t *sess, sysarg_t message,
    nic_device_id_t device_id, sysarg_t state, services_t target)
{
	async_exch_t *exch = async_exchange_begin(sess);
	async_msg_3(exch, message, (sysarg_t) device_id, state, target);
	async_exchange_end(exch);
	
	return EOK;
}

/** Notify a module about the device.
 *
 * @param[in] sess      Service module session.
 * @param[in] message   Service specific message.
 * @param[in] device_id Device identifier.
 * @param[in] service   Device module service.
 *
 * @return EOK on success.
 * @return Other error codes as defined for the specific service
 *         message.
 *
 */
int generic_device_req_remote(async_sess_t *sess, sysarg_t message,
    nic_device_id_t device_id, services_t service)
{
	async_exch_t *exch = async_exchange_begin(sess);
	int rc = async_req_2_0(exch, message, (sysarg_t) device_id,
	    (sysarg_t) service);
	async_exchange_end(exch);
	
	return rc;
}

/** Returns the address.
 *
 * @param[in] sess      Service module session.
 * @param[in] message   Service specific message.
 * @param[in] device_id Device identifier.
 * @param[out] address  Desired address.
 * @param[out] data     Address data container.
 *
 * @return EOK on success.
 * @return EBADMEM if the address parameter and/or the data
 *         parameter is NULL.
 * @return Other error codes as defined for the specific service
 *         message.
 *
 */
int generic_get_addr_req(async_sess_t *sess, sysarg_t message,
    nic_device_id_t device_id, uint8_t *address, size_t max_len)
{
	if (!address)
		return EBADMEM;
	
	/* Request the address */
	async_exch_t *exch = async_exchange_begin(sess);
	aid_t aid = async_send_1(exch, message, (sysarg_t) device_id,
	    NULL);
	int rc = async_data_read_start(exch, address, max_len);
	async_exchange_end(exch);
	
	sysarg_t result;
	async_wait_for(aid, &result);
	
	if (rc != EOK)
		return rc;
	
	return (int) result;
}

/** Return the device packet dimension for sending.
 *
 * @param[in] sess              Service module session.
 * @param[in] message           Service specific message.
 * @param[in] device_id         Device identifier.
 * @param[out] packet_dimension Packet dimension.
 *
 * @return EOK on success.
 * @return EBADMEM if the packet_dimension parameter is NULL.
 * @return Other error codes as defined for the specific service
 *         message.
 *
 */
int generic_packet_size_req_remote(async_sess_t *sess, sysarg_t message,
    nic_device_id_t device_id, packet_dimension_t *packet_dimension)
{
	if (!packet_dimension)
		return EBADMEM;
	
	sysarg_t addr_len;
	sysarg_t prefix;
	sysarg_t content;
	sysarg_t suffix;
	
	async_exch_t *exch = async_exchange_begin(sess);
	sysarg_t result = async_req_1_4(exch, message, (sysarg_t) device_id,
	    &addr_len, &prefix, &content, &suffix);
	async_exchange_end(exch);
	
	packet_dimension->prefix = (size_t) prefix;
	packet_dimension->content = (size_t) content;
	packet_dimension->suffix = (size_t) suffix;
	packet_dimension->addr_len = (size_t) addr_len;
	
	return (int) result;
}

/** Pass the packet queue to the module.
 *
 * @param[in] sess      Service module session.
 * @param[in] message   Service specific message.
 * @param[in] device_id Device identifier.
 * @param[in] packet_id Received packet or the received packet queue
 *                      identifier.
 * @param[in] target    Target module service.
 * @param[in] error     Error module service.
 *
 * @return EOK on success.
 *
 */
int generic_received_msg_remote(async_sess_t *sess, sysarg_t message,
    nic_device_id_t device_id, packet_id_t packet_id, services_t target,
    services_t error)
{
	async_exch_t *exch = async_exchange_begin(sess);
	
	if (error) {
		async_msg_4(exch, message, (sysarg_t) device_id,
		    (sysarg_t) packet_id, (sysarg_t) target, (sysarg_t) error);
	} else {
		async_msg_3(exch, message, (sysarg_t) device_id,
		    (sysarg_t) packet_id, (sysarg_t) target);
	}
	
	async_exchange_end(exch);
	
	return EOK;
}

/** Send the packet queue.
 *
 * @param[in] sess      Service module session.
 * @param[in] message   Service specific message.
 * @param[in] device_id Device identifier.
 * @param[in] packet_id Packet or the packet queue identifier.
 * @param[in] sender    Sending module service.
 * @param[in] error     Error module service.
 *
 * @return EOK on success.
 *
 */
int generic_send_msg_remote(async_sess_t *sess, sysarg_t message,
    nic_device_id_t device_id, packet_id_t packet_id, services_t sender,
    services_t error)
{
	async_exch_t *exch = async_exchange_begin(sess);
	
	if (error) {
		async_msg_4(exch, message, (sysarg_t) device_id,
		    (sysarg_t) packet_id, (sysarg_t) sender, (sysarg_t) error);
	} else {
		async_msg_3(exch, message, (sysarg_t) device_id,
		    (sysarg_t) packet_id, (sysarg_t) sender);
	}
	
	async_exchange_end(exch);
	
	return EOK;
}

/** Translates the given strings.
 *
 * Allocates and returns the needed memory block as the data parameter.
 *
 * @param[in] sess          Service module session.
 * @param[in] message       Service specific message.
 * @param[in] device_id     Device identifier.
 * @param[in] service       Module service.
 * @param[in] configuration Key strings.
 * @param[in] count         Number of configuration keys.
 * @param[out] translation  Translated values.
 * @param[out] data         Translation data container.
 *
 * @return EOK on success.
 * @return EINVAL if the configuration parameter is NULL.
 * @return EINVAL if the count parameter is zero.
 * @return EBADMEM if the translation or the data parameters are
 *         NULL.
 * @return Other error codes as defined for the specific service
 *         message.
 *
 */
int generic_translate_req(async_sess_t *sess, sysarg_t message,
    nic_device_id_t device_id, services_t service,
    measured_string_t *configuration, size_t count,
    measured_string_t **translation, uint8_t **data)
{
	if ((!configuration) || (count == 0))
		return EINVAL;
	
	if ((!translation) || (!data))
		return EBADMEM;
	
	/* Request the translation */
	async_exch_t *exch = async_exchange_begin(sess);
	aid_t message_id = async_send_3(exch, message, (sysarg_t) device_id,
	    (sysarg_t) count, (sysarg_t) service, NULL);
	measured_strings_send(exch, configuration, count);
	int rc = measured_strings_return(exch, translation, data, count);
	async_exchange_end(exch);
	
	sysarg_t result;
	async_wait_for(message_id, &result);
	
	/* If not successful */
	if ((rc == EOK) && (result != EOK)) {
		/* Clear the data */
		free(*translation);
		free(*data);
	}
	
	return (int) result;
}

/** @}
 */
