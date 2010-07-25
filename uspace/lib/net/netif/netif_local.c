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

/** @addtogroup netif
 * @{
 */

/** @file
 * Network interface module skeleton implementation.
 * @see netif.h
 */

#include <async.h>
#include <mem.h>
#include <fibril_synch.h>
#include <stdio.h>
#include <ipc/ipc.h>
#include <ipc/services.h>

#include <net_err.h>
#include <net_messages.h>
#include <net_modules.h>
#include <packet/packet.h>
#include <packet/packet_client.h>
#include <packet/packet_server.h>
#include <packet_remote.h>
#include <adt/measured_strings.h>
#include <net_device.h>
#include <nil_interface.h>
#include <netif_local.h>
#include <netif_messages.h>
#include <netif_interface.h>

DEVICE_MAP_IMPLEMENT(netif_device_map, netif_device_t);

/** Network interface global data.
 */
netif_globals_t netif_globals;

/** Probe the existence of the device.
 *
 * @param[in] netif_phone The network interface phone.
 * @param[in] device_id   The device identifier.
 * @param[in] irq         The device interrupt number.
 * @param[in] io          The device input/output address.
 *
 * @return EOK on success.
 * @return Other errro codes as defined for the netif_probe_message().
 *
 */
int netif_probe_req_local(int netif_phone, device_id_t device_id, int irq, int io)
{
	fibril_rwlock_write_lock(&netif_globals.lock);
	int result = netif_probe_message(device_id, irq, io);
	fibril_rwlock_write_unlock(&netif_globals.lock);
	
	return result;
}

/** Send the packet queue.
 *
 * @param[in] netif_phone The network interface phone.
 * @param[in] device_id   The device identifier.
 * @param[in] packet      The packet queue.
 * @param[in] sender      The sending module service.
 *
 * @return EOK on success.
 * @return Other error codes as defined for the generic_send_msg() function.
 *
 */
int netif_send_msg_local(int netif_phone, device_id_t device_id,
    packet_t packet, services_t sender)
{
	fibril_rwlock_write_lock(&netif_globals.lock);
	int result = netif_send_message(device_id, packet, sender);
	fibril_rwlock_write_unlock(&netif_globals.lock);
	
	return result;
}

/** Start the device.
 *
 * @param[in] netif_phone The network interface phone.
 * @param[in] device_id   The device identifier.
 *
 * @return EOK on success.
 * @return Other error codes as defined for the find_device() function.
 * @return Other error codes as defined for the netif_start_message() function.
 *
 */
int netif_start_req_local(int netif_phone, device_id_t device_id)
{
	ERROR_DECLARE;
	
	fibril_rwlock_write_lock(&netif_globals.lock);
	
	netif_device_t *device;
	if (ERROR_OCCURRED(find_device(device_id, &device))) {
		fibril_rwlock_write_unlock(&netif_globals.lock);
		return ERROR_CODE;
	}
	
	int result = netif_start_message(device);
	if (result > NETIF_NULL) {
		int phone = device->nil_phone;
		fibril_rwlock_write_unlock(&netif_globals.lock);
		nil_device_state_msg(phone, device_id, result);
		return EOK;
	}
	
	fibril_rwlock_write_unlock(&netif_globals.lock);
	
	return result;
}

/** Stop the device.
 *
 * @param[in] netif_phone The network interface phone.
 * @param[in] device_id   The device identifier.
 *
 * @return EOK on success.
 * @return Other error codes as defined for the find_device() function.
 * @return Other error codes as defined for the netif_stop_message() function.
 *
 */
int netif_stop_req_local(int netif_phone, device_id_t device_id)
{
	ERROR_DECLARE;
	
	fibril_rwlock_write_lock(&netif_globals.lock);
	
	netif_device_t *device;
	if (ERROR_OCCURRED(find_device(device_id, &device))) {
		fibril_rwlock_write_unlock(&netif_globals.lock);
		return ERROR_CODE;
	}
	
	int result = netif_stop_message(device);
	if (result > NETIF_NULL) {
		int phone = device->nil_phone;
		fibril_rwlock_write_unlock(&netif_globals.lock);
		nil_device_state_msg(phone, device_id, result);
		return EOK;
	}
	
	fibril_rwlock_write_unlock(&netif_globals.lock);
	
	return result;
}

/** Return the device usage statistics.
 *
 * @param[in]  netif_phone The network interface phone.
 * @param[in]  device_id   The device identifier.
 * @param[out] stats       The device usage statistics.
 *
 * @return EOK on success.
 *
 */
int netif_stats_req_local(int netif_phone, device_id_t device_id,
    device_stats_ref stats)
{
	fibril_rwlock_read_lock(&netif_globals.lock);
	int res = netif_get_device_stats(device_id, stats);
	fibril_rwlock_read_unlock(&netif_globals.lock);
	
	return res;
}

/** Return the device local hardware address.
 *
 * @param[in]  netif_phone The network interface phone.
 * @param[in]  device_id   The device identifier.
 * @param[out] address     The device local hardware address.
 * @param[out] data        The address data.
 *
 * @return EOK on success.
 * @return EBADMEM if the address parameter is NULL.
 * @return ENOENT if there no such device.
 * @return Other error codes as defined for the netif_get_addr_message()
 *         function.
 *
 */
int netif_get_addr_req_local(int netif_phone, device_id_t device_id,
    measured_string_ref *address, char **data)
{
	ERROR_DECLARE;
	
	if ((!address) || (!data))
		return EBADMEM;
	
	fibril_rwlock_read_lock(&netif_globals.lock);
	
	measured_string_t translation;
	if (!ERROR_OCCURRED(netif_get_addr_message(device_id, &translation))) {
		*address = measured_string_copy(&translation);
		ERROR_CODE = (*address) ? EOK : ENOMEM;
	}
	
	fibril_rwlock_read_unlock(&netif_globals.lock);
	
	*data = (**address).value;
	
	return ERROR_CODE;
}

/** Create bidirectional connection with the network interface module and registers the message receiver.
 *
 * @param[in] service   The network interface module service.
 * @param[in] device_id The device identifier.
 * @param[in] me        The requesting module service.
 * @param[in] receiver  The message receiver.
 *
 * @return The phone of the needed service.
 * @return EOK on success.
 * @return Other error codes as defined for the bind_service() function.
 *
 */
int netif_bind_service_local(services_t service, device_id_t device_id,
    services_t me, async_client_conn_t receiver)
{
	return EOK;
}

/** Find the device specific data.
 *
 * @param[in]  device_id The device identifier.
 * @param[out] device    The device specific data.
 *
 * @return EOK on success.
 * @return ENOENT if device is not found.
 * @return EPERM if the device is not initialized.
 *
 */
int find_device(device_id_t device_id, netif_device_t **device)
{
	if (!device)
		return EBADMEM;
	
	*device = netif_device_map_find(&netif_globals.device_map, device_id);
	if (*device == NULL)
		return ENOENT;
	
	if ((*device)->state == NETIF_NULL)
		return EPERM;
	
	return EOK;
}

/** Clear the usage statistics.
 *
 * @param[in] stats The usage statistics.
 *
 */
void null_device_stats(device_stats_ref stats)
{
	bzero(stats, sizeof(device_stats_t));
}

/** Initialize the netif module.
 *
 *  @param[in] client_connection The client connection functio to be
 *                               registered.
 *
 *  @return EOK on success.
 *  @return Other error codes as defined for each specific module
 *          message function.
 *
 */
int netif_init_module(async_client_conn_t client_connection)
{
	ERROR_DECLARE;
	
	async_set_client_connection(client_connection);
	
	netif_globals.net_phone = connect_to_service(SERVICE_NETWORKING);
	netif_device_map_initialize(&netif_globals.device_map);
	
	ERROR_PROPAGATE(pm_init());
	
	fibril_rwlock_initialize(&netif_globals.lock);
	if (ERROR_OCCURRED(netif_initialize())) {
		pm_destroy();
		return ERROR_CODE;
	}
	
	return EOK;
}

/** Release the given packet.
 *
 * Prepared for future optimization.
 *
 * @param[in] packet_id The packet identifier.
 *
 */
void netif_pq_release(packet_id_t packet_id)
{
	pq_release_remote(netif_globals.net_phone, packet_id);
}

/** Allocate new packet to handle the given content size.
 *
 * @param[in] content The minimum content size.
 *
 * @return The allocated packet.
 * @return NULL if there is an error.
 *
 */
packet_t netif_packet_get_1(size_t content)
{
	return packet_get_1_remote(netif_globals.net_phone, content);
}

/** Register the device notification receiver, the network interface layer module.
 *
 * @param[in] name      Module name.
 * @param[in] device_id The device identifier.
 * @param[in] phone     The network interface layer module phone.
 *
 * @return EOK on success.
 * @return ENOENT if there is no such device.
 * @return ELIMIT if there is another module registered.
 *
 */
static int register_message(const char *name, device_id_t device_id, int phone)
{
	ERROR_DECLARE;
	
	netif_device_t *device;
	ERROR_PROPAGATE(find_device(device_id, &device));
	if(device->nil_phone > 0)
		return ELIMIT;
	
	device->nil_phone = phone;
	printf("%s: Receiver of device %d registered (phone: %d)\n",
	    name, device->device_id, device->nil_phone);
	return EOK;
}

/** Process the netif module messages.
 *
 * @param[in]  name         Module name.
 * @param[in]  callid       The message identifier.
 * @param[in]  call         The message parameters.
 * @param[out] answer       The message answer parameters.
 * @param[out] answer_count The last parameter for the actual answer
 *                          in the answer parameter.
 *
 * @return EOK on success.
 * @return ENOTSUP if the message is not known.
 * @return Other error codes as defined for each specific module message function.
 *
 * @see IS_NET_NETIF_MESSAGE()
 *
 */
int netif_module_message_standalone(const char *name, ipc_callid_t callid,
    ipc_call_t *call, ipc_call_t *answer, int *answer_count)
{
	ERROR_DECLARE;
	
	size_t length;
	device_stats_t stats;
	packet_t packet;
	measured_string_t address;
	
	*answer_count = 0;
	switch (IPC_GET_METHOD(*call)) {
		case IPC_M_PHONE_HUNGUP:
			return EOK;
		case NET_NETIF_PROBE:
			return netif_probe_req_local(0, IPC_GET_DEVICE(call),
			    NETIF_GET_IRQ(call), NETIF_GET_IO(call));
		case IPC_M_CONNECT_TO_ME:
			fibril_rwlock_write_lock(&netif_globals.lock);
			ERROR_CODE = register_message(name, IPC_GET_DEVICE(call),
			    IPC_GET_PHONE(call));
			fibril_rwlock_write_unlock(&netif_globals.lock);
			return ERROR_CODE;
		case NET_NETIF_SEND:
			ERROR_PROPAGATE(packet_translate_remote(netif_globals.net_phone,
			    &packet, IPC_GET_PACKET(call)));
			return netif_send_msg_local(0, IPC_GET_DEVICE(call), packet,
			    IPC_GET_SENDER(call));
		case NET_NETIF_START:
			return netif_start_req_local(0, IPC_GET_DEVICE(call));
		case NET_NETIF_STATS:
			fibril_rwlock_read_lock(&netif_globals.lock);
			if (!ERROR_OCCURRED(async_data_read_receive(&callid, &length))) {
				if (length < sizeof(device_stats_t))
					ERROR_CODE = EOVERFLOW;
				else {
					if (!ERROR_OCCURRED(netif_get_device_stats(
					    IPC_GET_DEVICE(call), &stats)))
						ERROR_CODE = async_data_read_finalize(callid, &stats,
						    sizeof(device_stats_t));
				}
			}
			fibril_rwlock_read_unlock(&netif_globals.lock);
			return ERROR_CODE;
		case NET_NETIF_STOP:
			return netif_stop_req_local(0, IPC_GET_DEVICE(call));
		case NET_NETIF_GET_ADDR:
			fibril_rwlock_read_lock(&netif_globals.lock);
			if (!ERROR_OCCURRED(netif_get_addr_message(IPC_GET_DEVICE(call),
			    &address)))
				ERROR_CODE = measured_strings_reply(&address, 1);
			fibril_rwlock_read_unlock(&netif_globals.lock);
			return ERROR_CODE;
	}
	
	return netif_specific_message(callid, call, answer, answer_count);
}

/** Start the network interface module.
 *
 * Initialize the client connection serving function, initialize
 * the module, registers the module service and start the async
 * manager, processing IPC messages in an infinite loop.
 *
 * @param[in] client_connection The client connection processing
 *                              function. The module skeleton propagates
 *                              its own one.
 *
 * @return EOK on success.
 * @return Other error codes as defined for each specific module message
 *         function.
 *
 */
int netif_module_start_standalone(async_client_conn_t client_connection)
{
	ERROR_DECLARE;
	
	ERROR_PROPAGATE(netif_init_module(client_connection));
	
	async_manager();
	
	pm_destroy();
	return EOK;
}

/** @}
 */
