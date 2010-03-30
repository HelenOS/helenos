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
 *  @{
 */

/** @file
 *  Network interface module skeleton implementation.
 *  @see netif.h
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
#include <adt/measured_strings.h>
#include <net_device.h>
#include <netif_interface.h>
#include <nil_interface.h>
#include <netif.h>
#include <netif_messages.h>
#include <netif_module.h>

DEVICE_MAP_IMPLEMENT(device_map, device_t)

/** Network interface global data.
 */
netif_globals_t netif_globals;

int netif_probe_req(int netif_phone, device_id_t device_id, int irq, int io){
	int result;

	fibril_rwlock_write_lock(&netif_globals.lock);
	result = netif_probe_message(device_id, irq, io);
	fibril_rwlock_write_unlock(&netif_globals.lock);
	return result;
}

int netif_send_msg(int netif_phone, device_id_t device_id, packet_t packet, services_t sender){
	int result;

	fibril_rwlock_write_lock(&netif_globals.lock);
	result = netif_send_message(device_id, packet, sender);
	fibril_rwlock_write_unlock(&netif_globals.lock);
	return result;
}

int netif_start_req(int netif_phone, device_id_t device_id){
	ERROR_DECLARE;

	device_ref device;
	int result;
	int phone;

	fibril_rwlock_write_lock(&netif_globals.lock);
	if(ERROR_OCCURRED(find_device(device_id, &device))){
		fibril_rwlock_write_unlock(&netif_globals.lock);
		return ERROR_CODE;
	}
	result = netif_start_message(device);
	if(result > NETIF_NULL){
		phone = device->nil_phone;
		fibril_rwlock_write_unlock(&netif_globals.lock);
		nil_device_state_msg(phone, device_id, result);
		return EOK;
	}else{
		fibril_rwlock_write_unlock(&netif_globals.lock);
	}
	return result;
}

int netif_stop_req(int netif_phone, device_id_t device_id){
	ERROR_DECLARE;

	device_ref device;
	int result;
	int phone;

	fibril_rwlock_write_lock(&netif_globals.lock);
	if(ERROR_OCCURRED(find_device(device_id, &device))){
		fibril_rwlock_write_unlock(&netif_globals.lock);
		return ERROR_CODE;
	}
	result = netif_stop_message(device);
	if(result > NETIF_NULL){
		phone = device->nil_phone;
		fibril_rwlock_write_unlock(&netif_globals.lock);
		nil_device_state_msg(phone, device_id, result);
		return EOK;
	}else{
		fibril_rwlock_write_unlock(&netif_globals.lock);
	}
	return result;
}

int netif_stats_req(int netif_phone, device_id_t device_id, device_stats_ref stats){
	int res;

	fibril_rwlock_read_lock(&netif_globals.lock);
	res = netif_get_device_stats(device_id, stats);
	fibril_rwlock_read_unlock(&netif_globals.lock);
	return res;
}

int netif_get_addr_req(int netif_phone, device_id_t device_id, measured_string_ref * address, char ** data){
	ERROR_DECLARE;

	measured_string_t translation;

	if(!(address && data)){
		return EBADMEM;
	}
	fibril_rwlock_read_lock(&netif_globals.lock);
	if(! ERROR_OCCURRED(netif_get_addr_message(device_id, &translation))){
		*address = measured_string_copy(&translation);
		ERROR_CODE = (*address) ? EOK : ENOMEM;
	}
	fibril_rwlock_read_unlock(&netif_globals.lock);
	*data = (** address).value;
	return ERROR_CODE;
}

int netif_bind_service(services_t service, device_id_t device_id, services_t me, async_client_conn_t receiver){
	return EOK;
}

int find_device(device_id_t device_id, device_ref * device){
	if(! device){
		return EBADMEM;
	}
	*device = device_map_find(&netif_globals.device_map, device_id);
	if(! * device){
		return ENOENT;
	}
	if((** device).state == NETIF_NULL) return EPERM;
	return EOK;
}

void null_device_stats(device_stats_ref stats){
	bzero(stats, sizeof(device_stats_t));
}

/** Registers the device notification receiver, the network interface layer module.
 *  @param[in] device_id The device identifier.
 *  @param[in] phone The network interface layer module phone.
 *  @returns EOK on success.
 *  @returns ENOENT if there is no such device.
 *  @returns ELIMIT if there is another module registered.
 */
static int register_message(device_id_t device_id, int phone){
	ERROR_DECLARE;

	device_ref device;

	ERROR_PROPAGATE(find_device(device_id, &device));
	if(device->nil_phone > 0){
		return ELIMIT;
	}
	device->nil_phone = phone;
	printf("New receiver of the device %d registered:\n\tphone\t= %d\n", device->device_id, device->nil_phone);
	return EOK;
}

int netif_message(ipc_callid_t callid, ipc_call_t * call, ipc_call_t * answer, int * answer_count){
	ERROR_DECLARE;

	size_t length;
	device_stats_t stats;
	packet_t packet;
	measured_string_t address;

//	printf("message %d - %d\n", IPC_GET_METHOD(*call), NET_NETIF_FIRST);
	*answer_count = 0;
	switch(IPC_GET_METHOD(*call)){
		case IPC_M_PHONE_HUNGUP:
			return EOK;
		case NET_NETIF_PROBE:
			return netif_probe_req(0, IPC_GET_DEVICE(call), NETIF_GET_IRQ(call), NETIF_GET_IO(call));
		case IPC_M_CONNECT_TO_ME:
			fibril_rwlock_write_lock(&netif_globals.lock);
			ERROR_CODE = register_message(IPC_GET_DEVICE(call), IPC_GET_PHONE(call));
			fibril_rwlock_write_unlock(&netif_globals.lock);
			return ERROR_CODE;
		case NET_NETIF_SEND:
			ERROR_PROPAGATE(packet_translate(netif_globals.net_phone, &packet, IPC_GET_PACKET(call)));
			return netif_send_msg(0, IPC_GET_DEVICE(call), packet, IPC_GET_SENDER(call));
		case NET_NETIF_START:
			return netif_start_req(0, IPC_GET_DEVICE(call));
		case NET_NETIF_STATS:
			fibril_rwlock_read_lock(&netif_globals.lock);
			if(! ERROR_OCCURRED(async_data_read_receive(&callid, &length))){
				if(length < sizeof(device_stats_t)){
					ERROR_CODE = EOVERFLOW;
				}else{
					if(! ERROR_OCCURRED(netif_get_device_stats(IPC_GET_DEVICE(call), &stats))){
						ERROR_CODE = async_data_read_finalize(callid, &stats, sizeof(device_stats_t));
					}
				}
			}
			fibril_rwlock_read_unlock(&netif_globals.lock);
			return ERROR_CODE;
		case NET_NETIF_STOP:
			return netif_stop_req(0, IPC_GET_DEVICE(call));
		case NET_NETIF_GET_ADDR:
			fibril_rwlock_read_lock(&netif_globals.lock);
			if(! ERROR_OCCURRED(netif_get_addr_message(IPC_GET_DEVICE(call), &address))){
				ERROR_CODE = measured_strings_reply(&address, 1);
			}
			fibril_rwlock_read_unlock(&netif_globals.lock);
			return ERROR_CODE;
	}
	return netif_specific_message(callid, call, answer, answer_count);
}

int netif_init_module(async_client_conn_t client_connection){
	ERROR_DECLARE;

	async_set_client_connection(client_connection);
	netif_globals.net_phone = connect_to_service(SERVICE_NETWORKING);
	device_map_initialize(&netif_globals.device_map);
	ERROR_PROPAGATE(pm_init());
	fibril_rwlock_initialize(&netif_globals.lock);
	if(ERROR_OCCURRED(netif_initialize())){
		pm_destroy();
		return ERROR_CODE;
	}
	return EOK;
}

int netif_run_module(void){
	async_manager();

	pm_destroy();
	return EOK;
}

void netif_pq_release(packet_id_t packet_id){
	pq_release(netif_globals.net_phone, packet_id);
}

packet_t netif_packet_get_1(size_t content){
	return packet_get_1(netif_globals.net_phone, content);
}

/** @}
 */
