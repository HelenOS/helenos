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
#include <async_obsolete.h>
#include <ipc/services.h>
#include <net/device.h>
#include <adt/measured_strings.h>
#include <net/packet.h>

/** Notify the module about the device state change.
 *
 * @param[in] phone	The service module phone.
 * @param[in] message	The service specific message.
 * @param[in] device_id	The device identifier.
 * @param[in] state	The new device state.
 * @param[in] target	The target module service.
 * @return		EOK on success.
 *
 */
int
generic_device_state_msg_remote(int phone, int message, device_id_t device_id,
    int state, services_t target)
{
	async_obsolete_msg_3(phone, (sysarg_t) message, (sysarg_t) device_id,
	    (sysarg_t) state, target);
	
	return EOK;
}

/** Notify a module about the device.
 *
 * @param[in] phone	The service module phone.
 * @param[in] message	The service specific message.
 * @param[in] device_id	The device identifier.
 * @param[in] arg2	The second argument of the message.
 * @param[in] service	The device module service.
 * @return		EOK on success.
 * @return		Other error codes as defined for the specific service
 *			 message.
 *
 */
int
generic_device_req_remote(int phone, int message, device_id_t device_id,
    int arg2, services_t service)
{
	return (int) async_obsolete_req_3_0(phone, (sysarg_t) message,
	    (sysarg_t) device_id, (sysarg_t) arg2, (sysarg_t) service);
}

/** Returns the address.
 *
 * @param[in] phone	The service module phone.
 * @param[in] message	The service specific message.
 * @param[in] device_id	The device identifier.
 * @param[out] address	The desired address.
 * @param[out] data	The address data container.
 * @return		EOK on success.
 * @return		EBADMEM if the address parameter and/or the data
 *			parameter is NULL.
 * @return		Other error codes as defined for the specific service
 *			message.
 */
int
generic_get_addr_req(int phone, int message, device_id_t device_id,
    measured_string_t **address, uint8_t **data)
{
	aid_t message_id;
	sysarg_t result;
	int string;

	if (!address || !data)
		return EBADMEM;

	/* Request the address */
	message_id = async_obsolete_send_1(phone, (sysarg_t) message,
	    (sysarg_t) device_id, NULL);
	string = measured_strings_return(phone, address, data, 1);
	async_wait_for(message_id, &result);

	/* If not successful */
	if ((string == EOK) && (result != EOK)) {
		/* Clear the data */
		free(*address);
		free(*data);
	}
	
	return (int) result;
}

/** Return the device packet dimension for sending.
 *
 * @param[in] phone	The service module phone.
 * @param[in] message	The service specific message.
 * @param[in] device_id	The device identifier.
 * @param[out] packet_dimension The packet dimension.
 * @return		EOK on success.
 * @return		EBADMEM if the packet_dimension parameter is NULL.
 * @return		Other error codes as defined for the specific service
 *			message.
 */
int
generic_packet_size_req_remote(int phone, int message, device_id_t device_id,
    packet_dimension_t *packet_dimension)
{
	if (!packet_dimension)
		return EBADMEM;
	
	sysarg_t addr_len;
	sysarg_t prefix;
	sysarg_t content;
	sysarg_t suffix;
	
	sysarg_t result = async_obsolete_req_1_4(phone, (sysarg_t) message,
	    (sysarg_t) device_id, &addr_len, &prefix, &content, &suffix);
	
	packet_dimension->prefix = (size_t) prefix;
	packet_dimension->content = (size_t) content;
	packet_dimension->suffix = (size_t) suffix;
	packet_dimension->addr_len = (size_t) addr_len;
	
	return (int) result;
}

/** Pass the packet queue to the module.
 *
 * @param[in] phone	The service module phone.
 * @param[in] message	The service specific message.
 * @param[in] device_id	The device identifier.
 * @param[in] packet_id	The received packet or the received packet queue
 *			identifier.
 * @param[in] target	The target module service.
 * @param[in] error	The error module service.
 * @return		EOK on success.
 */
int
generic_received_msg_remote(int phone, int message, device_id_t device_id,
    packet_id_t packet_id, services_t target, services_t error)
{
	if (error) {
		async_obsolete_msg_4(phone, (sysarg_t) message, (sysarg_t) device_id,
		    (sysarg_t) packet_id, (sysarg_t) target, (sysarg_t) error);
	} else {
		async_obsolete_msg_3(phone, (sysarg_t) message, (sysarg_t) device_id,
		    (sysarg_t) packet_id, (sysarg_t) target);
	}
	
	return EOK;
}

/** Send the packet queue.
 *
 * @param[in] phone	The service module phone.
 * @param[in] message	The service specific message.
 * @param[in] device_id	The device identifier.
 * @param[in] packet_id	The packet or the packet queue identifier.
 * @param[in] sender	The sending module service.
 * @param[in] error	The error module service.
 * @return		EOK on success.
 *
 */
int
generic_send_msg_remote(int phone, int message, device_id_t device_id,
    packet_id_t packet_id, services_t sender, services_t error)
{
	if (error) {
		async_obsolete_msg_4(phone, (sysarg_t) message, (sysarg_t) device_id,
		    (sysarg_t) packet_id, (sysarg_t) sender, (sysarg_t) error);
	} else {
		async_obsolete_msg_3(phone, (sysarg_t) message, (sysarg_t) device_id,
		    (sysarg_t) packet_id, (sysarg_t) sender);
	}
	
	return EOK;
}

/** Translates the given strings.
 *
 * Allocates and returns the needed memory block as the data parameter.
 *
 * @param[in] phone	The service module phone.
 * @param[in] message	The service specific message.
 * @param[in] device_id	The device identifier.
 * @param[in] service	The module service.
 * @param[in] configuration The key strings.
 * @param[in] count	The number of configuration keys.
 * @param[out] translation The translated values.
 * @param[out] data	The translation data container.
 * @return		EOK on success.
 * @return		EINVAL if the configuration parameter is NULL.
 * @return		EINVAL if the count parameter is zero.
 * @return		EBADMEM if the translation or the data parameters are
 *			NULL.
 * @return		Other error codes as defined for the specific service
 *			message.
 */
int
generic_translate_req(int phone, int message, device_id_t device_id,
    services_t service, measured_string_t *configuration, size_t count,
    measured_string_t **translation, uint8_t **data)
{
	aid_t message_id;
	sysarg_t result;
	int string;

	if (!configuration || (count == 0))
		return EINVAL;
	if (!translation || !data)
		return EBADMEM;

	/* Request the translation */
	message_id = async_obsolete_send_3(phone, (sysarg_t) message,
	    (sysarg_t) device_id, (sysarg_t) count, (sysarg_t) service, NULL);
	measured_strings_send(phone, configuration, count);
	string = measured_strings_return(phone, translation, data, count);
	async_wait_for(message_id, &result);

	/* If not successful */
	if ((string == EOK) && (result != EOK)) {
		/* Clear the data */
		free(*translation);
		free(*data);
	}

	return (int) result;
}

/** @}
 */
