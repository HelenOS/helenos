/*
 * Copyright (c) 2011 Radim Vansa
 * Copyright (c) 2011 Jiri Michalec
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
 * @brief Internal implementation of general NIC operations
 */

#include <assert.h>
#include <fibril_synch.h>
#include <ns.h>
#include <stdio.h>
#include <str_error.h>
#include <sysinfo.h>
#include <as.h>
#include <ddf/interrupt.h>
#include <ops/nic.h>
#include <errno.h>

#include "nic_driver.h"
#include "nic_ev.h"
#include "nic_impl.h"

#define NIC_GLOBALS_MAX_CACHE_SIZE 16

nic_globals_t nic_globals;

/**
 * Initializes libraries required for NIC framework - logger
 *
 * @param name	Name of the device/driver (used in logging)
 */
errno_t nic_driver_init(const char *name)
{
	list_initialize(&nic_globals.frame_list_cache);
	nic_globals.frame_list_cache_size = 0;
	list_initialize(&nic_globals.frame_cache);
	nic_globals.frame_cache_size = 0;
	fibril_mutex_initialize(&nic_globals.lock);

	char buffer[256];
	snprintf(buffer, 256, "drv/" DEVICE_CATEGORY_NIC "/%s", name);

	return EOK;
}

/** Fill in the default implementations for device options and NIC interface.
 *
 * @param driver_ops
 * @param dev_ops
 * @param iface
 */
void nic_driver_implement(driver_ops_t *driver_ops, ddf_dev_ops_t *dev_ops,
    nic_iface_t *iface)
{
	if (dev_ops) {
		if (!dev_ops->open)
			dev_ops->open = nic_open_impl;
		if (!dev_ops->close)
			dev_ops->close = nic_close_impl;
		if (!dev_ops->interfaces[NIC_DEV_IFACE])
			dev_ops->interfaces[NIC_DEV_IFACE] = iface;
		if (!dev_ops->default_handler)
			dev_ops->default_handler = nic_default_handler_impl;
	}

	if (iface) {
		if (!iface->get_state)
			iface->get_state = nic_get_state_impl;
		if (!iface->set_state)
			iface->set_state = nic_set_state_impl;
		if (!iface->send_frame)
			iface->send_frame = nic_send_frame_impl;
		if (!iface->callback_create)
			iface->callback_create = nic_callback_create_impl;
		if (!iface->get_address)
			iface->get_address = nic_get_address_impl;
		if (!iface->get_stats)
			iface->get_stats = nic_get_stats_impl;
		if (!iface->unicast_get_mode)
			iface->unicast_get_mode = nic_unicast_get_mode_impl;
		if (!iface->unicast_set_mode)
			iface->unicast_set_mode = nic_unicast_set_mode_impl;
		if (!iface->multicast_get_mode)
			iface->multicast_get_mode = nic_multicast_get_mode_impl;
		if (!iface->multicast_set_mode)
			iface->multicast_set_mode = nic_multicast_set_mode_impl;
		if (!iface->broadcast_get_mode)
			iface->broadcast_get_mode = nic_broadcast_get_mode_impl;
		if (!iface->broadcast_set_mode)
			iface->broadcast_set_mode = nic_broadcast_set_mode_impl;
		if (!iface->blocked_sources_get)
			iface->blocked_sources_get = nic_blocked_sources_get_impl;
		if (!iface->blocked_sources_set)
			iface->blocked_sources_set = nic_blocked_sources_set_impl;
		if (!iface->vlan_get_mask)
			iface->vlan_get_mask = nic_vlan_get_mask_impl;
		if (!iface->vlan_set_mask)
			iface->vlan_set_mask = nic_vlan_set_mask_impl;
		if (!iface->wol_virtue_add)
			iface->wol_virtue_add = nic_wol_virtue_add_impl;
		if (!iface->wol_virtue_remove)
			iface->wol_virtue_remove = nic_wol_virtue_remove_impl;
		if (!iface->wol_virtue_probe)
			iface->wol_virtue_probe = nic_wol_virtue_probe_impl;
		if (!iface->wol_virtue_list)
			iface->wol_virtue_list = nic_wol_virtue_list_impl;
		if (!iface->wol_virtue_get_caps)
			iface->wol_virtue_get_caps = nic_wol_virtue_get_caps_impl;
		if (!iface->poll_get_mode)
			iface->poll_get_mode = nic_poll_get_mode_impl;
		if (!iface->poll_set_mode)
			iface->poll_set_mode = nic_poll_set_mode_impl;
		if (!iface->poll_now)
			iface->poll_now = nic_poll_now_impl;
	}
}

/**
 * Setup send frame handler. This MUST be called in the add_device handler
 * if the nic_send_message_impl function is used for sending messages (filled
 * as send_message member of the nic_iface_t structure). The function must not
 * be called anywhere else.
 *
 * @param nic_data
 * @param sffunc	Function handling the send_frame request
 */
void nic_set_send_frame_handler(nic_t *nic_data, send_frame_handler sffunc)
{
	nic_data->send_frame = sffunc;
}

/**
 * Setup event handlers for transitions between driver states.
 * This function can be called only in the add_device handler.
 *
 * @param on_activating	Called when device is going to the ACTIVE state.
 * @param on_going_down Called when device is going to the DOWN state.
 * @param on_stopping	Called when device is going to the STOP state.
 */
void nic_set_state_change_handlers(nic_t *nic_data,
	state_change_handler on_activating, state_change_handler on_going_down,
	state_change_handler on_stopping)
{
	nic_data->on_activating = on_activating;
	nic_data->on_going_down = on_going_down;
	nic_data->on_stopping = on_stopping;
}

/**
 * Setup event handlers for changing the filtering modes.
 * This function can be called only in the add_device handler.
 *
 * @param nic_data
 * @param on_unicast_mode_change
 * @param on_multicast_mode_change
 * @param on_broadcast_mode_change
 * @param on_blocked_sources_change
 * @param on_vlan_mask_change
 */
void nic_set_filtering_change_handlers(nic_t *nic_data,
	unicast_mode_change_handler on_unicast_mode_change,
	multicast_mode_change_handler on_multicast_mode_change,
	broadcast_mode_change_handler on_broadcast_mode_change,
	blocked_sources_change_handler on_blocked_sources_change,
	vlan_mask_change_handler on_vlan_mask_change)
{
	nic_data->on_unicast_mode_change = on_unicast_mode_change;
	nic_data->on_multicast_mode_change = on_multicast_mode_change;
	nic_data->on_broadcast_mode_change = on_broadcast_mode_change;
	nic_data->on_blocked_sources_change = on_blocked_sources_change;
	nic_data->on_vlan_mask_change = on_vlan_mask_change;
}

/**
 * Setup filters for WOL virtues add and removal.
 * This function can be called only in the add_device handler. Both handlers
 * must be set or none of them.
 *
 * @param on_wv_add		Called when a virtue is added
 * @param on_wv_remove	Called when a virtue is removed
 */
void nic_set_wol_virtue_change_handlers(nic_t *nic_data,
	wol_virtue_add_handler on_wv_add, wol_virtue_remove_handler on_wv_remove)
{
	assert(on_wv_add != NULL && on_wv_remove != NULL);
	nic_data->on_wol_virtue_add = on_wv_add;
	nic_data->on_wol_virtue_remove = on_wv_remove;
}

/**
 * Setup poll handlers.
 * This function can be called only in the add_device handler.
 *
 * @param on_poll_mode_change	Called when the mode is about to be changed
 * @param on_poll_request		Called when poll request is triggered
 */
void nic_set_poll_handlers(nic_t *nic_data,
	poll_mode_change_handler on_poll_mode_change, poll_request_handler on_poll_req)
{
	nic_data->on_poll_mode_change = on_poll_mode_change;
	nic_data->on_poll_request = on_poll_req;
}

/**
 * Connect to the parent's driver and get HW resources list in parsed format.
 * Note: this function should be called only from add_device handler, therefore
 * we don't need to use locks.
 *
 * @param		nic_data
 * @param[out]	resources	Parsed lists of resources.
 *
 * @return EOK or an error code
 */
errno_t nic_get_resources(nic_t *nic_data, hw_res_list_parsed_t *resources)
{
	ddf_dev_t *dev = nic_data->dev;
	async_sess_t *parent_sess;

	/* Connect to the parent's driver. */
	parent_sess = ddf_dev_parent_sess_get(dev);
	if (parent_sess == NULL)
		return EIO;

	return hw_res_get_list_parsed(parent_sess, resources, 0);
}

/** Allocate frame
 *
 *  @param nic_data 	The NIC driver data
 *  @param size	        Frame size in bytes
 *  @return pointer to allocated frame if success, NULL otherwise
 */
nic_frame_t *nic_alloc_frame(nic_t *nic_data, size_t size)
{
	nic_frame_t *frame;
	fibril_mutex_lock(&nic_globals.lock);
	if (nic_globals.frame_cache_size > 0) {
		link_t *first = list_first(&nic_globals.frame_cache);
		list_remove(first);
		nic_globals.frame_cache_size--;
		frame = list_get_instance(first, nic_frame_t, link);
		fibril_mutex_unlock(&nic_globals.lock);
	} else {
		fibril_mutex_unlock(&nic_globals.lock);
		frame = malloc(sizeof(nic_frame_t));
		if (!frame)
			return NULL;

		link_initialize(&frame->link);
	}

	frame->data = malloc(size);
	if (frame->data == NULL) {
		free(frame);
		return NULL;
	}

	frame->size = size;
	return frame;
}

/** Release frame
 *
 * @param nic_data	The driver data
 * @param frame		The frame to release
 */
void nic_release_frame(nic_t *nic_data, nic_frame_t *frame)
{
	if (!frame)
		return;

	if (frame->data != NULL) {
		free(frame->data);
		frame->data = NULL;
		frame->size = 0;
	}

	fibril_mutex_lock(&nic_globals.lock);
	if (nic_globals.frame_cache_size >= NIC_GLOBALS_MAX_CACHE_SIZE) {
		fibril_mutex_unlock(&nic_globals.lock);
		free(frame);
	} else {
		list_prepend(&frame->link, &nic_globals.frame_cache);
		nic_globals.frame_cache_size++;
		fibril_mutex_unlock(&nic_globals.lock);
	}
}

/**
 * Allocate a new frame list
 *
 * @return New frame list or NULL on error.
 */
nic_frame_list_t *nic_alloc_frame_list(void)
{
	nic_frame_list_t *frames;
	fibril_mutex_lock(&nic_globals.lock);

	if (nic_globals.frame_list_cache_size > 0) {
		frames =
		    list_get_instance(list_first(&nic_globals.frame_list_cache),
		    nic_frame_list_t, head);
		list_remove(&frames->head);
		list_initialize(frames);
		nic_globals.frame_list_cache_size--;
		fibril_mutex_unlock(&nic_globals.lock);
	} else {
		fibril_mutex_unlock(&nic_globals.lock);

		frames = malloc(sizeof (nic_frame_list_t));
		if (frames != NULL)
			list_initialize(frames);
	}

	return frames;
}

static void nic_driver_release_frame_list(nic_frame_list_t *frames)
{
	if (!frames)
		return;
	fibril_mutex_lock(&nic_globals.lock);
	if (nic_globals.frame_list_cache_size >= NIC_GLOBALS_MAX_CACHE_SIZE) {
		fibril_mutex_unlock(&nic_globals.lock);
		free(frames);
	} else {
		list_prepend(&frames->head, &nic_globals.frame_list_cache);
		nic_globals.frame_list_cache_size++;
		fibril_mutex_unlock(&nic_globals.lock);
	}
}

/**
 * Append a frame to the frame list
 *
 * @param frames	Frame list
 * @param frame		Appended frame
 */
void nic_frame_list_append(nic_frame_list_t *frames,
	nic_frame_t *frame)
{
	assert(frame != NULL && frames != NULL);
	list_append(&frame->link, frames);
}

/** Get the polling mode information from the device
 *
 *	The main lock should be locked, otherwise the inconsistency between
 *	mode and period can occure.
 *
 *  @param nic_data The controller data
 *  @param period [out] The the period. Valid only if mode == NIC_POLL_PERIODIC
 *  @return Current polling mode of the controller
 */
nic_poll_mode_t nic_query_poll_mode(nic_t *nic_data, struct timeval *period)
{
	if (period)
		*period = nic_data->poll_period;
	return nic_data->poll_mode;
}

/** Inform the NICF about poll mode
 *
 *  @param nic_data The controller data
 *  @param mode
 *  @param period [out] The the period. Valid only if mode == NIC_POLL_PERIODIC
 *  @return EOK
 *  @return EINVAL
 */
errno_t nic_report_poll_mode(nic_t *nic_data, nic_poll_mode_t mode,
	struct timeval *period)
{
	errno_t rc = EOK;
	fibril_rwlock_write_lock(&nic_data->main_lock);
	nic_data->poll_mode = mode;
	nic_data->default_poll_mode = mode;
	if (mode == NIC_POLL_PERIODIC) {
		if (period) {
			memcpy(&nic_data->default_poll_period, period, sizeof (struct timeval));
			memcpy(&nic_data->poll_period, period, sizeof (struct timeval));
		} else {
			rc = EINVAL;
		}
	}
	fibril_rwlock_write_unlock(&nic_data->main_lock);
	return rc;
}

/** Inform the NICF about device's MAC adress.
 *
 * @return EOK On success
 *
 */
errno_t nic_report_address(nic_t *nic_data, const nic_address_t *address)
{
	assert(nic_data);

	if (address->address[0] & 1)
		return EINVAL;

	fibril_rwlock_write_lock(&nic_data->main_lock);

	/* Notify NIL layer (and uppper) if bound - not in add_device */
	if (nic_data->client_session != NULL) {
		errno_t rc = nic_ev_addr_changed(nic_data->client_session,
		    address);

		if (rc != EOK) {
			fibril_rwlock_write_unlock(&nic_data->main_lock);
			return rc;
		}
	}

	fibril_rwlock_write_lock(&nic_data->rxc_lock);

	/*
	 * The initial address (all zeroes) shouldn't be
	 * there and we will ignore that error -- in next
	 * calls this should not happen.
	 */
	errno_t rc = nic_rxc_set_addr(&nic_data->rx_control,
	    &nic_data->mac, address);

	/* For the first time also record the default MAC */
	if (MAC_IS_ZERO(nic_data->default_mac.address)) {
		assert(MAC_IS_ZERO(nic_data->mac.address));
		memcpy(&nic_data->default_mac, address, sizeof(nic_address_t));
	}

	fibril_rwlock_write_unlock(&nic_data->rxc_lock);

	if ((rc != EOK) && (rc != ENOENT)) {
		fibril_rwlock_write_unlock(&nic_data->main_lock);
		return rc;
	}

	memcpy(&nic_data->mac, address, sizeof(nic_address_t));

	fibril_rwlock_write_unlock(&nic_data->main_lock);

	return EOK;
}

/**
 * Used to obtain devices MAC address.
 *
 * The main lock should be locked, otherwise the inconsistent address
 * can be returend.
 *
 * @param nic_data The controller data
 * @param address The output for address.
 */
void nic_query_address(nic_t *nic_data, nic_address_t *addr) {
	if (!addr)
		return;

	memcpy(addr, &nic_data->mac, sizeof(nic_address_t));
}

/**
 * The busy flag can be set to 1 only in the send_frame handler, to 0 it can
 * be set anywhere.
 *
 * @param nic_data
 * @param busy
 */
void nic_set_tx_busy(nic_t *nic_data, int busy)
{
	/*
	 * When the function is called in send_frame handler the main lock is
	 * locked so no race can happen.
	 * Otherwise, when it is unexpectedly set to 0 (even with main lock held
	 * by other fibril) it cannot crash anything.
	 */
	nic_data->tx_busy = busy;
}

/**
 * This is the function that the driver should call when it receives a frame.
 * The frame is checked by filters and then sent up to the NIL layer or
 * discarded. The frame is released.
 *
 * @param nic_data
 * @param frame		The received frame
 */
void nic_received_frame(nic_t *nic_data, nic_frame_t *frame)
{
	/* Note: this function must not lock main lock, because loopback driver
	 * 		 calls it inside send_frame handler (with locked main lock) */
	fibril_rwlock_read_lock(&nic_data->rxc_lock);
	nic_frame_type_t frame_type;
	bool check = nic_rxc_check(&nic_data->rx_control, frame->data,
	    frame->size, &frame_type);
	fibril_rwlock_read_unlock(&nic_data->rxc_lock);
	/* Update statistics */
	fibril_rwlock_write_lock(&nic_data->stats_lock);

	if (nic_data->state == NIC_STATE_ACTIVE && check) {
		nic_data->stats.receive_packets++;
		nic_data->stats.receive_bytes += frame->size;
		switch (frame_type) {
		case NIC_FRAME_MULTICAST:
			nic_data->stats.receive_multicast++;
			break;
		case NIC_FRAME_BROADCAST:
			nic_data->stats.receive_broadcast++;
			break;
		default:
			break;
		}
		fibril_rwlock_write_unlock(&nic_data->stats_lock);
		nic_ev_received(nic_data->client_session, frame->data,
		    frame->size);
	} else {
		switch (frame_type) {
		case NIC_FRAME_UNICAST:
			nic_data->stats.receive_filtered_unicast++;
			break;
		case NIC_FRAME_MULTICAST:
			nic_data->stats.receive_filtered_multicast++;
			break;
		case NIC_FRAME_BROADCAST:
			nic_data->stats.receive_filtered_broadcast++;
			break;
		}
		fibril_rwlock_write_unlock(&nic_data->stats_lock);
	}
	nic_release_frame(nic_data, frame);
}

/**
 * Some NICs can receive multiple frames during single interrupt. These can
 * send them in whole list of frames (actually nic_frame_t structures), then
 * the list is deallocated and each frame is passed to the
 * nic_received_packet function.
 *
 * @param nic_data
 * @param frames		List of received frames
 */
void nic_received_frame_list(nic_t *nic_data, nic_frame_list_t *frames)
{
	if (frames == NULL)
		return;
	while (!list_empty(frames)) {
		nic_frame_t *frame =
			list_get_instance(list_first(frames), nic_frame_t, link);

		list_remove(&frame->link);
		nic_received_frame(nic_data, frame);
	}
	nic_driver_release_frame_list(frames);
}

/** Allocate and initialize the driver data.
 *
 * @return Allocated structure or NULL.
 *
 */
static nic_t *nic_create(ddf_dev_t *dev)
{
	nic_t *nic_data = ddf_dev_data_alloc(dev, sizeof(nic_t));
	if (nic_data == NULL)
		return NULL;

	/* Force zero to all uninitialized fields (e.g. added in future) */
	if (nic_rxc_init(&nic_data->rx_control) != EOK) {
		return NULL;
	}

	if (nic_wol_virtues_init(&nic_data->wol_virtues) != EOK) {
		return NULL;
	}

	nic_data->dev = NULL;
	nic_data->fun = NULL;
	nic_data->state = NIC_STATE_STOPPED;
	nic_data->client_session = NULL;
	nic_data->poll_mode = NIC_POLL_IMMEDIATE;
	nic_data->default_poll_mode = NIC_POLL_IMMEDIATE;
	nic_data->send_frame = NULL;
	nic_data->on_activating = NULL;
	nic_data->on_going_down = NULL;
	nic_data->on_stopping = NULL;
	nic_data->specific = NULL;

	fibril_rwlock_initialize(&nic_data->main_lock);
	fibril_rwlock_initialize(&nic_data->stats_lock);
	fibril_rwlock_initialize(&nic_data->rxc_lock);
	fibril_rwlock_initialize(&nic_data->wv_lock);

	memset(&nic_data->mac, 0, sizeof(nic_address_t));
	memset(&nic_data->default_mac, 0, sizeof(nic_address_t));
	memset(&nic_data->stats, 0, sizeof(nic_device_stats_t));

	return nic_data;
}

/** Create NIC structure for the device and bind it to dev_fun_t
 *
 * The pointer to the created and initialized NIC structure will
 * be stored in device->nic_data.
 *
 * @param device The NIC device structure
 *
 * @return Pointer to created nic_t structure or NULL
 *
 */
nic_t *nic_create_and_bind(ddf_dev_t *device)
{
	nic_t *nic_data = nic_create(device);
	if (!nic_data)
		return NULL;

	nic_data->dev = device;

	return nic_data;
}

/**
 * Hangs up the phones in the structure, deallocates specific data and then
 * the structure itself.
 *
 * @param data
 */
static void nic_destroy(nic_t *nic_data)
{
	free(nic_data->specific);
}

/**
 * Unbind and destroy nic_t stored in ddf_dev_t.nic_data.
 * The ddf_dev_t.nic_data will be set to NULL, specific driver data will be
 * destroyed.
 *
 * @param device The NIC device structure
 */
void nic_unbind_and_destroy(ddf_dev_t *device)
{
	nic_destroy(nic_get_from_ddf_dev(device));
	return;
}

/**
 * Set information about current HW filtering.
 *  1 ...	Only those frames we want to receive are passed through HW
 *  0 ...	The HW filtering is imperfect
 * -1 ...	Don't change the setting
 * Can be called only from the on_*_change handler.
 *
 * @param	nic_data
 * @param	unicast_exact	Unicast frames
 * @param	mcast_exact		Multicast frames
 * @param	vlan_exact		VLAN tags
 */
void nic_report_hw_filtering(nic_t *nic_data,
	int unicast_exact, int multicast_exact, int vlan_exact)
{
	nic_rxc_hw_filtering(&nic_data->rx_control,
		unicast_exact, multicast_exact, vlan_exact);
}

/**
 * Computes hash for the address list based on standard multicast address
 * hashing.
 *
 * @param address_list
 * @param count
 *
 * @return Multicast hash
 *
 * @see multicast_hash
 */
uint64_t nic_mcast_hash(const nic_address_t *list, size_t count)
{
	return nic_rxc_mcast_hash(list, count);
}

/**
 * Computes hash for multicast addresses currently set up in the RX multicast
 * filtering. For promiscuous mode returns all ones, for blocking all zeroes.
 * Can be called only from the state change handlers (on_activating,
 * on_going_down and on_stopping).
 *
 * @param nic_data
 *
 * @return Multicast hash
 *
 * @see multicast_hash
 */
uint64_t nic_query_mcast_hash(nic_t *nic_data)
{
	fibril_rwlock_read_lock(&nic_data->rxc_lock);
	uint64_t hash = nic_rxc_multicast_get_hash(&nic_data->rx_control);
	fibril_rwlock_read_unlock(&nic_data->rxc_lock);
	return hash;
}

/**
 * Queries the current mode of unicast frames receiving.
 * Can be called only from the on_*_change handler.
 *
 * @param nic_data
 * @param mode			The new unicast mode
 * @param max_count		Max number of addresses that can be written into the
 * 						address_list.
 * @param address_list	List of MAC addresses or NULL.
 * @param address_count Number of addresses in the list
 */
void nic_query_unicast(const nic_t *nic_data,
	nic_unicast_mode_t *mode,
	size_t max_count, nic_address_t *address_list, size_t *address_count)
{
	assert(mode != NULL);
	nic_rxc_unicast_get_mode(&nic_data->rx_control, mode,
		max_count, address_list, address_count);
}

/**
 * Queries the current mode of multicast frames receiving.
 * Can be called only from the on_*_change handler.
 *
 * @param nic_data
 * @param mode			The current multicast mode
 * @param max_count		Max number of addresses that can be written into the
 * 						address_list.
 * @param address_list	List of MAC addresses or NULL.
 * @param address_count Number of addresses in the list
 */
void nic_query_multicast(const nic_t *nic_data,
	nic_multicast_mode_t *mode,
	size_t max_count, nic_address_t *address_list, size_t *address_count)
{
	assert(mode != NULL);
	nic_rxc_multicast_get_mode(&nic_data->rx_control, mode,
		max_count, address_list, address_count);
}

/**
 * Queries the current mode of broadcast frames receiving.
 * Can be called only from the on_*_change handler.
 *
 * @param nic_data
 * @param mode			The new broadcast mode
 */
void nic_query_broadcast(const nic_t *nic_data,
	nic_broadcast_mode_t *mode)
{
	assert(mode != NULL);
	nic_rxc_broadcast_get_mode(&nic_data->rx_control, mode);
}

/**
 * Queries the current blocked source addresses.
 * Can be called only from the on_*_change handler.
 *
 * @param nic_data
 * @param max_count		Max number of addresses that can be written into the
 * 						address_list.
 * @param address_list	List of MAC addresses or NULL.
 * @param address_count Number of addresses in the list
 */
void nic_query_blocked_sources(const nic_t *nic_data,
	size_t max_count, nic_address_t *address_list, size_t *address_count)
{
	nic_rxc_blocked_sources_get(&nic_data->rx_control,
		max_count, address_list, address_count);
}

/**
 * Query mask used for filtering according to the VLAN tags.
 * Can be called only from the on_*_change handler.
 *
 * @param nic_data
 * @param mask		Must be 512 bytes long
 *
 * @return EOK
 * @return ENOENT
 */
errno_t nic_query_vlan_mask(const nic_t *nic_data, nic_vlan_mask_t *mask)
{
	assert(mask);
	return nic_rxc_vlan_get_mask(&nic_data->rx_control, mask);
}

/**
 * Query maximum number of WOL virtues of specified type allowed on the device.
 * Can be called only from add_device and on_wol_virtue_* handlers.
 *
 * @param nic_data
 * @param type		The type of the WOL virtues
 *
 * @return	Maximal number of allowed virtues of this type. -1 means this type
 * 			is not supported at all.
 */
int nic_query_wol_max_caps(const nic_t *nic_data, nic_wv_type_t type)
{
	return nic_data->wol_virtues.caps_max[type];
}

/**
 * Sets maximum number of WOL virtues of specified type allowed on the device.
 * Can be called only from add_device and on_wol_virtue_* handlers.
 *
 * @param nic_data
 * @param type		The type of the WOL virtues
 * @param count		Maximal number of allowed virtues of this type. -1 means
 * 					this type is not supported at all.
 */
void nic_set_wol_max_caps(nic_t *nic_data, nic_wv_type_t type, int count)
{
	nic_data->wol_virtues.caps_max[type] = count;
}

/**
 * @param nic_data
 * @return The driver-specific structure for this NIC.
 */
void *nic_get_specific(nic_t *nic_data)
{
	return nic_data->specific;
}

/**
 * @param nic_data
 * @param specific The driver-specific structure for this NIC.
 */
void nic_set_specific(nic_t *nic_data, void *specific)
{
	nic_data->specific = specific;
}

/**
 * You can call the function only from one of the state change handlers.
 * @param	nic_data
 * @return	Current state of the NIC, prior to the actually executed change
 */
nic_device_state_t nic_query_state(nic_t *nic_data)
{
	return nic_data->state;
}

/**
 * @param nic_data
 * @return DDF device associated with this NIC.
 */
ddf_dev_t *nic_get_ddf_dev(nic_t *nic_data)
{
	return nic_data->dev;
}

/**
 * @param nic_data
 * @return DDF function associated with this NIC.
 */
ddf_fun_t *nic_get_ddf_fun(nic_t *nic_data)
{
	return nic_data->fun;
}

/**
 * @param nic_data
 * @param fun
 */
void nic_set_ddf_fun(nic_t *nic_data, ddf_fun_t *fun)
{
	nic_data->fun = fun;
}

/**
 * @param dev DDF device associated with NIC
 * @return The associated NIC structure
 */
nic_t *nic_get_from_ddf_dev(ddf_dev_t *dev)
{
	return (nic_t *) ddf_dev_data_get(dev);
}

/**
 * @param dev DDF function associated with NIC
 * @return The associated NIC structure
 */
nic_t *nic_get_from_ddf_fun(ddf_fun_t *fun)
{
	return (nic_t *) ddf_dev_data_get(ddf_fun_get_dev(fun));
}

/**
 * Raises the send_packets and send_bytes in device statistics.
 *
 * @param nic_data
 * @param packets	Number of received packets
 * @param bytes		Number of received bytes
 */
void nic_report_send_ok(nic_t *nic_data, size_t packets, size_t bytes)
{
	fibril_rwlock_write_lock(&nic_data->stats_lock);
	nic_data->stats.send_packets += packets;
	nic_data->stats.send_bytes += bytes;
	fibril_rwlock_write_unlock(&nic_data->stats_lock);
}

/**
 * Raises total error counter (send_errors) and the concrete send error counter
 * determined by the cause argument.
 *
 * @param nic_data
 * @param cause		The concrete error cause.
 */
void nic_report_send_error(nic_t *nic_data, nic_send_error_cause_t cause,
    unsigned count)
{
	if (count == 0)
		return;

	fibril_rwlock_write_lock(&nic_data->stats_lock);
	nic_data->stats.send_errors += count;
	switch (cause) {
	case NIC_SEC_BUFFER_FULL:
		nic_data->stats.send_dropped += count;
		break;
	case NIC_SEC_ABORTED:
		nic_data->stats.send_aborted_errors += count;
		break;
	case NIC_SEC_CARRIER_LOST:
		nic_data->stats.send_carrier_errors += count;
		break;
	case NIC_SEC_FIFO_OVERRUN:
		nic_data->stats.send_fifo_errors += count;
		break;
	case NIC_SEC_HEARTBEAT:
		nic_data->stats.send_heartbeat_errors += count;
		break;
	case NIC_SEC_WINDOW_ERROR:
		nic_data->stats.send_window_errors += count;
		break;
	case NIC_SEC_OTHER:
		break;
	}
	fibril_rwlock_write_unlock(&nic_data->stats_lock);
}

/**
 * Raises total error counter (receive_errors) and the concrete receive error
 * counter determined by the cause argument.
 *
 * @param nic_data
 * @param cause		The concrete error cause
 */
void nic_report_receive_error(nic_t *nic_data,
	nic_receive_error_cause_t cause, unsigned count)
{
	fibril_rwlock_write_lock(&nic_data->stats_lock);
	nic_data->stats.receive_errors += count;
	switch (cause) {
	case NIC_REC_BUFFER_FULL:
		nic_data->stats.receive_dropped += count;
		break;
	case NIC_REC_LENGTH:
		nic_data->stats.receive_length_errors += count;
		break;
	case NIC_REC_BUFFER_OVERFLOW:
		nic_data->stats.receive_dropped += count;
		break;
	case NIC_REC_CRC:
		nic_data->stats.receive_crc_errors += count;
		break;
	case NIC_REC_FRAME_ALIGNMENT:
		nic_data->stats.receive_frame_errors += count;
		break;
	case NIC_REC_FIFO_OVERRUN:
		nic_data->stats.receive_fifo_errors += count;
		break;
	case NIC_REC_MISSED:
		nic_data->stats.receive_missed_errors += count;
		break;
	case NIC_REC_OTHER:
		break;
	}
	fibril_rwlock_write_unlock(&nic_data->stats_lock);
}

/**
 * Raises the collisions counter in device statistics.
 */
void nic_report_collisions(nic_t *nic_data, unsigned count)
{
	fibril_rwlock_write_lock(&nic_data->stats_lock);
	nic_data->stats.collisions += count;
	fibril_rwlock_write_unlock(&nic_data->stats_lock);
}

/** Just wrapper for checking nonzero time interval
 *
 *  @oaram t The interval to check
 *  @returns Zero if the t is nonzero interval
 *  @returns Nonzero if t is zero interval
 */
static int timeval_nonpositive(struct timeval t) {
	return (t.tv_sec <= 0) && (t.tv_usec <= 0);
}

/** Main function of software period fibrill
 *
 *  Just calls poll() in the nic->poll_period period
 *
 *  @param  data The NIC structure pointer
 *
 *  @return 0, never reached
 */
static errno_t period_fibril_fun(void *data)
{
	nic_t *nic = data;
	struct sw_poll_info *info = &nic->sw_poll_info;
	while (true) {
		fibril_rwlock_read_lock(&nic->main_lock);
		int run = info->run;
		int running = info->running;
		struct timeval remaining = nic->poll_period;
		fibril_rwlock_read_unlock(&nic->main_lock);

		if (!running) {
			remaining.tv_sec = 5;
			remaining.tv_usec = 0;
		}

		/* Wait the period (keep attention to overflows) */
		while (!timeval_nonpositive(remaining)) {
			suseconds_t wait = 0;
			if (remaining.tv_sec > 0) {
				time_t wait_sec = remaining.tv_sec;
				/* wait maximaly 5 seconds to get reasonable reaction time
				 * when period is reset
				 */
				if (wait_sec > 5)
					wait_sec = 5;

				wait = (suseconds_t) wait_sec * 1000000;

				remaining.tv_sec -= wait_sec;
			} else {
				wait = remaining.tv_usec;

				if (wait > 5 * 1000000) {
					wait = 5 * 1000000;
				}

				remaining.tv_usec -= wait;
			}
			async_usleep(wait);

			/* Check if the period was not reset */
			if (info->run != run)
				break;
		}

		/* Provide polling if the period finished */
		fibril_rwlock_read_lock(&nic->main_lock);
		if (info->running && info->run == run) {
			nic->on_poll_request(nic);
		}
		fibril_rwlock_read_unlock(&nic->main_lock);
	}
	return EOK;
}

/** Starts software periodic polling
 *
 *  Reset to new period if the original period was running
 *
 *  @param nic_data Nic data structure
 */
void nic_sw_period_start(nic_t *nic_data)
{
	/* Create the fibril if it is not crated */
	if (nic_data->sw_poll_info.fibril == 0) {
		nic_data->sw_poll_info.fibril = fibril_create(period_fibril_fun,
		    nic_data);
		nic_data->sw_poll_info.running = 0;
		nic_data->sw_poll_info.run = 0;

		/* Start fibril */
		fibril_add_ready(nic_data->sw_poll_info.fibril);
	}

	/* Inform fibril about running with new period */
	nic_data->sw_poll_info.run = (nic_data->sw_poll_info.run + 1) % 100;
	nic_data->sw_poll_info.running = 1;
}

/** Stops software periodic polling
 *
 *  @param nic_data Nic data structure
 */
void nic_sw_period_stop(nic_t *nic_data)
{
	nic_data->sw_poll_info.running = 0;
}

/** @}
 */
