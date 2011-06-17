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
 * Network interface module skeleton implementation.
 * @see netif_skel.h
 */

#include <async.h>
#include <mem.h>
#include <fibril_synch.h>
#include <stdio.h>
#include <ipc/services.h>
#include <ipc/netif.h>
#include <errno.h>

#include <generic.h>
#include <net/modules.h>
#include <net/packet.h>
#include <packet_client.h>
#include <packet_remote.h>
#include <adt/measured_strings.h>
#include <net/device.h>
#include <netif_skel.h>
#include <nil_remote.h>

// FIXME: remove this header
#include <kernel/ipc/ipc_methods.h>

DEVICE_MAP_IMPLEMENT(netif_device_map, netif_device_t);

/** Network interface global data. */
netif_globals_t netif_globals;

/** Probe the existence of the device.
 *
 * @param[in] netif_phone Network interface phone.
 * @param[in] device_id   Device identifier.
 * @param[in] irq         Device interrupt number.
 * @param[in] io          Device input/output address.
 *
 * @return EOK on success.
 * @return Other error codes as defined for the
 *         netif_probe_message().
 *
 */
static int netif_probe_req_local(int netif_phone, device_id_t device_id,
    int irq, void *io)
{
	fibril_rwlock_write_lock(&netif_globals.lock);
	int result = netif_probe_message(device_id, irq, io);
	fibril_rwlock_write_unlock(&netif_globals.lock);
	
	return result;
}

/** Send the packet queue.
 *
 * @param[in] netif_phone Network interface phone.
 * @param[in] device_id   Device identifier.
 * @param[in] packet      Packet queue.
 * @param[in] sender      Sending module service.
 *
 * @return EOK on success.
 * @return Other error codes as defined for the generic_send_msg()
 *         function.
 *
 */
static int netif_send_msg_local(int netif_phone, device_id_t device_id,
    packet_t *packet, services_t sender)
{
	fibril_rwlock_write_lock(&netif_globals.lock);
	int result = netif_send_message(device_id, packet, sender);
	fibril_rwlock_write_unlock(&netif_globals.lock);
	
	return result;
}

/** Start the device.
 *
 * @param[in] netif_phone Network interface phone.
 * @param[in] device_id   Device identifier.
 *
 * @return EOK on success.
 * @return Other error codes as defined for the find_device()
 *         function.
 * @return Other error codes as defined for the
 *         netif_start_message() function.
 *
 */
static int netif_start_req_local(int netif_phone, device_id_t device_id)
{
	fibril_rwlock_write_lock(&netif_globals.lock);
	
	netif_device_t *device;
	int rc = find_device(device_id, &device);
	if (rc != EOK) {
		fibril_rwlock_write_unlock(&netif_globals.lock);
		return rc;
	}
	
	int result = netif_start_message(device);
	if (result > NETIF_NULL) {
		int phone = device->nil_phone;
		nil_device_state_msg(phone, device_id, result);
		fibril_rwlock_write_unlock(&netif_globals.lock);
		return EOK;
	}
	
	fibril_rwlock_write_unlock(&netif_globals.lock);
	
	return result;
}

/** Stop the device.
 *
 * @param[in] netif_phone Network interface phone.
 * @param[in] device_id   Device identifier.
 *
 * @return EOK on success.
 * @return Other error codes as defined for the find_device()
 *         function.
 * @return Other error codes as defined for the
 *         netif_stop_message() function.
 *
 */
static int netif_stop_req_local(int netif_phone, device_id_t device_id)
{
	fibril_rwlock_write_lock(&netif_globals.lock);
	
	netif_device_t *device;
	int rc = find_device(device_id, &device);
	if (rc != EOK) {
		fibril_rwlock_write_unlock(&netif_globals.lock);
		return rc;
	}
	
	int result = netif_stop_message(device);
	if (result > NETIF_NULL) {
		int phone = device->nil_phone;
		nil_device_state_msg(phone, device_id, result);
		fibril_rwlock_write_unlock(&netif_globals.lock);
		return EOK;
	}
	
	fibril_rwlock_write_unlock(&netif_globals.lock);
	
	return result;
}

/** Find the device specific data.
 *
 * @param[in]  device_id Device identifier.
 * @param[out] device    Device specific data.
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
void null_device_stats(device_stats_t *stats)
{
	bzero(stats, sizeof(device_stats_t));
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
 * @param[in] content Minimum content size.
 *
 * @return The allocated packet.
 * @return NULL on error.
 *
 */
packet_t *netif_packet_get_1(size_t content)
{
	return packet_get_1_remote(netif_globals.net_phone, content);
}

/** Register the device notification receiver,
 *
 * Register a  network interface layer module as the device
 * notification receiver.
 *
 * @param[in] device_id Device identifier.
 * @param[in] phone     Network interface layer module phone.
 *
 * @return EOK on success.
 * @return ENOENT if there is no such device.
 * @return ELIMIT if there is another module registered.
 *
 */
static int register_message(device_id_t device_id, int phone)
{
	netif_device_t *device;
	int rc = find_device(device_id, &device);
	if (rc != EOK)
		return rc;
	
	if (device->nil_phone >= 0)
		return ELIMIT;
	
	device->nil_phone = phone;
	return EOK;
}

/** Process the netif module messages.
 *
 * @param[in]  callid Mmessage identifier.
 * @param[in]  call   Message.
 * @param[out] answer Answer.
 * @param[out] count  Number of arguments of the answer.
 *
 * @return EOK on success.
 * @return ENOTSUP if the message is unknown.
 * @return Other error codes as defined for each specific module
 *         message function.
 *
 * @see IS_NET_NETIF_MESSAGE()
 *
 */
static int netif_module_message(ipc_callid_t callid, ipc_call_t *call,
    ipc_call_t *answer, size_t *count)
{
	size_t length;
	device_stats_t stats;
	packet_t *packet;
	measured_string_t address;
	int rc;
	
	*count = 0;
	
	if (!IPC_GET_IMETHOD(*call))
		return EOK;
	
	switch (IPC_GET_IMETHOD(*call)) {
	case NET_NETIF_PROBE:
		return netif_probe_req_local(0, IPC_GET_DEVICE(*call),
		    NETIF_GET_IRQ(*call), NETIF_GET_IO(*call));
	
	case IPC_M_CONNECT_TO_ME:
		fibril_rwlock_write_lock(&netif_globals.lock);
		
		rc = register_message(IPC_GET_DEVICE(*call), IPC_GET_PHONE(*call));
		
		fibril_rwlock_write_unlock(&netif_globals.lock);
		return rc;
	
	case NET_NETIF_SEND:
		rc = packet_translate_remote(netif_globals.net_phone, &packet,
		    IPC_GET_PACKET(*call));
		if (rc != EOK)
			return rc;
		
		return netif_send_msg_local(0, IPC_GET_DEVICE(*call), packet,
		    IPC_GET_SENDER(*call));
	
	case NET_NETIF_START:
		return netif_start_req_local(0, IPC_GET_DEVICE(*call));
	
	case NET_NETIF_STATS:
		fibril_rwlock_read_lock(&netif_globals.lock);
		
		rc = async_data_read_receive(&callid, &length);
		if (rc != EOK) {
			fibril_rwlock_read_unlock(&netif_globals.lock);
			return rc;
		}
		
		if (length < sizeof(device_stats_t)) {
			fibril_rwlock_read_unlock(&netif_globals.lock);
			return EOVERFLOW;
		}
		
		rc = netif_get_device_stats(IPC_GET_DEVICE(*call), &stats);
		if (rc == EOK) {
			rc = async_data_read_finalize(callid, &stats,
			    sizeof(device_stats_t));
		}
		
		fibril_rwlock_read_unlock(&netif_globals.lock);
		return rc;
	
	case NET_NETIF_STOP:
		return netif_stop_req_local(0, IPC_GET_DEVICE(*call));
	
	case NET_NETIF_GET_ADDR:
		fibril_rwlock_read_lock(&netif_globals.lock);
		
		rc = netif_get_addr_message(IPC_GET_DEVICE(*call), &address);
		if (rc == EOK)
			rc = measured_strings_reply(&address, 1);
		
		fibril_rwlock_read_unlock(&netif_globals.lock);
		return rc;
	}
	
	return netif_specific_message(callid, call, answer, count);
}

/** Default fibril for new module connections.
 *
 * @param[in] iid   Initial message identifier.
 * @param[in] icall Initial message call structure.
 *
 */
static void netif_client_connection(ipc_callid_t iid, ipc_call_t *icall)
{
	/*
	 * Accept the connection by answering
	 * the initial IPC_M_CONNECT_ME_TO call.
	 */
	async_answer_0(iid, EOK);
	
	while (true) {
		ipc_call_t answer;
		size_t count;
		
		/* Clear the answer structure */
		refresh_answer(&answer, &count);
		
		/* Fetch the next message */
		ipc_call_t call;
		ipc_callid_t callid = async_get_call(&call);
		
		/* Process the message */
		int res = netif_module_message(callid, &call, &answer, &count);
		
		/* End if said to either by the message or the processing result */
		if ((!IPC_GET_IMETHOD(call)) || (res == EHANGUP))
			return;
		
		/* Answer the message */
		answer_call(callid, res, &answer, count);
	}
}

/** Start the network interface module.
 *
 * Initialize the client connection serving function, initialize the module,
 * registers the module service and start the async manager, processing IPC
 * messages in an infinite loop.
 *
 * @return EOK on success.
 * @return Other error codes as defined for each specific module
 *         message function.
 *
 */
int netif_module_start(void)
{
	async_set_client_connection(netif_client_connection);
	
	netif_globals.net_phone = connect_to_service(SERVICE_NETWORKING);
	netif_device_map_initialize(&netif_globals.device_map);
	
	int rc = pm_init();
	if (rc != EOK)
		return rc;
	
	fibril_rwlock_initialize(&netif_globals.lock);
	
	rc = netif_initialize();
	if (rc != EOK) {
		pm_destroy();
		return rc;
	}
	
	async_manager();
	
	pm_destroy();
	return EOK;
}

/** @}
 */
