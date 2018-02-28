/*
 * Copyright (c) 2011 Radim Vansa
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

/**
 * @addtogroup libnic
 * @{
 */
/**
 * @file
 * @brief Default DDF NIC interface methods implementations
 */

#include <errno.h>
#include <str_error.h>
#include <ipc/services.h>
#include <ns.h>
#include "nic_driver.h"
#include "nic_ev.h"
#include "nic_impl.h"

/**
 * Default implementation of the set_state method. Trivial.
 *
 * @param		fun
 * @param[out]	state
 *
 * @return EOK always.
 */
errno_t nic_get_state_impl(ddf_fun_t *fun, nic_device_state_t *state)
{
	nic_t *nic_data = nic_get_from_ddf_fun(fun);
	fibril_rwlock_read_lock(&nic_data->main_lock);
	*state = nic_data->state;
	fibril_rwlock_read_unlock(&nic_data->main_lock);
	return EOK;
}

/**
 * Default implementation of the set_state method. Changes the internal
 * driver's state, calls the appropriate callback and notifies the NIL service
 * about this change.
 *
 * @param	fun
 * @param	state	The new device's state
 *
 * @return EOK		If the state was changed
 * @return EINVAL	If the state cannot be changed
 */
errno_t nic_set_state_impl(ddf_fun_t *fun, nic_device_state_t state)
{
	if (state >= NIC_STATE_MAX) {
		return EINVAL;
	}

	nic_t *nic_data = nic_get_from_ddf_fun(fun);

	fibril_rwlock_write_lock(&nic_data->main_lock);
	if (nic_data->state == state) {
		/* No change, nothing to do */
		fibril_rwlock_write_unlock(&nic_data->main_lock);
		return EOK;
	}
	if (state == NIC_STATE_ACTIVE) {
		if (nic_data->client_session == NULL) {
			fibril_rwlock_write_unlock(&nic_data->main_lock);
			return EINVAL;
		}
	}

	state_change_handler event_handler = NULL;
	switch (state) {
	case NIC_STATE_STOPPED:
		event_handler = nic_data->on_stopping;
		break;
	case NIC_STATE_DOWN:
		event_handler = nic_data->on_going_down;
		break;
	case NIC_STATE_ACTIVE:
		event_handler = nic_data->on_activating;
		break;
	default:
		break;
	}
	if (event_handler != NULL) {
		errno_t rc = event_handler(nic_data);
		if (rc != EOK) {
			fibril_rwlock_write_unlock(&nic_data->main_lock);
			return EINVAL;
		}
	}

	if (state == NIC_STATE_STOPPED) {
		/* Notify upper layers that we are reseting the MAC */
		errno_t rc = nic_ev_addr_changed(nic_data->client_session,
			&nic_data->default_mac);
		nic_data->poll_mode = nic_data->default_poll_mode;
		memcpy(&nic_data->poll_period, &nic_data->default_poll_period,
			sizeof (struct timeval));
		if (rc != EOK) {
			/* We have already ran the on stopped handler, even if we
			 * terminated the state change we would end up in undefined state.
			 * Therefore we just log the problem. */
		}

		fibril_rwlock_write_lock(&nic_data->stats_lock);
		memset(&nic_data->stats, 0, sizeof(nic_device_stats_t));
		fibril_rwlock_write_unlock(&nic_data->stats_lock);

		fibril_rwlock_write_lock(&nic_data->rxc_lock);
		nic_rxc_clear(&nic_data->rx_control);
		/* Reinsert device's default MAC */
		nic_rxc_set_addr(&nic_data->rx_control, NULL,
			&nic_data->default_mac);
		fibril_rwlock_write_unlock(&nic_data->rxc_lock);
		memcpy(&nic_data->mac, &nic_data->default_mac, sizeof (nic_address_t));

		fibril_rwlock_write_lock(&nic_data->wv_lock);
		nic_wol_virtues_clear(&nic_data->wol_virtues);
		fibril_rwlock_write_unlock(&nic_data->wv_lock);

		/* Ensure stopping period of NIC_POLL_SOFTWARE_PERIODIC */
		nic_sw_period_stop(nic_data);
	}

	nic_data->state = state;

	nic_ev_device_state(nic_data->client_session, state);

	fibril_rwlock_write_unlock(&nic_data->main_lock);

	return EOK;
}

/**
 * Default implementation of the send_frame method.
 * Send messages to the network.
 *
 * @param	fun
 * @param	data	Frame data
 * @param 	size	Frame size in bytes
 *
 * @return EOK		If the message was sent
 * @return EBUSY	If the device is not in state when the frame can be sent.
 */
errno_t nic_send_frame_impl(ddf_fun_t *fun, void *data, size_t size)
{
	nic_t *nic_data = nic_get_from_ddf_fun(fun);

	fibril_rwlock_read_lock(&nic_data->main_lock);
	if (nic_data->state != NIC_STATE_ACTIVE || nic_data->tx_busy) {
		fibril_rwlock_read_unlock(&nic_data->main_lock);
		return EBUSY;
	}

	nic_data->send_frame(nic_data, data, size);
	fibril_rwlock_read_unlock(&nic_data->main_lock);
	return EOK;
}

/**
 * Default implementation of the connect_client method.
 * Creates callback connection to the client.
 *
 * @param	fun
 *
 * @return EOK		On success, or an error code.
 */
errno_t nic_callback_create_impl(ddf_fun_t *fun)
{
	nic_t *nic = nic_get_from_ddf_fun(fun);
	fibril_rwlock_write_lock(&nic->main_lock);

	nic->client_session = async_callback_receive(EXCHANGE_SERIALIZE);
	if (nic->client_session == NULL) {
		fibril_rwlock_write_unlock(&nic->main_lock);
		return ENOMEM;
	}

	fibril_rwlock_write_unlock(&nic->main_lock);
	return EOK;
}

/**
 * Default implementation of the get_address method.
 * Retrieves the NIC's physical address.
 *
 * @param	fun
 * @param	address	Pointer to the structure where the address will be stored.
 *
 * @return EOK		If the services were bound
 * @return ELIMIT	If the buffer is too short
 */
errno_t nic_get_address_impl(ddf_fun_t *fun, nic_address_t *address)
{
	assert(address);
	nic_t *nic_data = nic_get_from_ddf_fun(fun);
	fibril_rwlock_read_lock(&nic_data->main_lock);
	memcpy(address, &nic_data->mac, sizeof (nic_address_t));
	fibril_rwlock_read_unlock(&nic_data->main_lock);
	return EOK;
}

/**
 * Default implementation of the get_stats method. Copies the statistics from
 * the drivers data to supplied buffer.
 *
 * @param		fun
 * @param[out]	stats	The buffer for statistics
 *
 * @return EOK (cannot fail)
 */
errno_t nic_get_stats_impl(ddf_fun_t *fun, nic_device_stats_t *stats)
{
	nic_t *nic_data = nic_get_from_ddf_fun(fun);
	assert (stats != NULL);
	fibril_rwlock_read_lock(&nic_data->stats_lock);
	memcpy(stats, &nic_data->stats, sizeof (nic_device_stats_t));
	fibril_rwlock_read_unlock(&nic_data->stats_lock);
	return EOK;
}

/**
 * Default implementation of unicast_get_mode method.
 *
 * @param		fun
 * @param[out]	mode		Current operation mode
 * @param[in]	max_count	Max number of addresses that can be written into the
 * 							buffer (addr_list).
 * @param[out]	addr_list	Buffer for addresses
 * @param[out]	addr_count	Number of addresses written into the list
 *
 * @return EOK
 */
errno_t nic_unicast_get_mode_impl(ddf_fun_t *fun, nic_unicast_mode_t *mode,
	size_t max_count, nic_address_t *addr_list, size_t *addr_count)
{
	nic_t *nic_data = nic_get_from_ddf_fun(fun);
	fibril_rwlock_read_lock(&nic_data->rxc_lock);
	nic_rxc_unicast_get_mode(&nic_data->rx_control, mode, max_count,
		addr_list, addr_count);
	fibril_rwlock_read_unlock(&nic_data->rxc_lock);
	return EOK;
}

/**
 * Default implementation of unicast_set_mode method.
 *
 * @param		fun
 * @param[in]	mode		New operation mode
 * @param[in]	addr_list	List of unicast addresses
 * @param[in]	addr_count	Number of addresses in the list
 *
 * @return EOK
 * @return EINVAL
 * @return ENOTSUP
 * @return ENOMEM
 */
errno_t nic_unicast_set_mode_impl(ddf_fun_t *fun,
	nic_unicast_mode_t mode, const nic_address_t *addr_list, size_t addr_count)
{
	assert((addr_count == 0 && addr_list == NULL)
		|| (addr_count != 0 && addr_list != NULL));
	size_t i;
	for (i = 0; i < addr_count; ++i) {
		if (addr_list[i].address[0] & 1)
			return EINVAL;
	}

	nic_t *nic_data = nic_get_from_ddf_fun(fun);
	fibril_rwlock_write_lock(&nic_data->rxc_lock);
	errno_t rc = ENOTSUP;
	if (nic_data->on_unicast_mode_change) {
		rc = nic_data->on_unicast_mode_change(nic_data,
			mode, addr_list, addr_count);
	}
	if (rc == EOK) {
		rc = nic_rxc_unicast_set_mode(&nic_data->rx_control, mode,
			addr_list, addr_count);
		/* After changing the mode the addr db gets cleared, therefore we have
		 * to reinsert also the physical address of NIC.
		 */
		nic_rxc_set_addr(&nic_data->rx_control, NULL, &nic_data->mac);
	}
	fibril_rwlock_write_unlock(&nic_data->rxc_lock);
	return rc;
}


/**
 * Default implementation of multicast_get_mode method.
 *
 * @param		fun
 * @param[out]	mode		Current operation mode
 * @param[in]	max_count	Max number of addresses that can be written into the
 * 							buffer (addr_list).
 * @param[out]	addr_list	Buffer for addresses
 * @param[out]	addr_count	Number of addresses written into the list
 *
 * @return EOK
 */
errno_t nic_multicast_get_mode_impl(ddf_fun_t *fun, nic_multicast_mode_t *mode,
	size_t max_count, nic_address_t *addr_list, size_t *addr_count)
{
	nic_t *nic_data = nic_get_from_ddf_fun(fun);
	fibril_rwlock_read_lock(&nic_data->rxc_lock);
	nic_rxc_multicast_get_mode(&nic_data->rx_control, mode, max_count,
		addr_list, addr_count);
	fibril_rwlock_read_unlock(&nic_data->rxc_lock);
	return EOK;
}

/**
 * Default implementation of multicast_set_mode method.
 *
 * @param		fun
 * @param[in]	mode		New operation mode
 * @param[in]	addr_list	List of multicast addresses
 * @param[in]	addr_count	Number of addresses in the list
 *
 * @return EOK
 * @return EINVAL
 * @return ENOTSUP
 * @return ENOMEM
 */
errno_t nic_multicast_set_mode_impl(ddf_fun_t *fun,	nic_multicast_mode_t mode,
	const nic_address_t *addr_list, size_t addr_count)
{
	assert((addr_count == 0 && addr_list == NULL)
		|| (addr_count != 0 && addr_list != NULL));
	size_t i;
	for (i = 0; i < addr_count; ++i) {
		if (!(addr_list[i].address[0] & 1))
			return EINVAL;
	}

	nic_t *nic_data = nic_get_from_ddf_fun(fun);
	fibril_rwlock_write_lock(&nic_data->rxc_lock);
	errno_t rc = ENOTSUP;
	if (nic_data->on_multicast_mode_change) {
		rc = nic_data->on_multicast_mode_change(nic_data, mode, addr_list, addr_count);
	}
	if (rc == EOK) {
		rc = nic_rxc_multicast_set_mode(&nic_data->rx_control, mode,
			addr_list, addr_count);
	}
	fibril_rwlock_write_unlock(&nic_data->rxc_lock);
	return rc;
}

/**
 * Default implementation of broadcast_get_mode method.
 *
 * @param		fun
 * @param[out]	mode		Current operation mode
 *
 * @return EOK
 */
errno_t nic_broadcast_get_mode_impl(ddf_fun_t *fun, nic_broadcast_mode_t *mode)
{
	nic_t *nic_data = nic_get_from_ddf_fun(fun);
	fibril_rwlock_read_lock(&nic_data->rxc_lock);
	nic_rxc_broadcast_get_mode(&nic_data->rx_control, mode);
	fibril_rwlock_read_unlock(&nic_data->rxc_lock);
	return EOK;
}

/**
 * Default implementation of broadcast_set_mode method.
 *
 * @param		fun
 * @param[in]	mode		New operation mode
 *
 * @return EOK
 * @return EINVAL
 * @return ENOTSUP
 * @return ENOMEM
 */
errno_t nic_broadcast_set_mode_impl(ddf_fun_t *fun, nic_broadcast_mode_t mode)
{
	nic_t *nic_data = nic_get_from_ddf_fun(fun);
	fibril_rwlock_write_lock(&nic_data->rxc_lock);
	errno_t rc = ENOTSUP;
	if (nic_data->on_broadcast_mode_change) {
		rc = nic_data->on_broadcast_mode_change(nic_data, mode);
	}
	if (rc == EOK) {
		rc = nic_rxc_broadcast_set_mode(&nic_data->rx_control, mode);
	}
	fibril_rwlock_write_unlock(&nic_data->rxc_lock);
	return rc;
}

/**
 * Default implementation of blocked_sources_get method.
 *
 * @param		fun
 * @param[in]	max_count	Max number of addresses that can be written into the
 * 							buffer (addr_list).
 * @param[out]	addr_list	Buffer for addresses
 * @param[out]	addr_count	Number of addresses written into the list
 *
 * @return EOK
 */
errno_t nic_blocked_sources_get_impl(ddf_fun_t *fun,
	size_t max_count, nic_address_t *addr_list, size_t *addr_count)
{
	nic_t *nic_data = nic_get_from_ddf_fun(fun);
	fibril_rwlock_read_lock(&nic_data->rxc_lock);
	nic_rxc_blocked_sources_get(&nic_data->rx_control,
		max_count, addr_list, addr_count);
	fibril_rwlock_read_unlock(&nic_data->rxc_lock);
	return EOK;
}

/**
 * Default implementation of blocked_sources_set method.
 *
 * @param		fun
 * @param[in]	addr_list	List of blocked addresses
 * @param[in]	addr_count	Number of addresses in the list
 *
 * @return EOK
 * @return EINVAL
 * @return ENOTSUP
 * @return ENOMEM
 */
errno_t nic_blocked_sources_set_impl(ddf_fun_t *fun,
	const nic_address_t *addr_list, size_t addr_count)
{
	nic_t *nic_data = nic_get_from_ddf_fun(fun);
	fibril_rwlock_write_lock(&nic_data->rxc_lock);
	if (nic_data->on_blocked_sources_change) {
		nic_data->on_blocked_sources_change(nic_data, addr_list, addr_count);
	}
	errno_t rc = nic_rxc_blocked_sources_set(&nic_data->rx_control,
		addr_list, addr_count);
	fibril_rwlock_write_unlock(&nic_data->rxc_lock);
	return rc;
}

/**
 * Default implementation of vlan_get_mask method.
 *
 * @param		fun
 * @param[out]	mask	Current VLAN mask
 *
 * @return EOK
 * @return ENOENT	If the mask is not set
 */
errno_t nic_vlan_get_mask_impl(ddf_fun_t *fun, nic_vlan_mask_t *mask)
{
	nic_t *nic_data = nic_get_from_ddf_fun(fun);
	fibril_rwlock_read_lock(&nic_data->rxc_lock);
	errno_t rc = nic_rxc_vlan_get_mask(&nic_data->rx_control, mask);
	fibril_rwlock_read_unlock(&nic_data->rxc_lock);
	return rc;
}

/**
 * Default implementation of vlan_set_mask method.
 *
 * @param		fun
 * @param[in]	mask	The new VLAN mask
 *
 * @return EOK
 * @return ENOMEM
 */
errno_t nic_vlan_set_mask_impl(ddf_fun_t *fun, const nic_vlan_mask_t *mask)
{
	nic_t *nic_data = nic_get_from_ddf_fun(fun);
	fibril_rwlock_write_lock(&nic_data->rxc_lock);
	errno_t rc = nic_rxc_vlan_set_mask(&nic_data->rx_control, mask);
	if (rc == EOK && nic_data->on_vlan_mask_change) {
		nic_data->on_vlan_mask_change(nic_data, mask);
	}
	fibril_rwlock_write_unlock(&nic_data->rxc_lock);
	return rc;
}

/**
 * Default implementation of the wol_virtue_add method.
 * Create a new WOL virtue.
 *
 * @param[in]	fun
 * @param[in]	type		Type of the virtue
 * @param[in]	data		Data required for this virtue (depends on type)
 * @param[in]	length		Length of the data
 * @param[out]	filter		Identifier of the new virtue
 *
 * @return EOK		If the operation was successfully completed
 * @return EINVAL	If virtue type is not supported or the data are invalid
 * @return ELIMIT	If the driver does not allow to create more virtues
 * @return ENOMEM	If there was not enough memory to complete the operation
 */
errno_t nic_wol_virtue_add_impl(ddf_fun_t *fun, nic_wv_type_t type,
	const void *data, size_t length, nic_wv_id_t *new_id)
{
	nic_t *nic_data = nic_get_from_ddf_fun(fun);
	if (nic_data->on_wol_virtue_add == NULL
		|| nic_data->on_wol_virtue_remove == NULL) {
		return ENOTSUP;
	}
	if (type == NIC_WV_NONE || type >= NIC_WV_MAX) {
		return EINVAL;
	}
	if (nic_wol_virtues_verify(type, data, length) != EOK) {
		return EINVAL;
	}
	nic_wol_virtue_t *virtue = malloc(sizeof (nic_wol_virtue_t));
	if (virtue == NULL) {
		return ENOMEM;
	}
	memset(virtue, 0, sizeof(nic_wol_virtue_t));
	if (length != 0) {
		virtue->data = malloc(length);
		if (virtue->data == NULL) {
			free(virtue);
			return ENOMEM;
		}
		memcpy((void *) virtue->data, data, length);
	}
	virtue->type = type;
	virtue->length = length;

	fibril_rwlock_write_lock(&nic_data->wv_lock);
	/* Check if we haven't reached the maximum */
	if (nic_data->wol_virtues.caps_max[type] < 0) {
		fibril_rwlock_write_unlock(&nic_data->wv_lock);
		return EINVAL;
	}
	if ((int) nic_data->wol_virtues.lists_sizes[type] >=
		nic_data->wol_virtues.caps_max[type]) {
		fibril_rwlock_write_unlock(&nic_data->wv_lock);
		return ELIMIT;
	}
	/* Call the user-defined add callback */
	errno_t rc = nic_data->on_wol_virtue_add(nic_data, virtue);
	if (rc != EOK) {
		free(virtue->data);
		free(virtue);
		fibril_rwlock_write_unlock(&nic_data->wv_lock);
		return rc;
	}
	rc = nic_wol_virtues_add(&nic_data->wol_virtues, virtue);
	if (rc != EOK) {
		/* If the adding fails, call user-defined remove callback */
		nic_data->on_wol_virtue_remove(nic_data, virtue);
		fibril_rwlock_write_unlock(&nic_data->wv_lock);
		free(virtue->data);
		free(virtue);
		return rc;
	} else {
		*new_id = virtue->id;
		fibril_rwlock_write_unlock(&nic_data->wv_lock);
	}
	return EOK;
}

/**
 * Default implementation of the wol_virtue_remove method.
 * Destroys the WOL virtue.
 *
 * @param[in] fun
 * @param[in] id	WOL virtue identification
 *
 * @return EOK		If the operation was successfully completed
 * @return ENOTSUP	If the function is not supported by the driver or device
 * @return ENOENT	If the virtue identifier is not valid.
 */
errno_t nic_wol_virtue_remove_impl(ddf_fun_t *fun, nic_wv_id_t id)
{
	nic_t *nic_data = nic_get_from_ddf_fun(fun);
	if (nic_data->on_wol_virtue_add == NULL
		|| nic_data->on_wol_virtue_remove == NULL) {
		return ENOTSUP;
	}
	fibril_rwlock_write_lock(&nic_data->wv_lock);
	nic_wol_virtue_t *virtue =
		nic_wol_virtues_remove(&nic_data->wol_virtues, id);
	if (virtue == NULL) {
		fibril_rwlock_write_unlock(&nic_data->wv_lock);
		return ENOENT;
	}
	/* The event handler is called after the filter was removed */
	nic_data->on_wol_virtue_remove(nic_data, virtue);
	fibril_rwlock_write_unlock(&nic_data->wv_lock);
	free(virtue->data);
	free(virtue);
	return EOK;
}

/**
 * Default implementation of the wol_virtue_probe method.
 * Queries the type and data of the virtue.
 *
 * @param[in]	fun
 * @param[in]	id		Virtue identifier
 * @param[out]	type		Type of the virtue. Can be NULL.
 * @param[out]	data		Data used when the virtue was created. Can be NULL.
 * @param[out]	length		Length of the data. Can be NULL.
 *
 * @return EOK		If the operation was successfully completed
 * @return ENOENT	If the virtue identifier is not valid.
 * @return ENOMEM	If there was not enough memory to complete the operation
 */
errno_t nic_wol_virtue_probe_impl(ddf_fun_t *fun, nic_wv_id_t id,
	nic_wv_type_t *type, size_t max_length, void *data, size_t *length)
{
	nic_t *nic_data = nic_get_from_ddf_fun(fun);
	fibril_rwlock_read_lock(&nic_data->wv_lock);
	const nic_wol_virtue_t *virtue =
			nic_wol_virtues_find(&nic_data->wol_virtues, id);
	if (virtue == NULL) {
		*type = NIC_WV_NONE;
		*length = 0;
		fibril_rwlock_read_unlock(&nic_data->wv_lock);
		return ENOENT;
	} else {
		*type = virtue->type;
		if (max_length > virtue->length) {
			max_length = virtue->length;
		}
		memcpy(data, virtue->data, max_length);
		*length = virtue->length;
		fibril_rwlock_read_unlock(&nic_data->wv_lock);
		return EOK;
	}
}

/**
 * Default implementation of the wol_virtue_list method.
 * List filters of the specified type. If NIC_WV_NONE is the type, it lists all
 * filters.
 *
 * @param[in]	fun
 * @param[in]	type	Type of the virtues
 * @param[out]	virtues	Vector of virtue ID's.
 * @param[out]	count	Length of the data. Can be NULL.
 *
 * @return EOK		If the operation was successfully completed
 * @return ENOENT	If the filter identification is not valid.
 * @return ENOMEM	If there was not enough memory to complete the operation
 */
errno_t nic_wol_virtue_list_impl(ddf_fun_t *fun, nic_wv_type_t type,
	size_t max_count, nic_wv_id_t *id_list, size_t *id_count)
{
	nic_t *nic_data = nic_get_from_ddf_fun(fun);
	fibril_rwlock_read_lock(&nic_data->wv_lock);
	errno_t rc = nic_wol_virtues_list(&nic_data->wol_virtues, type,
		max_count, id_list, id_count);
	fibril_rwlock_read_unlock(&nic_data->wv_lock);
	return rc;
}

/**
 * Default implementation of the wol_virtue_get_caps method.
 * Queries for the current capabilities for some type of filter.
 *
 * @param[in]	fun
 * @param[in]	type	Type of the virtues
 * @param[out]	count	Number of virtues of this type that can be currently set
 *
 * @return EOK		If the operation was successfully completed
  */
errno_t nic_wol_virtue_get_caps_impl(ddf_fun_t *fun, nic_wv_type_t type, int *count)
{
	nic_t *nic_data = nic_get_from_ddf_fun(fun);
	fibril_rwlock_read_lock(&nic_data->wv_lock);
	*count = nic_data->wol_virtues.caps_max[type]
	    - (int) nic_data->wol_virtues.lists_sizes[type];
	fibril_rwlock_read_unlock(&nic_data->wv_lock);
	return EOK;
}

/**
 * Default implementation of the poll_get_mode method.
 * Queries the current interrupt/poll mode of the NIC
 *
 * @param[in]	fun
 * @param[out]	mode		Current poll mode
 * @param[out]	period		Period used in periodic polling. Can be NULL.
 *
 * @return EOK		If the operation was successfully completed
 * @return ENOTSUP	This function is not supported.
 * @return EPARTY	Error in communication protocol
 */
errno_t nic_poll_get_mode_impl(ddf_fun_t *fun,
	nic_poll_mode_t *mode, struct timeval *period)
{
	nic_t *nic_data = nic_get_from_ddf_fun(fun);
	fibril_rwlock_read_lock(&nic_data->main_lock);
	*mode = nic_data->poll_mode;
	memcpy(period, &nic_data->poll_period, sizeof (struct timeval));
	fibril_rwlock_read_unlock(&nic_data->main_lock);
	return EOK;
}

/**
 * Default implementation of the poll_set_mode_impl method.
 * Sets the interrupt/poll mode of the NIC.
 *
 * @param[in]	fun
 * @param[in]	mode		The new poll mode
 * @param[in]	period		Period used in periodic polling. Can be NULL.
 *
 * @return EOK		If the operation was successfully completed
 * @return ENOTSUP	This operation is not supported.
 * @return EPARTY	Error in communication protocol
 */
errno_t nic_poll_set_mode_impl(ddf_fun_t *fun,
	nic_poll_mode_t mode, const struct timeval *period)
{
	nic_t *nic_data = nic_get_from_ddf_fun(fun);
	/* If the driver does not implement the poll mode change handler it cannot
	 * switch off interrupts and this is not supported. */
	if (nic_data->on_poll_mode_change == NULL)
		return ENOTSUP;

	if ((mode == NIC_POLL_ON_DEMAND) && nic_data->on_poll_request == NULL)
		return ENOTSUP;

	if (mode == NIC_POLL_PERIODIC || mode == NIC_POLL_SOFTWARE_PERIODIC) {
		if (period == NULL)
			return EINVAL;
		if (period->tv_sec == 0 && period->tv_usec == 0)
			return EINVAL;
		if (period->tv_sec < 0 || period->tv_usec < 0)
			return EINVAL;
	}
	fibril_rwlock_write_lock(&nic_data->main_lock);
	errno_t rc = nic_data->on_poll_mode_change(nic_data, mode, period);
	assert(rc == EOK || rc == ENOTSUP || rc == EINVAL);
	if (rc == ENOTSUP && (nic_data->on_poll_request != NULL) &&
	    (mode == NIC_POLL_PERIODIC || mode == NIC_POLL_SOFTWARE_PERIODIC) ) {

		rc = nic_data->on_poll_mode_change(nic_data, NIC_POLL_ON_DEMAND, NULL);
		assert(rc == EOK || rc == ENOTSUP);
		if (rc == EOK)
			nic_sw_period_start(nic_data);
	}
	if (rc == EOK) {
		nic_data->poll_mode = mode;
		if (period)
			nic_data->poll_period = *period;
	}
	fibril_rwlock_write_unlock(&nic_data->main_lock);
	return rc;
}

/**
 * Default implementation of the poll_now method.
 * Wrapper for the actual poll implementation.
 *
 * @param[in]	fun
 *
 * @return EOK		If the NIC was polled
 * @return ENOTSUP	If the function is not supported
 * @return EINVAL	If the NIC is not in state where it allows on demand polling
 */
errno_t nic_poll_now_impl(ddf_fun_t *fun) {
	nic_t *nic_data = nic_get_from_ddf_fun(fun);
	fibril_rwlock_read_lock(&nic_data->main_lock);
	if (nic_data->poll_mode != NIC_POLL_ON_DEMAND) {
		fibril_rwlock_read_unlock(&nic_data->main_lock);
		return EINVAL;
	}
	if (nic_data->on_poll_request != NULL) {
		nic_data->on_poll_request(nic_data);
		fibril_rwlock_read_unlock(&nic_data->main_lock);
		return EOK;
	} else {
		fibril_rwlock_read_unlock(&nic_data->main_lock);
		return ENOTSUP;
	}
}

/**
 * Default handler for unknown methods (outside of the NIC interface).
 * Logs a warning message and returns ENOTSUP to the caller.
 *
 * @param fun		The DDF function where the method should be called.
 * @param callid	IPC call identifier
 * @param call		IPC call data
 */
void nic_default_handler_impl(ddf_fun_t *fun, ipc_callid_t callid,
    ipc_call_t *call)
{
	async_answer_0(callid, ENOTSUP);
}

/**
 * Default (empty) OPEN function implementation.
 *
 * @param fun	The DDF function
 *
 * @return EOK always.
 */
errno_t nic_open_impl(ddf_fun_t *fun)
{
	return EOK;
}

/**
 * Default (empty) OPEN function implementation.
 *
 * @param fun	The DDF function
 */
void nic_close_impl(ddf_fun_t *fun)
{
}

/** @}
 */
