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
 * @brief Prototypes of default DDF NIC interface methods implementations
 */

#ifndef NIC_IMPL_H__
#define NIC_IMPL_H__

#include <assert.h>
#include <nic/nic.h>
#include <ddf/driver.h>

/* Inclusion of this file is not prohibited, because drivers could want to
 * inject some adaptation layer between the DDF call and NICF implementation */

extern errno_t nic_get_address_impl(ddf_fun_t *dev_fun, nic_address_t *address);
extern errno_t nic_send_frame_impl(ddf_fun_t *dev_fun, void *data, size_t size);
extern errno_t nic_callback_create_impl(ddf_fun_t *dev_fun);
extern errno_t nic_get_state_impl(ddf_fun_t *dev_fun, nic_device_state_t *state);
extern errno_t nic_set_state_impl(ddf_fun_t *dev_fun, nic_device_state_t state);
extern errno_t nic_get_stats_impl(ddf_fun_t *dev_fun, nic_device_stats_t *stats);
extern errno_t nic_unicast_get_mode_impl(ddf_fun_t *dev_fun,
    nic_unicast_mode_t *, size_t, nic_address_t *, size_t *);
extern errno_t nic_unicast_set_mode_impl(ddf_fun_t *dev_fun,
    nic_unicast_mode_t, const nic_address_t *, size_t);
extern errno_t nic_multicast_get_mode_impl(ddf_fun_t *dev_fun,
    nic_multicast_mode_t *, size_t, nic_address_t *, size_t *);
extern errno_t nic_multicast_set_mode_impl(ddf_fun_t *dev_fun,
    nic_multicast_mode_t, const nic_address_t *, size_t);
extern errno_t nic_broadcast_get_mode_impl(ddf_fun_t *, nic_broadcast_mode_t *);
extern errno_t nic_broadcast_set_mode_impl(ddf_fun_t *, nic_broadcast_mode_t);
extern errno_t nic_blocked_sources_get_impl(ddf_fun_t *,
    size_t, nic_address_t *, size_t *);
extern errno_t nic_blocked_sources_set_impl(ddf_fun_t *, const nic_address_t *, size_t);
extern errno_t nic_vlan_get_mask_impl(ddf_fun_t *, nic_vlan_mask_t *);
extern errno_t nic_vlan_set_mask_impl(ddf_fun_t *, const nic_vlan_mask_t *);
extern errno_t nic_wol_virtue_add_impl(ddf_fun_t *dev_fun, nic_wv_type_t type,
    const void *data, size_t length, nic_wv_id_t *new_id);
extern errno_t nic_wol_virtue_remove_impl(ddf_fun_t *dev_fun, nic_wv_id_t id);
extern errno_t nic_wol_virtue_probe_impl(ddf_fun_t *dev_fun, nic_wv_id_t id,
    nic_wv_type_t *type, size_t max_length, void *data, size_t *length);
extern errno_t nic_wol_virtue_list_impl(ddf_fun_t *dev_fun, nic_wv_type_t type,
    size_t max_count, nic_wv_id_t *id_list, size_t *id_count);
extern errno_t nic_wol_virtue_get_caps_impl(ddf_fun_t *, nic_wv_type_t, int *);
extern errno_t nic_poll_get_mode_impl(ddf_fun_t *,
    nic_poll_mode_t *, struct timeval *);
extern errno_t nic_poll_set_mode_impl(ddf_fun_t *,
    nic_poll_mode_t, const struct timeval *);
extern errno_t nic_poll_now_impl(ddf_fun_t *);

extern void nic_default_handler_impl(ddf_fun_t *dev_fun,
    cap_call_handle_t chandle, ipc_call_t *call);
extern errno_t nic_open_impl(ddf_fun_t *fun);
extern void nic_close_impl(ddf_fun_t *fun);

extern void nic_device_added_impl(ddf_dev_t *dev);

#endif

/** @}
 */
