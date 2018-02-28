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
 * @brief Internal NICF structures
 */

#ifndef __NIC_DRIVER_H__
#define __NIC_DRIVER_H__

#ifndef LIBNIC_INTERNAL
#error "This is internal libnic's header, please do not include it"
#endif

#include <fibril_synch.h>
#include <nic/nic.h>
#include <async.h>

#include "nic.h"
#include "nic_rx_control.h"
#include "nic_wol_virtues.h"

struct sw_poll_info {
	fid_t fibril;
	volatile int run;
	volatile int running;
};

struct nic {
	/**
	 * Device from device manager's point of view.
	 * The value must be set within the add_device handler
	 * (in nic_create_and_bind) and must not be changed.
	 */
	ddf_dev_t *dev;
	/**
	 * Device's NIC function.
	 * The value must be set within the add_device handler
	 * (in nic_driver_register_as_ddf_function) and must not be changed.
	 */
	ddf_fun_t *fun;
	/** Current state of the device */
	nic_device_state_t state;
	/** Transmiter is busy - messages are dropped */
	int tx_busy;
	/** Device's MAC address */
	nic_address_t mac;
	/** Device's default MAC address (assigned the first time, used in STOP) */
	nic_address_t default_mac;
	/** Client callback session */
	async_sess_t *client_session;
	/** Current polling mode of the NIC */
	nic_poll_mode_t poll_mode;
	/** Polling period (applicable when poll_mode == NIC_POLL_PERIODIC) */
	struct timeval poll_period;
	/** Current polling mode of the NIC */
	nic_poll_mode_t default_poll_mode;
	/** Polling period (applicable when default_poll_mode == NIC_POLL_PERIODIC) */
	struct timeval default_poll_period;
	/** Software period fibrill information */
	struct sw_poll_info sw_poll_info;
	/**
	 * Lock on everything but statistics, rx control and wol virtues. This lock
	 * cannot be used if filters_lock or stats_lock is already held - you must
	 * acquire main_lock first (otherwise deadlock could happen).
	 */
	fibril_rwlock_t main_lock;
	/** Device statistics */
	nic_device_stats_t stats;
	/**
	 * Lock for statistics. You must not hold any other lock from nic_t except
	 * the main_lock at the same moment. If both this lock and main_lock should
	 * be locked, the main_lock must be locked as the first.
	 */
	fibril_rwlock_t stats_lock;
	/** Receive control configuration */
	nic_rxc_t rx_control;
	/**
	 * Lock for receive control. You must not hold any other lock from nic_t
	 * except the main_lock at the same moment. If both this lock and main_lock
	 * should be locked, the main_lock must be locked as the first.
	 */
	fibril_rwlock_t rxc_lock;
	/** WOL virtues configuration */
	nic_wol_virtues_t wol_virtues;
	/**
	 * Lock for WOL virtues. You must not hold any other lock from nic_t
	 * except the main_lock at the same moment. If both this lock and main_lock
	 * should be locked, the main_lock must be locked as the first.
	 */
	fibril_rwlock_t wv_lock;
	/**
	 * Function really sending the data. This MUST be filled in if the
	 * nic_send_message_impl function is used for sending messages (filled
	 * as send_message member of the nic_iface_t structure).
	 * Called with the main_lock locked for reading.
	 */
	send_frame_handler send_frame;
	/**
	 * Event handler called when device goes to the ACTIVE state.
	 * The implementation is optional.
	 * Called with the main_lock locked for writing.
	 */
	state_change_handler on_activating;
	/**
	 * Event handler called when device goes to the DOWN state.
	 * The implementation is optional.
	 * Called with the main_lock locked for writing.
	 */
	state_change_handler on_going_down;
	/**
	 * Event handler called when device goes to the STOPPED state.
	 * The implementation is optional.
	 * Called with the main_lock locked for writing.
	 */
	state_change_handler on_stopping;
	/**
	 * Event handler called when the unicast receive mode is changed.
	 * The implementation is optional. Called with rxc_lock locked for writing.
	 */
	unicast_mode_change_handler on_unicast_mode_change;
	/**
	 * Event handler called when the multicast receive mode is changed.
	 * The implementation is optional. Called with rxc_lock locked for writing.
	 */
	multicast_mode_change_handler on_multicast_mode_change;
	/**
	 * Event handler called when the broadcast receive mode is changed.
	 * The implementation is optional. Called with rxc_lock locked for writing.
	 */
	broadcast_mode_change_handler on_broadcast_mode_change;
	/**
	 * Event handler called when the blocked sources set is changed.
	 * The implementation is optional. Called with rxc_lock locked for writing.
	 */
	blocked_sources_change_handler on_blocked_sources_change;
	/**
	 * Event handler called when the VLAN mask is changed.
	 * The implementation is optional. Called with rxc_lock locked for writing.
	 */
	vlan_mask_change_handler on_vlan_mask_change;
	/**
	 * Event handler called when a new WOL virtue is added.
	 * The implementation is optional.
	 * Called with filters_lock locked for writing.
	 */
	wol_virtue_add_handler on_wol_virtue_add;
	/**
	 * Event handler called when a WOL virtue is removed.
	 * The implementation is optional.
	 * Called with filters_lock locked for writing.
	 */
	wol_virtue_remove_handler on_wol_virtue_remove;
	/**
	 * Event handler called when the polling mode is changed.
	 * The implementation is optional.
	 * Called with main_lock locked for writing.
	 */
	poll_mode_change_handler on_poll_mode_change;
	/**
	 * Event handler called when the NIC should poll its buffers for a new frame
	 * (in NIC_POLL_PERIODIC or NIC_POLL_ON_DEMAND) modes.
	 * Called with the main_lock locked for reading.
	 * The implementation is optional.
	 */
	poll_request_handler on_poll_request;
	/** Data specific for particular driver */
	void *specific;
};

/**
 * Structure keeping global data
 */
typedef struct nic_globals {
	list_t frame_list_cache;
	size_t frame_list_cache_size;
	list_t frame_cache;
	size_t frame_cache_size;
	fibril_mutex_t lock;
} nic_globals_t;

#endif

/** @}
 */
