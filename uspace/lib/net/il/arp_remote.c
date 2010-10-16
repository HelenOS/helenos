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
 * ARP interface implementation for remote modules.
 * @see arp_interface.h
 */

#include <arp_interface.h>
#include <generic.h>

#include <async.h>
#include <errno.h>
#include <ipc/ipc.h>
#include <ipc/services.h>
#include <ipc/arp.h>

#include <net/modules.h>
#include <net/device.h>
#include <adt/measured_strings.h>

/** Connects to the ARP module.
 *
 * @param service	The ARP module service. Ignored parameter.
 * @returns		The ARP module phone on success.
 */
int arp_connect_module(services_t service)
{
	if (service != SERVICE_ARP)
		return EINVAL;

	return connect_to_service(SERVICE_ARP);
}

/** Cleans the cache.
 *
 * @param[in] arp_phone	The ARP module phone used for (semi)remote calls.
 * @returns		EOK on success.
 */
int arp_clean_cache_req(int arp_phone)
{
	return (int) async_req_0_0(arp_phone, NET_ARP_CLEAN_CACHE);
}

/** Clears the given protocol address from the cache.
 *
 * @param[in] arp_phone	The ARP module phone used for (semi)remote calls.
 * @param[in] device_id	The device identifier.
 * @param[in] protocol	The requesting protocol service.
 * @param[in] address	The protocol address to be cleared.
 * @returns		EOK on success.
 * @returns		ENOENT if the mapping is not found.
 */
int
arp_clear_address_req(int arp_phone, device_id_t device_id, services_t protocol,
    measured_string_ref address)
{
	aid_t message_id;
	ipcarg_t result;

	message_id = async_send_2(arp_phone, NET_ARP_CLEAR_ADDRESS,
	    (ipcarg_t) device_id, protocol, NULL);
	measured_strings_send(arp_phone, address, 1);
	async_wait_for(message_id, &result);

	return (int) result;
}

/** Clears the device cache.
 *
 * @param[in] arp_phone	The ARP module phone used for (semi)remote calls.
 * @param[in] device_id	The device identifier.
 * @returns		EOK on success.
 * @returns		ENOENT if the device is not found.
 */
int arp_clear_device_req(int arp_phone, device_id_t device_id)
{
	return (int) async_req_1_0(arp_phone, NET_ARP_CLEAR_DEVICE,
	    (ipcarg_t) device_id);
}

/** Registers the new device and the requesting protocol service.
 *
 * Connects to the network interface layer service.
 * Determines the device broadcast address, its address lengths and packet size.
 *
 * @param[in] arp_phone	The ARP module phone used for (semi)remote calls.
 * @param[in] device_id	The new device identifier.
 * @param[in] protocol	The requesting protocol service.
 * @param[in] netif	The underlying device network interface layer service.
 * @param[in] address	The local requesting protocol address of the device.
 * @returns		EOK on success.
 * @returns		EEXIST if the device is already used.
 * @returns		ENOMEM if there is not enough memory left.
 * @returns		ENOENT if the network interface service is not known.
 * @returns		EREFUSED if the network interface service is not
 *			responding.
 * @returns		Other error codes as defined for the
 *			nil_packet_get_size() function.
 * @returns		Other error codes as defined for the nil_get_addr()
 *			function.
 * @returns		Other error codes as defined for the
 *			nil_get_broadcast_addr() function.
 */
int arp_device_req(int arp_phone, device_id_t device_id, services_t protocol,
    services_t netif, measured_string_ref address)
{
	aid_t message_id;
	ipcarg_t result;

	message_id = async_send_3(arp_phone, NET_ARP_DEVICE,
	    (ipcarg_t) device_id, protocol, netif, NULL);
	measured_strings_send(arp_phone, address, 1);
	async_wait_for(message_id, &result);

	return (int) result;
}

/** Returns the ARP task identifier.
 *
 * @returns		0 if called by the remote module.
 */
task_id_t arp_task_get_id(void)
{
	return 0;
}

/** Translates the given protocol address to the network interface address.
 *
 * Broadcasts the ARP request if the mapping is not found.
 * Allocates and returns the needed memory block as the data parameter.
 *
 * @param[in] arp_phone	The ARP module phone used for (semi)remote calls.
 * @param[in] device_id	The device identifier.
 * @param[in] protocol	The requesting protocol service.
 * @param[in] address	The local requesting protocol address.
 * @param[out] translation The translation of the local protocol address.
 * @param[out] data	The allocated raw translation data container.
 * @returns		EOK on success.
 * @returns		EINVAL if the address parameter is NULL.
 * @returns		EBADMEM if the translation or the data parameters are
 *			NULL.
 * @returns		ENOENT if the mapping is not found.
 */
int
arp_translate_req(int arp_phone, device_id_t device_id, services_t protocol,
    measured_string_ref address, measured_string_ref *translation, char **data)
{
	return generic_translate_req(arp_phone, NET_ARP_TRANSLATE, device_id,
	    protocol, address, 1, translation, data);
}

/** @}
 */
