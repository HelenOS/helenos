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
 * @brief Public header exposing NICF to drivers
 */

#ifndef __NIC_H__
#define __NIC_H__

#include <adt/list.h>
#include <adt/hash_table.h>
#include <ddf/driver.h>
#include <device/hw_res_parsed.h>
#include <ops/nic.h>

#define DEVICE_CATEGORY_NIC "nic"

struct nic;
typedef struct nic nic_t;

/**
 * Single WOL virtue descriptor.
 */
typedef struct nic_wol_virtue {
	ht_link_t item;
	nic_wv_id_t id;
	nic_wv_type_t type;
	void *data;
	size_t length;
	struct nic_wol_virtue *next;
} nic_wol_virtue_t;

/**
 * Simple structure for sending lists of frames.
 */
typedef struct {
	link_t link;
	void *data;
	size_t size;
} nic_frame_t;

typedef list_t nic_frame_list_t;

/**
 * Handler for writing frame data to the NIC device.
 * The function is responsible for releasing the frame.
 * It does not return anything, if some error is detected the function just
 * silently fails (logging on debug level is suggested).
 *
 * @param nic_data
 * @param data		Pointer to frame data
 * @param size		Size of frame data in bytes
 */
typedef void (*send_frame_handler)(nic_t *, void *, size_t);

/**
 * The handler for transitions between driver states.
 * If the handler returns error code, the transition between
 * states is canceled (the state is not changed).
 *
 * @param nic_data	NICF main structure
 *
 * @return EOK	If everything is all right.
 * @return error code on error.
 */
typedef errno_t (*state_change_handler)(nic_t *);

/**
 * Handler for unicast filtering mode change.
 *
 * @param nic_data		NICF main structure
 * @param new_mode		The mode that should be set up
 * @param address_list	Addresses as argument to the mode
 * @param address_count	Number of addresses in the list
 *
 * @return EOK		If the mode is set up
 * @return ENOTSUP	If this mode is not supported
 */
typedef errno_t (*unicast_mode_change_handler)(nic_t *,
    nic_unicast_mode_t, const nic_address_t *, size_t);

/**
 * Handler for multicast filtering mode change.
 *
 * @param nic_data		NICF main structure
 * @param new_mode		The mode that should be set up
 * @param address_list	Addresses as argument to the mode
 * @param address_count	Number of addresses in the list
 *
 * @return EOK		If the mode is set up
 * @return ENOTSUP	If this mode is not supported
 */
typedef errno_t (*multicast_mode_change_handler)(nic_t *,
    nic_multicast_mode_t, const nic_address_t *, size_t);

/**
 * Handler for broadcast filtering mode change.
 *
 * @param nic_data	NICF main structure
 * @param new_mode	The mode that should be set up
 *
 * @return EOK		If the mode is set up
 * @return ENOTSUP	If this mode is not supported
 */
typedef errno_t (*broadcast_mode_change_handler)(nic_t *, nic_broadcast_mode_t);

/**
 * Handler for blocked sources list change.
 *
 * @param nic_data		NICF main structure
 * @param address_list	Addresses as argument to the mode
 * @param address_count	Number of addresses in the list
 */
typedef void (*blocked_sources_change_handler)(nic_t *,
    const nic_address_t *, size_t);

/**
 * Handler for VLAN filtering mask change.
 * @param nic_data		NICF main structure
 * @param vlan_mask		The new mask | NULL for disabling vlan filter
 */
typedef void (*vlan_mask_change_handler)(nic_t *, const nic_vlan_mask_t *);

/**
 * Handler called when a WOL virtue is added.
 * If the maximum of accepted WOL virtues changes due to adding this virtue
 * you should update the vector wol_virtues.caps_max.
 * The driver is allowed to store pointer to the virtue data until
 * on_wol_virtue_remove on these data is called (although probably this is
 * not a good practice).
 *
 * @param nic_data	NICF main structure
 * @param virtue	Structure with virtue description
 *
 * @return EOK		If the filter can be used. Software emulation of the
 * 					filter is activated unless the emulate is set to 0.
 * @return ENOTSUP	If this filter cannot work on this NIC (e.g. the NIC
 * 					cannot run in promiscuous node or the limit of WOL
 * 					frames' specifications was reached).
 * @return ELIMIT	If this filter must implemented in HW but currently the
 * 					limit of these HW filters was reached.
 */
typedef errno_t (*wol_virtue_add_handler)(nic_t *, const nic_wol_virtue_t *);

/**
 * Handler called when a WOL virtue is removed.
 * If the maximum of accepted WOL virtues changes due to removing this
 * virtue you should update the vector wol_virtues.caps_max.
 *
 * @param nic_data	NICF main structure
 * @param virtue		Structure with virtue description
 */
typedef void (*wol_virtue_remove_handler)(nic_t *, const nic_wol_virtue_t *);

/**
 * Handler for poll mode change.
 *
 * @param nic_data	NICF main structure
 * @param mode		Mode to be set up
 * @param period	New period of polling (with NIC_POLL_PERIODIC)
 *
 * @return EOK		If the mode was fully setup
 * @return ENOTSUP	If NICF should do the periodic polling
 * @return EINVAL	If this mode cannot be set up under no circumstances
 */
typedef errno_t (*poll_mode_change_handler)(nic_t *,
    nic_poll_mode_t, const struct timeval *);

/**
 * Event handler called when the NIC should poll its buffers for a new frame
 * (in NIC_POLL_PERIODIC or NIC_POLL_ON_DEMAND) modes.
 *
 * @param nic_data	NICF main structure
 */
typedef void (*poll_request_handler)(nic_t *);

/* nic_t allocation and deallocation */
extern nic_t *nic_create_and_bind(ddf_dev_t *);
extern void nic_unbind_and_destroy(ddf_dev_t *);

/* Functions called in the main function */
extern errno_t nic_driver_init(const char *);
extern void nic_driver_implement(driver_ops_t *, ddf_dev_ops_t *,
    nic_iface_t *);

/* Functions called in add_device */
extern errno_t nic_get_resources(nic_t *, hw_res_list_parsed_t *);
extern void nic_set_specific(nic_t *, void *);
extern void nic_set_send_frame_handler(nic_t *, send_frame_handler);
extern void nic_set_state_change_handlers(nic_t *,
    state_change_handler, state_change_handler, state_change_handler);
extern void nic_set_filtering_change_handlers(nic_t *,
    unicast_mode_change_handler, multicast_mode_change_handler,
    broadcast_mode_change_handler, blocked_sources_change_handler,
    vlan_mask_change_handler);
extern void nic_set_wol_virtue_change_handlers(nic_t *,
    wol_virtue_add_handler, wol_virtue_remove_handler);
extern void nic_set_poll_handlers(nic_t *,
    poll_mode_change_handler, poll_request_handler);

/* General driver functions */
extern ddf_dev_t *nic_get_ddf_dev(nic_t *);
extern ddf_fun_t *nic_get_ddf_fun(nic_t *);
extern void nic_set_ddf_fun(nic_t *, ddf_fun_t *);
extern nic_t *nic_get_from_ddf_dev(ddf_dev_t *);
extern nic_t *nic_get_from_ddf_fun(ddf_fun_t *);
extern void *nic_get_specific(nic_t *);
extern nic_device_state_t nic_query_state(nic_t *);
extern void nic_set_tx_busy(nic_t *, int);
extern errno_t nic_report_address(nic_t *, const nic_address_t *);
extern errno_t nic_report_poll_mode(nic_t *, nic_poll_mode_t, struct timeval *);
extern void nic_query_address(nic_t *, nic_address_t *);
extern void nic_received_frame(nic_t *, nic_frame_t *);
extern void nic_received_frame_list(nic_t *, nic_frame_list_t *);
extern nic_poll_mode_t nic_query_poll_mode(nic_t *, struct timeval *);

/* Statistics updates */
extern void nic_report_send_ok(nic_t *, size_t, size_t);
extern void nic_report_send_error(nic_t *, nic_send_error_cause_t, unsigned);
extern void nic_report_receive_error(nic_t *, nic_receive_error_cause_t,
    unsigned);
extern void nic_report_collisions(nic_t *, unsigned);

/* Frame / frame list allocation and deallocation */
extern nic_frame_t *nic_alloc_frame(nic_t *, size_t);
extern nic_frame_list_t *nic_alloc_frame_list(void);
extern void nic_frame_list_append(nic_frame_list_t *, nic_frame_t *);
extern void nic_release_frame(nic_t *, nic_frame_t *);

/* RXC query and report functions */
extern void nic_report_hw_filtering(nic_t *, int, int, int);
extern void nic_query_unicast(const nic_t *,
    nic_unicast_mode_t *, size_t, nic_address_t *, size_t *);
extern void nic_query_multicast(const nic_t *,
    nic_multicast_mode_t *, size_t, nic_address_t *, size_t *);
extern void nic_query_broadcast(const nic_t *, nic_broadcast_mode_t *);
extern void nic_query_blocked_sources(const nic_t *,
    size_t, nic_address_t *, size_t *);
extern errno_t nic_query_vlan_mask(const nic_t *, nic_vlan_mask_t *);
extern int nic_query_wol_max_caps(const nic_t *, nic_wv_type_t);
extern void nic_set_wol_max_caps(nic_t *, nic_wv_type_t, int);
extern uint64_t nic_mcast_hash(const nic_address_t *, size_t);
extern uint64_t nic_query_mcast_hash(nic_t *);

/* Software period functions */
extern void nic_sw_period_start(nic_t *);
extern void nic_sw_period_stop(nic_t *);

#endif // __NIC_H__

/** @}
 */
