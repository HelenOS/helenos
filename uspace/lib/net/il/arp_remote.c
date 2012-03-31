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
#include <ipc/services.h>
#include <ipc/arp.h>
#include <net/modules.h>
#include <net/device.h>
#include <adt/measured_strings.h>
#include <async.h>
#include <errno.h>

/** Connect to the ARP module.
 *
 * @return ARP module session on success.
 *
 */
async_sess_t *arp_connect_module(services_t service)
{
	// FIXME: Get rid of the useless argument
	return connect_to_service(SERVICE_ARP);
}

/** Cleans the cache.
 *
 * @param[in] sess ARP module session.
 *
 * @return EOK on success.
 *
 */
int arp_clean_cache_req(async_sess_t *sess)
{
	async_exch_t *exch = async_exchange_begin(sess);
	int rc = async_req_0_0(exch, NET_ARP_CLEAN_CACHE);
	async_exchange_end(exch);
	
	return rc;
}

/** Clear the given protocol address from the cache.
 *
 * @param[in] sess      ARP module session.
 * @param[in] device_id Device identifier.
 * @param[in] protocol  Requesting protocol service.
 * @param[in] address   Protocol address to be cleared.
 *
 * @return EOK on success.
 * @return ENOENT if the mapping is not found.
 *
 */
int arp_clear_address_req(async_sess_t *sess, nic_device_id_t device_id,
    services_t protocol, measured_string_t *address)
{
	async_exch_t *exch = async_exchange_begin(sess);
	aid_t message_id = async_send_2(exch, NET_ARP_CLEAR_ADDRESS,
	    (sysarg_t) device_id, protocol, NULL);
	measured_strings_send(exch, address, 1);
	async_exchange_end(exch);
	
	sysarg_t result;
	async_wait_for(message_id, &result);
	
	return (int) result;
}

/** Clear the device cache.
 *
 * @param[in] sess      ARP module session.
 * @param[in] device_id Device identifier.
 *
 * @return EOK on success.
 * @return ENOENT if the device is not found.
 *
 */
int arp_clear_device_req(async_sess_t *sess, nic_device_id_t device_id)
{
	async_exch_t *exch = async_exchange_begin(sess);
	int rc = async_req_1_0(exch, NET_ARP_CLEAR_DEVICE,
	    (sysarg_t) device_id);
	async_exchange_end(exch);
	
	return rc;
}

/** Register new device and the requesting protocol service.
 *
 * Connect to the network interface layer service.
 * Determine the device broadcast address, its address lengths and packet size.
 *
 * @param[in] sess      ARP module session.
 * @param[in] device_id New device identifier.
 * @param[in] protocol  Requesting protocol service.
 * @param[in] netif     Underlying device network interface layer service.
 * @param[in] address   Local requesting protocol address of the device.
 *
 * @return EOK on success.
 * @return EEXIST if the device is already used.
 * @return ENOMEM if there is not enough memory left.
 * @return ENOENT if the network interface service is not known.
 * @return EREFUSED if the network interface service is not
 *         responding.
 * @return Other error codes as defined for the
 *         nil_packet_get_size() function.
 * @return Other error codes as defined for the nil_get_addr()
 *         function.
 * @return Other error codes as defined for the
 *         nil_get_broadcast_addr() function.
 *
 */
int arp_device_req(async_sess_t *sess, nic_device_id_t device_id,
    services_t protocol, services_t netif, measured_string_t *address)
{
	async_exch_t *exch = async_exchange_begin(sess);
	aid_t message_id = async_send_3(exch, NET_ARP_DEVICE,
	    (sysarg_t) device_id, protocol, netif, NULL);
	measured_strings_send(exch, address, 1);
	async_exchange_end(exch);
	
	sysarg_t result;
	async_wait_for(message_id, &result);

	return (int) result;
}

/** Translate the given protocol address to the network interface address.
 *
 * Broadcast the ARP request if the mapping is not found.
 * Allocate and returns the needed memory block as the data parameter.
 *
 * @param[in]  sess        ARP module session.
 * @param[in]  device_id   Device identifier.
 * @param[in]  protocol    Requesting protocol service.
 * @param[in]  address     Local requesting protocol address.
 * @param[out] translation Translation of the local protocol address.
 * @param[out] data        Allocated raw translation data container.
 *
 * @return EOK on success.
 * @return EINVAL if the address parameter is NULL.
 * @return EBADMEM if the translation or the data parameters are
 *         NULL.
 * @return ENOENT if the mapping is not found.
 *
 */
int arp_translate_req(async_sess_t *sess, nic_device_id_t device_id,
    services_t protocol, measured_string_t *address,
    measured_string_t **translation, uint8_t **data)
{
	return generic_translate_req(sess, NET_ARP_TRANSLATE, device_id,
	    protocol, address, 1, translation, data);
}

/** @}
 */
