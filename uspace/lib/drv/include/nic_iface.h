/*
 * Copyright (c) 2010 Radim Vansa
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

 /** @addtogroup libc
 * @{
 */
/** @file
 */

#ifndef LIBDRV_NIC_IFACE_H_
#define LIBDRV_NIC_IFACE_H_

#include <async.h>
#include <nic/nic.h>
#include <ipc/common.h>


typedef enum {
	NIC_EV_ADDR_CHANGED = IPC_FIRST_USER_METHOD,
	NIC_EV_RECEIVED,
	NIC_EV_DEVICE_STATE
} nic_event_t;

extern errno_t nic_send_frame(async_sess_t *, void *, size_t);
extern errno_t nic_callback_create(async_sess_t *, async_port_handler_t, void *);
extern errno_t nic_get_state(async_sess_t *, nic_device_state_t *);
extern errno_t nic_set_state(async_sess_t *, nic_device_state_t);
extern errno_t nic_get_address(async_sess_t *, nic_address_t *);
extern errno_t nic_set_address(async_sess_t *, const nic_address_t *);
extern errno_t nic_get_stats(async_sess_t *, nic_device_stats_t *);
extern errno_t nic_get_device_info(async_sess_t *, nic_device_info_t *);
extern errno_t nic_get_cable_state(async_sess_t *, nic_cable_state_t *);

extern errno_t nic_get_operation_mode(async_sess_t *, int *, nic_channel_mode_t *,
    nic_role_t *);
extern errno_t nic_set_operation_mode(async_sess_t *, int, nic_channel_mode_t,
    nic_role_t);
extern errno_t nic_autoneg_enable(async_sess_t *, uint32_t);
extern errno_t nic_autoneg_disable(async_sess_t *);
extern errno_t nic_autoneg_probe(async_sess_t *, uint32_t *, uint32_t *,
    nic_result_t *, nic_result_t *);
extern errno_t nic_autoneg_restart(async_sess_t *);
extern errno_t nic_get_pause(async_sess_t *, nic_result_t *, nic_result_t *,
    uint16_t *);
extern errno_t nic_set_pause(async_sess_t *, int, int, uint16_t);

extern errno_t nic_unicast_get_mode(async_sess_t *, nic_unicast_mode_t *, size_t,
    nic_address_t *, size_t *);
extern errno_t nic_unicast_set_mode(async_sess_t *, nic_unicast_mode_t,
    const nic_address_t *, size_t);
extern errno_t nic_multicast_get_mode(async_sess_t *, nic_multicast_mode_t *,
    size_t, nic_address_t *, size_t *);
extern errno_t nic_multicast_set_mode(async_sess_t *, nic_multicast_mode_t,
    const nic_address_t *, size_t);
extern errno_t nic_broadcast_get_mode(async_sess_t *, nic_broadcast_mode_t *);
extern errno_t nic_broadcast_set_mode(async_sess_t *, nic_broadcast_mode_t);
extern errno_t nic_defective_get_mode(async_sess_t *, uint32_t *);
extern errno_t nic_defective_set_mode(async_sess_t *, uint32_t);
extern errno_t nic_blocked_sources_get(async_sess_t *, size_t, nic_address_t *,
    size_t *);
extern errno_t nic_blocked_sources_set(async_sess_t *, const nic_address_t *,
    size_t);

extern errno_t nic_vlan_get_mask(async_sess_t *, nic_vlan_mask_t *);
extern errno_t nic_vlan_set_mask(async_sess_t *, const nic_vlan_mask_t *);
extern errno_t nic_vlan_set_tag(async_sess_t *, uint16_t, bool, bool);

extern errno_t nic_wol_virtue_add(async_sess_t *, nic_wv_type_t, const void *,
    size_t, nic_wv_id_t *);
extern errno_t nic_wol_virtue_remove(async_sess_t *, nic_wv_id_t);
extern errno_t nic_wol_virtue_probe(async_sess_t *, nic_wv_id_t, nic_wv_type_t *,
    size_t, void *, size_t *);
extern errno_t nic_wol_virtue_list(async_sess_t *, nic_wv_type_t, size_t,
    nic_wv_id_t *, size_t *);
extern errno_t nic_wol_virtue_get_caps(async_sess_t *, nic_wv_type_t, int *);
extern errno_t nic_wol_load_info(async_sess_t *, nic_wv_type_t *, size_t, uint8_t *,
    size_t *);

extern errno_t nic_offload_probe(async_sess_t *, uint32_t *, uint32_t *);
extern errno_t nic_offload_set(async_sess_t *, uint32_t, uint32_t);

extern errno_t nic_poll_get_mode(async_sess_t *, nic_poll_mode_t *,
    struct timeval *);
extern errno_t nic_poll_set_mode(async_sess_t *, nic_poll_mode_t,
    const struct timeval *);
extern errno_t nic_poll_now(async_sess_t *);

#endif

/** @}
 */
