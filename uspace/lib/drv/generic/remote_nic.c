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

/** @addtogroup libdrv
 * @{
 */
/**
 * @file
 * @brief Driver-side RPC skeletons for DDF NIC interface
 */

#include <assert.h>
#include <async.h>
#include <errno.h>
#include <ipc/services.h>
#include <sys/time.h>
#include "ops/nic.h"

static void remote_nic_send_frame(ddf_fun_t *dev, void *iface,
    ipc_callid_t callid, ipc_call_t *call)
{
	nic_iface_t *nic_iface = (nic_iface_t *) iface;
	assert(nic_iface->send_frame);
	
	void *data;
	size_t size;
	int rc;
	
	rc = async_data_write_accept(&data, false, 0, 0, 0, &size);
	if (rc != EOK) {
		async_answer_0(callid, EINVAL);
		return;
	}
	
	rc = nic_iface->send_frame(dev, data, size);
	async_answer_0(callid, rc);
	free(data);
}

static void remote_nic_callback_create(ddf_fun_t *dev, void *iface,
    ipc_callid_t callid, ipc_call_t *call)
{
	nic_iface_t *nic_iface = (nic_iface_t *) iface;
	assert(nic_iface->callback_create);
	
	int rc = nic_iface->callback_create(dev);
	async_answer_0(callid, rc);
}

static void remote_nic_get_state(ddf_fun_t *dev, void *iface,
    ipc_callid_t callid, ipc_call_t *call)
{
	nic_iface_t *nic_iface = (nic_iface_t *) iface;
	assert(nic_iface->get_state);
	
	nic_device_state_t state = NIC_STATE_MAX;
	
	int rc = nic_iface->get_state(dev, &state);
	async_answer_1(callid, rc, state);
}

static void remote_nic_set_state(ddf_fun_t *dev, void *iface,
    ipc_callid_t callid, ipc_call_t *call)
{
	nic_iface_t *nic_iface = (nic_iface_t *) iface;
	assert(nic_iface->set_state);
	
	nic_device_state_t state = (nic_device_state_t) IPC_GET_ARG2(*call);
	
	int rc = nic_iface->set_state(dev, state);
	async_answer_0(callid, rc);
}

static void remote_nic_get_address(ddf_fun_t *dev, void *iface,
    ipc_callid_t callid, ipc_call_t *call)
{
	nic_iface_t *nic_iface = (nic_iface_t *) iface;
	assert(nic_iface->get_address);
	
	nic_address_t address;
	bzero(&address, sizeof(nic_address_t));
	
	int rc = nic_iface->get_address(dev, &address);
	if (rc == EOK) {
		size_t max_len;
		ipc_callid_t data_callid;
		
		/* All errors will be translated into EPARTY anyway */
		if (!async_data_read_receive(&data_callid, &max_len)) {
			async_answer_0(data_callid, EINVAL);
			async_answer_0(callid, EINVAL);
			return;
		}
		
		if (max_len != sizeof(nic_address_t)) {
			async_answer_0(data_callid, ELIMIT);
			async_answer_0(callid, ELIMIT);
			return;
		}
		
		async_data_read_finalize(data_callid, &address,
		    sizeof(nic_address_t));
	}
	
	async_answer_0(callid, rc);
}

static void remote_nic_set_address(ddf_fun_t *dev, void *iface,
    ipc_callid_t callid, ipc_call_t *call)
{
	nic_iface_t *nic_iface = (nic_iface_t *) iface;
	
	size_t length;
	ipc_callid_t data_callid;
	if (!async_data_write_receive(&data_callid, &length)) {
		async_answer_0(data_callid, EINVAL);
		async_answer_0(callid, EINVAL);
		return;
	}
	
	if (length > sizeof(nic_address_t)) {
		async_answer_0(data_callid, ELIMIT);
		async_answer_0(callid, ELIMIT);
		return;
	}
	
	nic_address_t address;
	if (async_data_write_finalize(data_callid, &address, length) != EOK) {
		async_answer_0(callid, EINVAL);
		return;
	}
	
	if (nic_iface->set_address != NULL) {
		int rc = nic_iface->set_address(dev, &address);
		async_answer_0(callid, rc);
	} else
		async_answer_0(callid, ENOTSUP);
}

static void remote_nic_get_stats(ddf_fun_t *dev, void *iface,
    ipc_callid_t callid, ipc_call_t *call)
{
	nic_iface_t *nic_iface = (nic_iface_t *) iface;
	if (nic_iface->get_stats == NULL) {
		async_answer_0(callid, ENOTSUP);
		return;
	}
	
	nic_device_stats_t stats;
	bzero(&stats, sizeof(nic_device_stats_t));
	
	int rc = nic_iface->get_stats(dev, &stats);
	if (rc == EOK) {
		ipc_callid_t data_callid;
		size_t max_len;
		if (!async_data_read_receive(&data_callid, &max_len)) {
			async_answer_0(data_callid, EINVAL);
			async_answer_0(callid, EINVAL);
			return;
		}
		
		if (max_len < sizeof(nic_device_stats_t)) {
			async_answer_0(data_callid, ELIMIT);
			async_answer_0(callid, ELIMIT);
			return;
		}
		
		async_data_read_finalize(data_callid, &stats,
		    sizeof(nic_device_stats_t));
	}
	
	async_answer_0(callid, rc);
}

static void remote_nic_get_device_info(ddf_fun_t *dev, void *iface,
    ipc_callid_t callid, ipc_call_t *call)
{
	nic_iface_t *nic_iface = (nic_iface_t *) iface;
	if (nic_iface->get_device_info == NULL) {
		async_answer_0(callid, ENOTSUP);
		return;
	}
	
	nic_device_info_t info;
	bzero(&info, sizeof(nic_device_info_t));
	
	int rc = nic_iface->get_device_info(dev, &info);
	if (rc == EOK) {
		ipc_callid_t data_callid;
		size_t max_len;
		if (!async_data_read_receive(&data_callid, &max_len)) {
			async_answer_0(data_callid, EINVAL);
			async_answer_0(callid, EINVAL);
			return;
		}
		
		if (max_len < sizeof (nic_device_info_t)) {
			async_answer_0(data_callid, ELIMIT);
			async_answer_0(callid, ELIMIT);
			return;
		}
		
		async_data_read_finalize(data_callid, &info,
		    sizeof(nic_device_info_t));
	}
	
	async_answer_0(callid, rc);
}

static void remote_nic_get_cable_state(ddf_fun_t *dev, void *iface,
    ipc_callid_t callid, ipc_call_t *call)
{
	nic_iface_t *nic_iface = (nic_iface_t *) iface;
	if (nic_iface->get_cable_state == NULL) {
		async_answer_0(callid, ENOTSUP);
		return;
	}
	
	nic_cable_state_t cs = NIC_CS_UNKNOWN;
	
	int rc = nic_iface->get_cable_state(dev, &cs);
	async_answer_1(callid, rc, (sysarg_t) cs);
}

static void remote_nic_get_operation_mode(ddf_fun_t *dev, void *iface,
    ipc_callid_t callid, ipc_call_t *call)
{
	nic_iface_t *nic_iface = (nic_iface_t *) iface;
	if (nic_iface->get_operation_mode == NULL) {
		async_answer_0(callid, ENOTSUP);
		return;
	}
	
	int speed = 0;
	nic_channel_mode_t duplex = NIC_CM_UNKNOWN;
	nic_role_t role = NIC_ROLE_UNKNOWN;
	
	int rc = nic_iface->get_operation_mode(dev, &speed, &duplex, &role);
	async_answer_3(callid, rc, (sysarg_t) speed, (sysarg_t) duplex,
	    (sysarg_t) role);
}

static void remote_nic_set_operation_mode(ddf_fun_t *dev, void *iface,
    ipc_callid_t callid, ipc_call_t *call)
{
	nic_iface_t *nic_iface = (nic_iface_t *) iface;
	if (nic_iface->set_operation_mode == NULL) {
		async_answer_0(callid, ENOTSUP);
		return;
	}
	
	int speed = (int) IPC_GET_ARG2(*call);
	nic_channel_mode_t duplex = (nic_channel_mode_t) IPC_GET_ARG3(*call);
	nic_role_t role = (nic_role_t) IPC_GET_ARG4(*call);
	
	int rc = nic_iface->set_operation_mode(dev, speed, duplex, role);
	async_answer_0(callid, rc);
}

static void remote_nic_autoneg_enable(ddf_fun_t *dev, void *iface,
    ipc_callid_t callid, ipc_call_t *call)
{
	nic_iface_t *nic_iface = (nic_iface_t *) iface;
	if (nic_iface->autoneg_enable == NULL) {
		async_answer_0(callid, ENOTSUP);
		return;
	}
	
	uint32_t advertisement = (uint32_t) IPC_GET_ARG2(*call);
	
	int rc = nic_iface->autoneg_enable(dev, advertisement);
	async_answer_0(callid, rc);
}

static void remote_nic_autoneg_disable(ddf_fun_t *dev, void *iface,
    ipc_callid_t callid, ipc_call_t *call)
{
	nic_iface_t *nic_iface = (nic_iface_t *) iface;
	if (nic_iface->autoneg_disable == NULL) {
		async_answer_0(callid, ENOTSUP);
		return;
	}
	
	int rc = nic_iface->autoneg_disable(dev);
	async_answer_0(callid, rc);
}

static void remote_nic_autoneg_probe(ddf_fun_t *dev, void *iface,
    ipc_callid_t callid, ipc_call_t *call)
{
	nic_iface_t *nic_iface = (nic_iface_t *) iface;
	if (nic_iface->autoneg_probe == NULL) {
		async_answer_0(callid, ENOTSUP);
		return;
	}
	
	uint32_t our_adv = 0;
	uint32_t their_adv = 0;
	nic_result_t result = NIC_RESULT_NOT_AVAILABLE;
	nic_result_t their_result = NIC_RESULT_NOT_AVAILABLE;
	
	int rc = nic_iface->autoneg_probe(dev, &our_adv, &their_adv, &result,
	    &their_result);
	async_answer_4(callid, rc, our_adv, their_adv, (sysarg_t) result,
	    (sysarg_t) their_result);
}

static void remote_nic_autoneg_restart(ddf_fun_t *dev, void *iface,
    ipc_callid_t callid, ipc_call_t *call)
{
	nic_iface_t *nic_iface = (nic_iface_t *) iface;
	if (nic_iface->autoneg_restart == NULL) {
		async_answer_0(callid, ENOTSUP);
		return;
	}
	
	int rc = nic_iface->autoneg_restart(dev);
	async_answer_0(callid, rc);
}

static void remote_nic_get_pause(ddf_fun_t *dev, void *iface,
    ipc_callid_t callid, ipc_call_t *call)
{
	nic_iface_t *nic_iface = (nic_iface_t *) iface;
	if (nic_iface->get_pause == NULL) {
		async_answer_0(callid, ENOTSUP);
		return;
	}
	
	nic_result_t we_send;
	nic_result_t we_receive;
	uint16_t pause;
	
	int rc = nic_iface->get_pause(dev, &we_send, &we_receive, &pause);
	async_answer_3(callid, rc, we_send, we_receive, pause);
}

static void remote_nic_set_pause(ddf_fun_t *dev, void *iface,
    ipc_callid_t callid, ipc_call_t *call)
{
	nic_iface_t *nic_iface = (nic_iface_t *) iface;
	if (nic_iface->set_pause == NULL) {
		async_answer_0(callid, ENOTSUP);
		return;
	}
	
	int allow_send = (int) IPC_GET_ARG2(*call);
	int allow_receive = (int) IPC_GET_ARG3(*call);
	uint16_t pause = (uint16_t) IPC_GET_ARG4(*call);
	
	int rc = nic_iface->set_pause(dev, allow_send, allow_receive,
	    pause);
	async_answer_0(callid, rc);
}

static void remote_nic_unicast_get_mode(ddf_fun_t *dev, void *iface,
    ipc_callid_t callid, ipc_call_t *call)
{
	nic_iface_t *nic_iface = (nic_iface_t *) iface;
	if (nic_iface->unicast_get_mode == NULL) {
		async_answer_0(callid, ENOTSUP);
		return;
	}
	
	size_t max_count = IPC_GET_ARG2(*call);
	nic_address_t *address_list = NULL;
	
	if (max_count != 0) {
		address_list = malloc(max_count * sizeof (nic_address_t));
		if (!address_list) {
			async_answer_0(callid, ENOMEM);
			return;
		}
	}
	
	bzero(address_list, max_count * sizeof(nic_address_t));
	nic_unicast_mode_t mode = NIC_UNICAST_DEFAULT;
	size_t address_count = 0;
	
	int rc = nic_iface->unicast_get_mode(dev, &mode, max_count, address_list,
	    &address_count);
	
	if ((rc != EOK) || (max_count == 0) || (address_count == 0)) {
		free(address_list);
		async_answer_2(callid, rc, mode, address_count);
		return;
	}
	
	ipc_callid_t data_callid;
	size_t max_len;
	if (!async_data_read_receive(&data_callid, &max_len)) {
		async_answer_0(data_callid, EINVAL);
		async_answer_2(callid, rc, mode, address_count);
		free(address_list);
		return;
	}
	
	if (max_len > address_count * sizeof(nic_address_t))
		max_len = address_count * sizeof(nic_address_t);
	
	if (max_len > max_count * sizeof(nic_address_t))
		max_len = max_count * sizeof(nic_address_t);
	
	async_data_read_finalize(data_callid, address_list, max_len);
	async_answer_0(data_callid, EINVAL);
	
	free(address_list);
	async_answer_2(callid, rc, mode, address_count);
}

static void remote_nic_unicast_set_mode(ddf_fun_t *dev, void *iface,
    ipc_callid_t callid, ipc_call_t *call)
{
	nic_iface_t *nic_iface = (nic_iface_t *) iface;
	
	size_t length;
	nic_unicast_mode_t mode = IPC_GET_ARG2(*call);
	size_t address_count = IPC_GET_ARG3(*call);
	nic_address_t *address_list = NULL;
	
	if (address_count) {
		ipc_callid_t data_callid;
		if (!async_data_write_receive(&data_callid, &length)) {
			async_answer_0(data_callid, EINVAL);
			async_answer_0(callid, EINVAL);
			return;
		}
		
		if (length != address_count * sizeof(nic_address_t)) {
			async_answer_0(data_callid, ELIMIT);
			async_answer_0(callid, ELIMIT);
			return;
		}
		
		address_list = malloc(length);
		if (address_list == NULL) {
			async_answer_0(data_callid, ENOMEM);
			async_answer_0(callid, ENOMEM);
			return;
		}
		
		if (async_data_write_finalize(data_callid, address_list,
		    length) != EOK) {
			async_answer_0(callid, EINVAL);
			free(address_list);
			return;
		}
	}
	
	if (nic_iface->unicast_set_mode != NULL) {
		int rc = nic_iface->unicast_set_mode(dev, mode, address_list,
		    address_count);
		async_answer_0(callid, rc);
	} else
		async_answer_0(callid, ENOTSUP);
	
	free(address_list);
}

static void remote_nic_multicast_get_mode(ddf_fun_t *dev, void *iface,
    ipc_callid_t callid, ipc_call_t *call)
{
	nic_iface_t *nic_iface = (nic_iface_t *) iface;
	if (nic_iface->multicast_get_mode == NULL) {
		async_answer_0(callid, ENOTSUP);
		return;
	}
	
	size_t max_count = IPC_GET_ARG2(*call);
	nic_address_t *address_list = NULL;
	
	if (max_count != 0) {
		address_list = malloc(max_count * sizeof(nic_address_t));
		if (!address_list) {
			async_answer_0(callid, ENOMEM);
			return;
		}
	}
	
	bzero(address_list, max_count * sizeof(nic_address_t));
	nic_multicast_mode_t mode = NIC_MULTICAST_BLOCKED;
	size_t address_count = 0;
	
	int rc = nic_iface->multicast_get_mode(dev, &mode, max_count, address_list,
	    &address_count);
	
	
	if ((rc != EOK) || (max_count == 0) || (address_count == 0)) {
		free(address_list);
		async_answer_2(callid, rc, mode, address_count);
		return;
	}
	
	ipc_callid_t data_callid;
	size_t max_len;
	if (!async_data_read_receive(&data_callid, &max_len)) {
		async_answer_0(data_callid, EINVAL);
		async_answer_2(callid, rc, mode, address_count);
		free(address_list);
		return;
	}
	
	if (max_len > address_count * sizeof(nic_address_t))
		max_len = address_count * sizeof(nic_address_t);
	
	if (max_len > max_count * sizeof(nic_address_t))
		max_len = max_count * sizeof(nic_address_t);
	
	async_data_read_finalize(data_callid, address_list, max_len);
	
	free(address_list);
	async_answer_2(callid, rc, mode, address_count);
}

static void remote_nic_multicast_set_mode(ddf_fun_t *dev, void *iface,
    ipc_callid_t callid, ipc_call_t *call)
{
	nic_iface_t *nic_iface = (nic_iface_t *) iface;
	
	nic_multicast_mode_t mode = IPC_GET_ARG2(*call);
	size_t address_count = IPC_GET_ARG3(*call);
	nic_address_t *address_list = NULL;
	
	if (address_count) {
		ipc_callid_t data_callid;
		size_t length;
		if (!async_data_write_receive(&data_callid, &length)) {
			async_answer_0(data_callid, EINVAL);
			async_answer_0(callid, EINVAL);
			return;
		}
		
		if (length != address_count * sizeof (nic_address_t)) {
			async_answer_0(data_callid, ELIMIT);
			async_answer_0(callid, ELIMIT);
			return;
		}
		
		address_list = malloc(length);
		if (address_list == NULL) {
			async_answer_0(data_callid, ENOMEM);
			async_answer_0(callid, ENOMEM);
			return;
		}
		
		if (async_data_write_finalize(data_callid, address_list,
		    length) != EOK) {
			async_answer_0(callid, EINVAL);
			free(address_list);
			return;
		}
	}
	
	if (nic_iface->multicast_set_mode != NULL) {
		int rc = nic_iface->multicast_set_mode(dev, mode, address_list,
		    address_count);
		async_answer_0(callid, rc);
	} else
		async_answer_0(callid, ENOTSUP);
	
	free(address_list);
}

static void remote_nic_broadcast_get_mode(ddf_fun_t *dev, void *iface,
    ipc_callid_t callid, ipc_call_t *call)
{
	nic_iface_t *nic_iface = (nic_iface_t *) iface;
	if (nic_iface->broadcast_get_mode == NULL) {
		async_answer_0(callid, ENOTSUP);
		return;
	}
	
	nic_broadcast_mode_t mode = NIC_BROADCAST_ACCEPTED;
	
	int rc = nic_iface->broadcast_get_mode(dev, &mode);
	async_answer_1(callid, rc, mode);
}

static void remote_nic_broadcast_set_mode(ddf_fun_t *dev, void *iface,
    ipc_callid_t callid, ipc_call_t *call)
{
	nic_iface_t *nic_iface = (nic_iface_t *) iface;
	if (nic_iface->broadcast_set_mode == NULL) {
		async_answer_0(callid, ENOTSUP);
		return;
	}
	
	nic_broadcast_mode_t mode = IPC_GET_ARG2(*call);
	
	int rc = nic_iface->broadcast_set_mode(dev, mode);
	async_answer_0(callid, rc);
}

static void remote_nic_defective_get_mode(ddf_fun_t *dev, void *iface,
    ipc_callid_t callid, ipc_call_t *call)
{
	nic_iface_t *nic_iface = (nic_iface_t *) iface;
	if (nic_iface->defective_get_mode == NULL) {
		async_answer_0(callid, ENOTSUP);
		return;
	}
	
	uint32_t mode = 0;
	
	int rc = nic_iface->defective_get_mode(dev, &mode);
	async_answer_1(callid, rc, mode);
}

static void remote_nic_defective_set_mode(ddf_fun_t *dev, void *iface,
    ipc_callid_t callid, ipc_call_t *call)
{
	nic_iface_t *nic_iface = (nic_iface_t *) iface;
	if (nic_iface->defective_set_mode == NULL) {
		async_answer_0(callid, ENOTSUP);
		return;
	}
	
	uint32_t mode = IPC_GET_ARG2(*call);
	
	int rc = nic_iface->defective_set_mode(dev, mode);
	async_answer_0(callid, rc);
}

static void remote_nic_blocked_sources_get(ddf_fun_t *dev, void *iface,
    ipc_callid_t callid, ipc_call_t *call)
{
	nic_iface_t *nic_iface = (nic_iface_t *) iface;
	if (nic_iface->blocked_sources_get == NULL) {
		async_answer_0(callid, ENOTSUP);
		return;
	}
	
	size_t max_count = IPC_GET_ARG2(*call);
	nic_address_t *address_list = NULL;
	
	if (max_count != 0) {
		address_list = malloc(max_count * sizeof(nic_address_t));
		if (!address_list) {
			async_answer_0(callid, ENOMEM);
			return;
		}
	}
	
	bzero(address_list, max_count * sizeof(nic_address_t));
	size_t address_count = 0;
	
	int rc = nic_iface->blocked_sources_get(dev, max_count, address_list,
	    &address_count);
	
	if ((rc != EOK) || (max_count == 0) || (address_count == 0)) {
		async_answer_1(callid, rc, address_count);
		free(address_list);
		return;
	}
	
	ipc_callid_t data_callid;
	size_t max_len;
	if (!async_data_read_receive(&data_callid, &max_len)) {
		async_answer_0(data_callid, EINVAL);
		async_answer_1(callid, rc, address_count);
		free(address_list);
		return;
	}
	
	if (max_len > address_count * sizeof(nic_address_t))
		max_len = address_count * sizeof(nic_address_t);
	
	if (max_len > max_count * sizeof(nic_address_t))
		max_len = max_count * sizeof(nic_address_t);
	
	async_data_read_finalize(data_callid, address_list, max_len);
	async_answer_0(data_callid, EINVAL);
	
	free(address_list);
	async_answer_1(callid, rc, address_count);
}

static void remote_nic_blocked_sources_set(ddf_fun_t *dev, void *iface,
    ipc_callid_t callid, ipc_call_t *call)
{
	nic_iface_t *nic_iface = (nic_iface_t *) iface;
	
	size_t length;
	size_t address_count = IPC_GET_ARG2(*call);
	nic_address_t *address_list = NULL;
	
	if (address_count) {
		ipc_callid_t data_callid;
		if (!async_data_write_receive(&data_callid, &length)) {
			async_answer_0(data_callid, EINVAL);
			async_answer_0(callid, EINVAL);
			return;
		}
		
		if (length != address_count * sizeof(nic_address_t)) {
			async_answer_0(data_callid, ELIMIT);
			async_answer_0(callid, ELIMIT);
			return;
		}
		
		address_list = malloc(length);
		if (address_list == NULL) {
			async_answer_0(data_callid, ENOMEM);
			async_answer_0(callid, ENOMEM);
			return;
		}
		
		if (async_data_write_finalize(data_callid, address_list,
		    length) != EOK) {
			async_answer_0(callid, EINVAL);
			free(address_list);
			return;
		}
	}
	
	if (nic_iface->blocked_sources_set != NULL) {
		int rc = nic_iface->blocked_sources_set(dev, address_list,
		    address_count);
		async_answer_0(callid, rc);
	} else
		async_answer_0(callid, ENOTSUP);
	
	free(address_list);
}

static void remote_nic_vlan_get_mask(ddf_fun_t *dev, void *iface,
    ipc_callid_t callid, ipc_call_t *call)
{
	nic_iface_t *nic_iface = (nic_iface_t *) iface;
	if (nic_iface->vlan_get_mask == NULL) {
		async_answer_0(callid, ENOTSUP);
		return;
	}
	
	nic_vlan_mask_t vlan_mask;
	bzero(&vlan_mask, sizeof(nic_vlan_mask_t));
	
	int rc = nic_iface->vlan_get_mask(dev, &vlan_mask);
	if (rc == EOK) {
		ipc_callid_t data_callid;
		size_t max_len;
		if (!async_data_read_receive(&data_callid, &max_len)) {
			async_answer_0(data_callid, EINVAL);
			async_answer_0(callid, EINVAL);
			return;
		}
		
		if (max_len != sizeof(nic_vlan_mask_t)) {
			async_answer_0(data_callid, EINVAL);
			async_answer_0(callid, EINVAL);
			return;
		}
		
		async_data_read_finalize(data_callid, &vlan_mask, max_len);
	}
	
	async_answer_0(callid, rc);
}

static void remote_nic_vlan_set_mask(ddf_fun_t *dev, void *iface,
    ipc_callid_t callid, ipc_call_t *call)
{
	nic_iface_t *nic_iface = (nic_iface_t *) iface;
	
	nic_vlan_mask_t vlan_mask;
	nic_vlan_mask_t *vlan_mask_pointer = NULL;
	bool vlan_mask_set = (bool) IPC_GET_ARG2(*call);
	
	if (vlan_mask_set) {
		ipc_callid_t data_callid;
		size_t length;
		if (!async_data_write_receive(&data_callid, &length)) {
			async_answer_0(data_callid, EINVAL);
			async_answer_0(callid, EINVAL);
			return;
		}
		
		if (length != sizeof(nic_vlan_mask_t)) {
			async_answer_0(data_callid, ELIMIT);
			async_answer_0(callid, ELIMIT);
			return;
		}
		
		if (async_data_write_finalize(data_callid, &vlan_mask,
		    length) != EOK) {
			async_answer_0(callid, EINVAL);
			return;
		}
		
		vlan_mask_pointer = &vlan_mask;
	}
	
	if (nic_iface->vlan_set_mask != NULL) {
		int rc = nic_iface->vlan_set_mask(dev, vlan_mask_pointer);
		async_answer_0(callid, rc);
	} else
		async_answer_0(callid, ENOTSUP);
}

static void remote_nic_vlan_set_tag(ddf_fun_t *dev, void *iface,
    ipc_callid_t callid, ipc_call_t *call)
{
	nic_iface_t *nic_iface = (nic_iface_t *) iface;
	
	if (nic_iface->vlan_set_tag == NULL) {
		async_answer_0(callid, ENOTSUP);
		return;
	}
	
	uint16_t tag = (uint16_t) IPC_GET_ARG2(*call);
	bool add = (int) IPC_GET_ARG3(*call);
	bool strip = (int) IPC_GET_ARG4(*call);
	
	int rc = nic_iface->vlan_set_tag(dev, tag, add, strip);
	async_answer_0(callid, rc);
}

static void remote_nic_wol_virtue_add(ddf_fun_t *dev, void *iface,
    ipc_callid_t callid, ipc_call_t *call)
{
	nic_iface_t *nic_iface = (nic_iface_t *) iface;
	
	int send_data = (int) IPC_GET_ARG3(*call);
	ipc_callid_t data_callid;
	
	if (nic_iface->wol_virtue_add == NULL) {
		if (send_data) {
			async_data_write_receive(&data_callid, NULL);
			async_answer_0(data_callid, ENOTSUP);
		}
		
		async_answer_0(callid, ENOTSUP);
	}
	
	size_t length = 0;
	void *data = NULL;
	
	if (send_data) {
		if (!async_data_write_receive(&data_callid, &length)) {
			async_answer_0(data_callid, EINVAL);
			async_answer_0(callid, EINVAL);
			return;
		}
		
		data = malloc(length);
		if (data == NULL) {
			async_answer_0(data_callid, ENOMEM);
			async_answer_0(callid, ENOMEM);
			return;
		}
		
		if (async_data_write_finalize(data_callid, data,
		    length) != EOK) {
			async_answer_0(callid, EINVAL);
			free(data);
			return;
		}
	}
	
	nic_wv_id_t id = 0;
	nic_wv_type_t type = (nic_wv_type_t) IPC_GET_ARG2(*call);
	
	int rc = nic_iface->wol_virtue_add(dev, type, data, length, &id);
	async_answer_1(callid, rc, (sysarg_t) id);
	free(data);
}

static void remote_nic_wol_virtue_remove(ddf_fun_t *dev, void *iface,
    ipc_callid_t callid, ipc_call_t *call)
{
	nic_iface_t *nic_iface = (nic_iface_t *) iface;
	
	if (nic_iface->wol_virtue_remove == NULL) {
		async_answer_0(callid, ENOTSUP);
		return;
	}
	
	nic_wv_id_t id = (nic_wv_id_t) IPC_GET_ARG2(*call);
	
	int rc = nic_iface->wol_virtue_remove(dev, id);
	async_answer_0(callid, rc);
}

static void remote_nic_wol_virtue_probe(ddf_fun_t *dev, void *iface,
    ipc_callid_t callid, ipc_call_t *call)
{
	nic_iface_t *nic_iface = (nic_iface_t *) iface;
	
	if (nic_iface->wol_virtue_probe == NULL) {
		async_answer_0(callid, ENOTSUP);
		return;
	}
	
	nic_wv_id_t id = (nic_wv_id_t) IPC_GET_ARG2(*call);
	size_t max_length = IPC_GET_ARG3(*call);
	nic_wv_type_t type = NIC_WV_NONE;
	size_t length = 0;
	ipc_callid_t data_callid;
	void *data = NULL;
	
	if (max_length != 0) {
		data = malloc(max_length);
		if (data == NULL) {
			async_answer_0(callid, ENOMEM);
			return;
		}
	}
	
	bzero(data, max_length);
	
	int rc = nic_iface->wol_virtue_probe(dev, id, &type, max_length,
	    data, &length);
	
	if ((max_length != 0) && (length != 0)) {
		size_t req_length;
		if (!async_data_read_receive(&data_callid, &req_length)) {
			async_answer_0(data_callid, EINVAL);
			async_answer_0(callid, EINVAL);
			free(data);
			return;
		}
		
		if (req_length > length)
			req_length = length;
		
		if (req_length > max_length)
			req_length = max_length;
		
		async_data_read_finalize(data_callid, data, req_length);
	}
	
	async_answer_2(callid, rc, (sysarg_t) type, (sysarg_t) length);
	free(data);
}

static void remote_nic_wol_virtue_list(ddf_fun_t *dev, void *iface,
    ipc_callid_t callid, ipc_call_t *call)
{
	nic_iface_t *nic_iface = (nic_iface_t *) iface;
	if (nic_iface->wol_virtue_list == NULL) {
		async_answer_0(callid, ENOTSUP);
		return;
	}
	
	nic_wv_type_t type = (nic_wv_type_t) IPC_GET_ARG2(*call);
	size_t max_count = IPC_GET_ARG3(*call);
	size_t count = 0;
	nic_wv_id_t *id_list = NULL;
	ipc_callid_t data_callid;
	
	if (max_count != 0) {
		id_list = malloc(max_count * sizeof(nic_wv_id_t));
		if (id_list == NULL) {
			async_answer_0(callid, ENOMEM);
			return;
		}
	}
	
	bzero(id_list, max_count * sizeof (nic_wv_id_t));
	
	int rc = nic_iface->wol_virtue_list(dev, type, max_count, id_list,
	    &count);
	
	if ((max_count != 0) && (count != 0)) {
		size_t req_length;
		if (!async_data_read_receive(&data_callid, &req_length)) {
			async_answer_0(data_callid, EINVAL);
			async_answer_0(callid, EINVAL);
			free(id_list);
			return;
		}
		
		if (req_length > count * sizeof(nic_wv_id_t))
			req_length = count * sizeof(nic_wv_id_t);
		
		if (req_length > max_count * sizeof(nic_wv_id_t))
			req_length = max_count * sizeof(nic_wv_id_t);
		
		rc = async_data_read_finalize(data_callid, id_list, req_length);
	}
	
	async_answer_1(callid, rc, (sysarg_t) count);
	free(id_list);
}

static void remote_nic_wol_virtue_get_caps(ddf_fun_t *dev, void *iface,
    ipc_callid_t callid, ipc_call_t *call)
{
	nic_iface_t *nic_iface = (nic_iface_t *) iface;
	if (nic_iface->wol_virtue_get_caps == NULL) {
		async_answer_0(callid, ENOTSUP);
		return;
	}
	
	int count = -1;
	nic_wv_type_t type = (nic_wv_type_t) IPC_GET_ARG2(*call);
	
	int rc = nic_iface->wol_virtue_get_caps(dev, type, &count);
	async_answer_1(callid, rc, (sysarg_t) count);
}

static void remote_nic_wol_load_info(ddf_fun_t *dev, void *iface,
    ipc_callid_t callid, ipc_call_t *call)
{
	nic_iface_t *nic_iface = (nic_iface_t *) iface;
	if (nic_iface->wol_load_info == NULL) {
		async_answer_0(callid, ENOTSUP);
		return;
	}
	
	size_t max_length = (size_t) IPC_GET_ARG2(*call);
	size_t frame_length = 0;
	nic_wv_type_t type = NIC_WV_NONE;
	uint8_t *data = NULL;
	
	if (max_length != 0) {
		data = malloc(max_length);
		if (data == NULL) {
			async_answer_0(callid, ENOMEM);
			return;
		}
	}
	
	bzero(data, max_length);
	
	int rc = nic_iface->wol_load_info(dev, &type, max_length, data,
	    &frame_length);
	if (rc == EOK) {
		ipc_callid_t data_callid;
		size_t req_length;
		if (!async_data_read_receive(&data_callid, &req_length)) {
			async_answer_0(data_callid, EINVAL);
			async_answer_0(callid, EINVAL);
			free(data);
			return;
		}
		
		req_length = req_length > max_length ? max_length : req_length;
		req_length = req_length > frame_length ? frame_length : req_length;
		async_data_read_finalize(data_callid, data, req_length);
	}
	
	async_answer_2(callid, rc, (sysarg_t) type, (sysarg_t) frame_length);
	free(data);
}

static void remote_nic_offload_probe(ddf_fun_t *dev, void *iface,
    ipc_callid_t callid, ipc_call_t *call)
{
	nic_iface_t *nic_iface = (nic_iface_t *) iface;
	if (nic_iface->offload_probe == NULL) {
		async_answer_0(callid, ENOTSUP);
		return;
	}
	
	uint32_t supported = 0;
	uint32_t active = 0;
	
	int rc = nic_iface->offload_probe(dev, &supported, &active);
	async_answer_2(callid, rc, supported, active);
}

static void remote_nic_offload_set(ddf_fun_t *dev, void *iface,
    ipc_callid_t callid, ipc_call_t *call)
{
	nic_iface_t *nic_iface = (nic_iface_t *) iface;
	if (nic_iface->offload_set == NULL) {
		async_answer_0(callid, ENOTSUP);
		return;
	}
	
	uint32_t mask = (uint32_t) IPC_GET_ARG2(*call);
	uint32_t active = (uint32_t) IPC_GET_ARG3(*call);
	
	int rc = nic_iface->offload_set(dev, mask, active);
	async_answer_0(callid, rc);
}

static void remote_nic_poll_get_mode(ddf_fun_t *dev, void *iface,
    ipc_callid_t callid, ipc_call_t *call)
{
	nic_iface_t *nic_iface = (nic_iface_t *) iface;
	if (nic_iface->poll_get_mode == NULL) {
		async_answer_0(callid, ENOTSUP);
		return;
	}
	
	nic_poll_mode_t mode = NIC_POLL_IMMEDIATE;
	int request_data = IPC_GET_ARG2(*call);
	struct timeval period = {
		.tv_sec = 0,
		.tv_usec = 0
	};
	
	int rc = nic_iface->poll_get_mode(dev, &mode, &period);
	if ((rc == EOK) && (request_data)) {
		size_t max_len;
		ipc_callid_t data_callid;
		
		if (!async_data_read_receive(&data_callid, &max_len)) {
			async_answer_0(data_callid, EINVAL);
			async_answer_0(callid, EINVAL);
			return;
		}
		
		if (max_len != sizeof(struct timeval)) {
			async_answer_0(data_callid, ELIMIT);
			async_answer_0(callid, ELIMIT);
			return;
		}
		
		async_data_read_finalize(data_callid, &period,
		    sizeof(struct timeval));
	}
	
	async_answer_1(callid, rc, (sysarg_t) mode);
}

static void remote_nic_poll_set_mode(ddf_fun_t *dev, void *iface,
    ipc_callid_t callid, ipc_call_t *call)
{
	nic_iface_t *nic_iface = (nic_iface_t *) iface;
	
	nic_poll_mode_t mode = IPC_GET_ARG2(*call);
	int has_period = IPC_GET_ARG3(*call);
	struct timeval period_buf;
	struct timeval *period = NULL;
	size_t length;
	
	if (has_period) {
		ipc_callid_t data_callid;
		if (!async_data_write_receive(&data_callid, &length)) {
			async_answer_0(data_callid, EINVAL);
			async_answer_0(callid, EINVAL);
			return;
		}
		
		if (length != sizeof(struct timeval)) {
			async_answer_0(data_callid, ELIMIT);
			async_answer_0(callid, ELIMIT);
			return;
		}
		
		period = &period_buf;
		if (async_data_write_finalize(data_callid, period,
		    length) != EOK) {
			async_answer_0(callid, EINVAL);
			return;
		}
	}
	
	if (nic_iface->poll_set_mode != NULL) {
		int rc = nic_iface->poll_set_mode(dev, mode, period);
		async_answer_0(callid, rc);
	} else
		async_answer_0(callid, ENOTSUP);
}

static void remote_nic_poll_now(ddf_fun_t *dev, void *iface,
    ipc_callid_t callid, ipc_call_t *call)
{
	nic_iface_t *nic_iface = (nic_iface_t *) iface;
	if (nic_iface->poll_now == NULL) {
		async_answer_0(callid, ENOTSUP);
		return;
	}
	
	int rc = nic_iface->poll_now(dev);
	async_answer_0(callid, rc);
}

/** Remote NIC interface operations.
 *
 */
static remote_iface_func_ptr_t remote_nic_iface_ops[] = {
	&remote_nic_send_frame,
	&remote_nic_callback_create,
	&remote_nic_get_state,
	&remote_nic_set_state,
	&remote_nic_get_address,
	&remote_nic_set_address,
	&remote_nic_get_stats,
	&remote_nic_get_device_info,
	&remote_nic_get_cable_state,
	&remote_nic_get_operation_mode,
	&remote_nic_set_operation_mode,
	&remote_nic_autoneg_enable,
	&remote_nic_autoneg_disable,
	&remote_nic_autoneg_probe,
	&remote_nic_autoneg_restart,
	&remote_nic_get_pause,
	&remote_nic_set_pause,
	&remote_nic_unicast_get_mode,
	&remote_nic_unicast_set_mode,
	&remote_nic_multicast_get_mode,
	&remote_nic_multicast_set_mode,
	&remote_nic_broadcast_get_mode,
	&remote_nic_broadcast_set_mode,
	&remote_nic_defective_get_mode,
	&remote_nic_defective_set_mode,
	&remote_nic_blocked_sources_get,
	&remote_nic_blocked_sources_set,
	&remote_nic_vlan_get_mask,
	&remote_nic_vlan_set_mask,
	&remote_nic_vlan_set_tag,
	&remote_nic_wol_virtue_add,
	&remote_nic_wol_virtue_remove,
	&remote_nic_wol_virtue_probe,
	&remote_nic_wol_virtue_list,
	&remote_nic_wol_virtue_get_caps,
	&remote_nic_wol_load_info,
	&remote_nic_offload_probe,
	&remote_nic_offload_set,
	&remote_nic_poll_get_mode,
	&remote_nic_poll_set_mode,
	&remote_nic_poll_now
};

/** Remote NIC interface structure.
 *
 * Interface for processing request from remote
 * clients addressed to the NIC interface.
 *
 */
remote_iface_t remote_nic_iface = {
	.method_count = sizeof(remote_nic_iface_ops) /
	    sizeof(remote_iface_func_ptr_t),
	.methods = remote_nic_iface_ops
};

/**
 * @}
 */
