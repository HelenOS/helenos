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

/** @addtogroup lo
 * @{
 */

/** @file
 * Loopback network interface implementation.
 */

#include <async.h>
#include <errno.h>
#include <stdio.h>
#include <str.h>

#include <ipc/ipc.h>
#include <ipc/services.h>
#include <ipc/nil.h>

#include <net/modules.h>
#include <adt/measured_strings.h>
#include <packet_client.h>
#include <net/device.h>
#include <netif_skel.h>
#include <nil_remote.h>

/** Default address length. */
#define DEFAULT_ADDR_LEN  6

/** Loopback module name. */
#define NAME  "lo"

static uint8_t default_addr[DEFAULT_ADDR_LEN] =
    {0, 0, 0, 0, 0, 0};

int netif_specific_message(ipc_callid_t callid, ipc_call_t *call,
    ipc_call_t *answer, size_t *count)
{
	return ENOTSUP;
}

int netif_get_addr_message(device_id_t device_id, measured_string_t *address)
{
	if (!address)
		return EBADMEM;
	
	address->value = default_addr;
	address->length = DEFAULT_ADDR_LEN;
	
	return EOK;
}

int netif_get_device_stats(device_id_t device_id, device_stats_t *stats)
{
	if (!stats)
		return EBADMEM;
	
	netif_device_t *device;
	int rc = find_device(device_id, &device);
	if (rc != EOK)
		return rc;
	
	memcpy(stats, (device_stats_t *) device->specific,
	    sizeof(device_stats_t));
	
	return EOK;
}

/** Change the loopback state.
 *
 * @param[in] device The device structure.
 * @param[in] state  The new device state.
 *
 * @return New state if changed.
 * @return EOK otherwise.
 *
 */
static void change_state_message(netif_device_t *device, device_state_t state)
{
	if (device->state != state) {
		device->state = state;
		
		const char *desc;
		switch (state) {
		case NETIF_ACTIVE:
			desc = "active";
			break;
		case NETIF_STOPPED:
			desc = "stopped";
			break;
		default:
			desc = "unknown";
		}
		
		printf("%s: State changed to %s\n", NAME, desc);
	}
}

/** Create and return the loopback network interface structure.
 *
 * @param[in]  device_id New devce identifier.
 * @param[out] device    Device structure.
 *
 * @return EOK on success.
 * @return EXDEV if one loopback network interface already exists.
 * @return ENOMEM if there is not enough memory left.
 *
 */
static int lo_create(device_id_t device_id, netif_device_t **device)
{
	if (netif_device_map_count(&netif_globals.device_map) > 0)
		return EXDEV;
	
	*device = (netif_device_t *) malloc(sizeof(netif_device_t));
	if (!*device)
		return ENOMEM;
	
	(*device)->specific = (device_stats_t *) malloc(sizeof(device_stats_t));
	if (!(*device)->specific) {
		free(*device);
		return ENOMEM;
	}
	
	null_device_stats((device_stats_t *) (*device)->specific);
	(*device)->device_id = device_id;
	(*device)->nil_phone = -1;
	(*device)->state = NETIF_STOPPED;
	int index = netif_device_map_add(&netif_globals.device_map,
	    (*device)->device_id, *device);
	
	if (index < 0) {
		free(*device);
		free((*device)->specific);
		*device = NULL;
		return index;
	}
	
	return EOK;
}

int netif_initialize(void)
{
	return ipc_connect_to_me(PHONE_NS, SERVICE_LO, 0, 0, NULL, NULL);
}

int netif_probe_message(device_id_t device_id, int irq, void *io)
{
	/* Create a new device */
	netif_device_t *device;
	int rc = lo_create(device_id, &device);
	if (rc != EOK)
		return rc;
	
	printf("%s: Device created (id: %d)\n", NAME, device->device_id);
	return EOK;
}

int netif_send_message(device_id_t device_id, packet_t *packet, services_t sender)
{
	netif_device_t *device;
	int rc = find_device(device_id, &device);
	if (rc != EOK)
		return EOK;
	
	if (device->state != NETIF_ACTIVE) {
		netif_pq_release(packet_get_id(packet));
		return EFORWARD;
	}
	
	packet_t *next = packet;
	do {
		((device_stats_t *) device->specific)->send_packets++;
		((device_stats_t *) device->specific)->receive_packets++;
		size_t length = packet_get_data_length(next);
		((device_stats_t *) device->specific)->send_bytes += length;
		((device_stats_t *) device->specific)->receive_bytes += length;
		next = pq_next(next);
	} while (next);
	
	int phone = device->nil_phone;
	fibril_rwlock_write_unlock(&netif_globals.lock);
	
	nil_received_msg(phone, device_id, packet, sender);
	
	fibril_rwlock_write_lock(&netif_globals.lock);
	return EOK;
}

int netif_start_message(netif_device_t *device)
{
	change_state_message(device, NETIF_ACTIVE);
	return device->state;
}

int netif_stop_message(netif_device_t *device)
{
	change_state_message(device, NETIF_STOPPED);
	return device->state;
}

int main(int argc, char *argv[])
{
	/* Start the module */
	return netif_module_start();
}

/** @}
 */
