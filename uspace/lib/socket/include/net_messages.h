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

/** @addtogroup net
 *  @{
 */

/** @file
 *  Networking common message definitions.
 */

#ifndef __NET_MESSAGES_H__
#define __NET_MESSAGES_H__

#include <async.h>
#include <ipc/ipc.h>
#include <ipc/services.h>

#include <net_device.h>
#include <adt/measured_strings.h>
#include <net/packet.h>

/** @name Networking specific message arguments definitions
 */
/*@{*/

/** @name First arguments
 */
/*@{*/

/** Returns the device identifier message argument.
 *  @param[in] call The message call structure.
 */
#define IPC_GET_DEVICE(call) \
	({device_id_t device_id = (device_id_t) IPC_GET_ARG1(*call); device_id;})

/*@;})*/

/** @name Second arguments
 */
/*@({*/

/** Returns the packet identifier message argument.
 *  @param[in] call The message call structure.
 */
#define IPC_GET_PACKET(call) \
	({packet_id_t packet_id = (packet_id_t) IPC_GET_ARG2(*call); packet_id;})

/** Returns the count message argument.
 *  @param[in] call The message call structure.
 */
#define IPC_GET_COUNT(call) \
	({size_t size = (size_t) IPC_GET_ARG2(*call); size;})

/** Returns the device state message argument.
 *  @param[in] call The message call structure.
 */
#define IPC_GET_STATE(call) \
	({device_state_t device_state = (device_state_t) IPC_GET_ARG2(*call); device_state;})

/** Returns the maximum transmission unit message argument.
 *  @param[in] call The message call structure.
 */
#define IPC_GET_MTU(call) \
	({size_t size = (size_t) IPC_GET_ARG2(*call); size;})

/*@;})*/

/** @name Third arguments
 */
/*@({*/

/** Returns the device driver service message argument.
 *  @param[in] call The message call structure.
 */
 #define IPC_GET_SERVICE(call) \
	({services_t service = (services_t) IPC_GET_ARG3(*call); service;})

/** Returns the target service message argument.
 *  @param[in] call The message call structure.
 */
#define IPC_GET_TARGET(call) \
	({services_t service = (services_t) IPC_GET_ARG3(*call); service;})

/** Returns the sender service message argument.
 *  @param[in] call The message call structure.
 */
#define IPC_GET_SENDER(call) \
	({services_t service = (services_t) IPC_GET_ARG3(*call); service;})

/*@;})*/

/** @name Fourth arguments
 */
/*@({*/

/** Returns the error service message argument.
 *  @param[in] call The message call structure.
 */
#define IPC_GET_ERROR(call) \
	({services_t service = (services_t) IPC_GET_ARG4(*call); service;})

/*@;})*/

/** @name Fifth arguments
 */
/*@({*/

/** Returns the phone message argument.
 *  @param[in] call The message call structure.
 */
#define IPC_GET_PHONE(call) \
	({int phone = (int) IPC_GET_ARG5(*call); phone;})

/*@}*/

/** @name First answers
 */
/*@{*/

/** Sets the device identifier in the message answer.
 *  @param[out] answer The message answer structure.
 */
#define IPC_SET_DEVICE(answer, value) \
	{ipcarg_t argument = (ipcarg_t) (value); IPC_SET_ARG1(*answer, argument);}

/** Sets the minimum address length in the message answer.
 *  @param[out] answer The message answer structure.
 */
#define IPC_SET_ADDR(answer, value) \
	{ipcarg_t argument = (ipcarg_t) (value); IPC_SET_ARG1(*answer, argument);}

/*@}*/

/** @name Second answers
 */
/*@{*/

/** Sets the minimum prefix size in the message answer.
 *  @param[out] answer The message answer structure.
 */
#define IPC_SET_PREFIX(answer, value) \
	{ipcarg_t argument = (ipcarg_t) (value); IPC_SET_ARG2(*answer, argument);}

/*@}*/

/** @name Third answers
 */
/*@{*/

/** Sets the maximum content size in the message answer.
 *  @param[out] answer The message answer structure.
 */
#define IPC_SET_CONTENT(answer, value) \
	{ipcarg_t argument = (ipcarg_t) (value); IPC_SET_ARG3(*answer, argument);}

/*@}*/

/** @name Fourth answers
 */
/*@{*/

/** Sets the minimum suffix size in the message answer.
 *  @param[out] answer The message answer structure.
 */
#define IPC_SET_SUFFIX(answer, value) \
	{ipcarg_t argument = (ipcarg_t) (value); IPC_SET_ARG4(*answer, argument);}

/*@}*/

/*@}*/

/** Notify the module about the device state change.
 *
 * @param[in] phone     The service module phone.
 * @param[in] message   The service specific message.
 * @param[in] device_id The device identifier.
 * @param[in] state     The new device state.
 * @param[in] target    The target module service.
 *
 * @return EOK on success.
 *
 */
static inline int generic_device_state_msg_remote(int phone, int message,
    device_id_t device_id, int state, services_t target)
{
	async_msg_3(phone, (ipcarg_t) message, (ipcarg_t) device_id,
	    (ipcarg_t) state, target);
	
	return EOK;
}

/** Notify a module about the device.
 *
 * @param[in] phone     The service module phone.
 * @param[in] message   The service specific message.
 * @param[in] device_id The device identifier.
 * @param[in] arg2      The second argument of the message.
 * @param[in] service   The device module service.
 *
 * @return EOK on success.
 * @return Other error codes as defined for the specific service message.
 *
 */
static inline int generic_device_req_remote(int phone, int message,
    device_id_t device_id, int arg2, services_t service)
{
	return (int) async_req_3_0(phone, (ipcarg_t) message, (ipcarg_t) device_id,
	    (ipcarg_t) arg2, (ipcarg_t) service);
}

/** Returns the address.
 *  @param[in] phone The service module phone.
 *  @param[in] message The service specific message.
 *  @param[in] device_id The device identifier.
 *  @param[out] address The desired address.
 *  @param[out] data The address data container.
 *  @returns EOK on success.
 *  @returns EBADMEM if the address parameter and/or the data parameter is NULL.
 *  @returns Other error codes as defined for the specific service message.
 */
static inline int generic_get_addr_req(int phone, int message, device_id_t device_id, measured_string_ref * address, char ** data){
	aid_t message_id;
	ipcarg_t result;
	int string;

	if(!(address && data)){
		return EBADMEM;
	}

	// request the address
	message_id = async_send_1(phone, (ipcarg_t) message, (ipcarg_t) device_id, NULL);
	string = measured_strings_return(phone, address, data, 1);
	async_wait_for(message_id, &result);

	// if not successful
	if((string == EOK) && (result != EOK)){
		// clear the data
		free(*address);
		free(*data);
	}
	return (int) result;
}

/** Return the device packet dimension for sending.
 *
 * @param[in]  phone            The service module phone.
 * @param[in]  message          The service specific message.
 * @param[in]  device_id        The device identifier.
 * @param[out] packet_dimension The packet dimension.
 *
 * @return EOK on success.
 * @return EBADMEM if the packet_dimension parameter is NULL.
 * @return Other error codes as defined for the specific service message.
 *
 */
static inline int generic_packet_size_req_remote(int phone, int message,
    device_id_t device_id, packet_dimension_ref packet_dimension)
{
	if (!packet_dimension)
		return EBADMEM;
	
	ipcarg_t addr_len;
	ipcarg_t prefix;
	ipcarg_t content;
	ipcarg_t suffix;
	
	ipcarg_t result = async_req_1_4(phone, (ipcarg_t) message,
	    (ipcarg_t) device_id, &addr_len, &prefix, &content, &suffix);
	
	packet_dimension->prefix = (size_t) prefix;
	packet_dimension->content = (size_t) content;
	packet_dimension->suffix = (size_t) suffix;
	packet_dimension->addr_len = (size_t) addr_len;
	
	return (int) result;
}

/** Pass the packet queue to the module.
 *
 * @param[in] phone     The service module phone.
 * @param[in] message   The service specific message.
 * @param[in] device_id The device identifier.
 * @param[in] packet_id The received packet or the received packet queue
 *                      identifier.
 * @param[in] target    The target module service.
 * @param[in] error     The error module service.
 *
 * @return EOK on success.
 *
 */
static inline int generic_received_msg_remote(int phone, int message,
    device_id_t device_id, packet_id_t packet_id, services_t target,
    services_t error)
{
	if (error)
		async_msg_4(phone, (ipcarg_t) message, (ipcarg_t) device_id,
		    (ipcarg_t) packet_id, (ipcarg_t) target, (ipcarg_t) error);
	else
		async_msg_3(phone, (ipcarg_t) message, (ipcarg_t) device_id,
		    (ipcarg_t) packet_id, (ipcarg_t) target);
	
	return EOK;
}

/** Send the packet queue.
 *
 * @param[in] phone     The service module phone.
 * @param[in] message   The service specific message.
 * @param[in] device_id The device identifier.
 * @param[in] packet_id The packet or the packet queue identifier.
 * @param[in] sender    The sending module service.
 * @param[in] error     The error module service.
 *
 * @return EOK on success.
 *
 */
static inline int generic_send_msg_remote(int phone, int message,
    device_id_t device_id, packet_id_t packet_id, services_t sender,
    services_t error)
{
	if (error)
		async_msg_4(phone, (ipcarg_t) message, (ipcarg_t) device_id,
		    (ipcarg_t) packet_id, (ipcarg_t) sender, (ipcarg_t) error);
	else
		async_msg_3(phone, (ipcarg_t) message, (ipcarg_t) device_id,
		    (ipcarg_t) packet_id, (ipcarg_t) sender);
	
	return EOK;
}

/** Translates the given strings.
 *  Allocates and returns the needed memory block as the data parameter.
 *  @param[in] phone The service module phone.
 *  @param[in] message The service specific message.
 *  @param[in] device_id The device identifier.
 *  @param[in] service The module service.
 *  @param[in] configuration The key strings.
 *  @param[in] count The number of configuration keys.
 *  @param[out] translation The translated values.
 *  @param[out] data The translation data container.
 *  @returns EOK on success.
 *  @returns EINVAL if the configuration parameter is NULL.
 *  @returns EINVAL if the count parameter is zero (0).
 *  @returns EBADMEM if the translation or the data parameters are NULL.
 *  @returns Other error codes as defined for the specific service message.
 */
static inline int generic_translate_req(int phone, int message, device_id_t device_id, services_t service, measured_string_ref configuration, size_t count, measured_string_ref * translation, char ** data){
	aid_t message_id;
	ipcarg_t result;
	int string;

	if(!(configuration && (count > 0))){
		return EINVAL;
	}
	if(!(translation && data)){
		return EBADMEM;
	}

	// request the translation
	message_id = async_send_3(phone, (ipcarg_t) message, (ipcarg_t) device_id, (ipcarg_t) count, (ipcarg_t) service, NULL);
	measured_strings_send(phone, configuration, count);
	string = measured_strings_return(phone, translation, data, count);
	async_wait_for(message_id, &result);

	// if not successful
	if((string == EOK) && (result != EOK)){
		// clear the data
		free(*translation);
		free(*data);
	}

	return (int) result;
}

#endif

/** @}
 */
