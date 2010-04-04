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

/** @addtogroup arp
 *  @{
 */

/** @file
 *  ARP interface implementation for standalone remote modules.
 *  @see arp_interface.h
 */

#include <async.h>
#include <errno.h>
#include <ipc/ipc.h>
#include <ipc/services.h>

#include <net_messages.h>
#include <net_modules.h>
#include <net_device.h>
#include <arp_interface.h>
#include <adt/measured_strings.h>
#include <arp_messages.h>

int arp_connect_module(services_t service){
	if(service != SERVICE_ARP){
		return EINVAL;
	}
	return connect_to_service(SERVICE_ARP);
}

int arp_clean_cache_req(int arp_phone){
	return (int) async_req_0_0(arp_phone, NET_ARP_CLEAN_CACHE);
}

int arp_clear_address_req(int arp_phone, device_id_t device_id, services_t protocol, measured_string_ref address){
	aid_t message_id;
	ipcarg_t result;

	message_id = async_send_2(arp_phone, NET_ARP_CLEAR_ADDRESS, (ipcarg_t) device_id, protocol, NULL);
	measured_strings_send(arp_phone, address, 1);
	async_wait_for(message_id, &result);
	return (int) result;
}

int arp_clear_device_req(int arp_phone, device_id_t device_id){
	return (int) async_req_1_0(arp_phone, NET_ARP_CLEAR_DEVICE, (ipcarg_t) device_id);
}

int arp_device_req(int arp_phone, device_id_t device_id, services_t protocol, services_t netif, measured_string_ref address){
	aid_t message_id;
	ipcarg_t result;

	message_id = async_send_3(arp_phone, NET_ARP_DEVICE, (ipcarg_t) device_id, protocol, netif, NULL);
	measured_strings_send(arp_phone, address, 1);
	async_wait_for(message_id, &result);
	return (int) result;
}

task_id_t arp_task_get_id(void){
	return 0;
}

int arp_translate_req(int arp_phone, device_id_t device_id, services_t protocol, measured_string_ref address, measured_string_ref * translation, char ** data){
	return generic_translate_req(arp_phone, NET_ARP_TRANSLATE, device_id, protocol, address, 1, translation, data);
}

/** @}
 */
