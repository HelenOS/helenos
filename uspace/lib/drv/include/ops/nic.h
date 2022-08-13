/*
 * SPDX-FileCopyrightText: 2011 Radim Vansa
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup libdrv
 * @{
 */
/** @file
 * @brief DDF NIC interface definition
 */

#ifndef LIBDRV_OPS_NIC_H_
#define LIBDRV_OPS_NIC_H_

#include <ipc/services.h>
#include <nic/nic.h>
#include <time.h>
#include "../ddf/driver.h"

typedef struct nic_iface {
	/** Mandatory methods */
	errno_t (*send_frame)(ddf_fun_t *, void *, size_t);
	errno_t (*callback_create)(ddf_fun_t *);
	errno_t (*get_state)(ddf_fun_t *, nic_device_state_t *);
	errno_t (*set_state)(ddf_fun_t *, nic_device_state_t);
	errno_t (*get_address)(ddf_fun_t *, nic_address_t *);

	/** Optional methods */
	errno_t (*set_address)(ddf_fun_t *, const nic_address_t *);
	errno_t (*get_stats)(ddf_fun_t *, nic_device_stats_t *);
	errno_t (*get_device_info)(ddf_fun_t *, nic_device_info_t *);
	errno_t (*get_cable_state)(ddf_fun_t *, nic_cable_state_t *);

	errno_t (*get_operation_mode)(ddf_fun_t *, int *, nic_channel_mode_t *,
	    nic_role_t *);
	errno_t (*set_operation_mode)(ddf_fun_t *, int, nic_channel_mode_t,
	    nic_role_t);
	errno_t (*autoneg_enable)(ddf_fun_t *, uint32_t);
	errno_t (*autoneg_disable)(ddf_fun_t *);
	errno_t (*autoneg_probe)(ddf_fun_t *, uint32_t *, uint32_t *,
	    nic_result_t *, nic_result_t *);
	errno_t (*autoneg_restart)(ddf_fun_t *);
	errno_t (*get_pause)(ddf_fun_t *, nic_result_t *, nic_result_t *,
	    uint16_t *);
	errno_t (*set_pause)(ddf_fun_t *, int, int, uint16_t);

	errno_t (*unicast_get_mode)(ddf_fun_t *, nic_unicast_mode_t *, size_t,
	    nic_address_t *, size_t *);
	errno_t (*unicast_set_mode)(ddf_fun_t *, nic_unicast_mode_t,
	    const nic_address_t *, size_t);
	errno_t (*multicast_get_mode)(ddf_fun_t *, nic_multicast_mode_t *, size_t,
	    nic_address_t *, size_t *);
	errno_t (*multicast_set_mode)(ddf_fun_t *, nic_multicast_mode_t,
	    const nic_address_t *, size_t);
	errno_t (*broadcast_get_mode)(ddf_fun_t *, nic_broadcast_mode_t *);
	errno_t (*broadcast_set_mode)(ddf_fun_t *, nic_broadcast_mode_t);
	errno_t (*defective_get_mode)(ddf_fun_t *, uint32_t *);
	errno_t (*defective_set_mode)(ddf_fun_t *, uint32_t);
	errno_t (*blocked_sources_get)(ddf_fun_t *, size_t, nic_address_t *,
	    size_t *);
	errno_t (*blocked_sources_set)(ddf_fun_t *, const nic_address_t *, size_t);

	errno_t (*vlan_get_mask)(ddf_fun_t *, nic_vlan_mask_t *);
	errno_t (*vlan_set_mask)(ddf_fun_t *, const nic_vlan_mask_t *);
	errno_t (*vlan_set_tag)(ddf_fun_t *, uint16_t, bool, bool);

	errno_t (*wol_virtue_add)(ddf_fun_t *, nic_wv_type_t, const void *,
	    size_t, nic_wv_id_t *);
	errno_t (*wol_virtue_remove)(ddf_fun_t *, nic_wv_id_t);
	errno_t (*wol_virtue_probe)(ddf_fun_t *, nic_wv_id_t, nic_wv_type_t *,
	    size_t, void *, size_t *);
	errno_t (*wol_virtue_list)(ddf_fun_t *, nic_wv_type_t, size_t,
	    nic_wv_id_t *, size_t *);
	errno_t (*wol_virtue_get_caps)(ddf_fun_t *, nic_wv_type_t, int *);
	errno_t (*wol_load_info)(ddf_fun_t *, nic_wv_type_t *, size_t,
	    uint8_t *, size_t *);

	errno_t (*offload_probe)(ddf_fun_t *, uint32_t *, uint32_t *);
	errno_t (*offload_set)(ddf_fun_t *, uint32_t, uint32_t);

	errno_t (*poll_get_mode)(ddf_fun_t *, nic_poll_mode_t *,
	    struct timespec *);
	errno_t (*poll_set_mode)(ddf_fun_t *, nic_poll_mode_t,
	    const struct timespec *);
	errno_t (*poll_now)(ddf_fun_t *);
} nic_iface_t;

#endif

/**
 * @}
 */
