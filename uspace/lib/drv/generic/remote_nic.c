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
#include <time.h>
#include <macros.h>

#include "ops/nic.h"
#include "nic_iface.h"

typedef enum {
	NIC_SEND_MESSAGE = 0,
	NIC_CALLBACK_CREATE,
	NIC_GET_STATE,
	NIC_SET_STATE,
	NIC_GET_ADDRESS,
	NIC_SET_ADDRESS,
	NIC_GET_STATS,
	NIC_GET_DEVICE_INFO,
	NIC_GET_CABLE_STATE,
	NIC_GET_OPERATION_MODE,
	NIC_SET_OPERATION_MODE,
	NIC_AUTONEG_ENABLE,
	NIC_AUTONEG_DISABLE,
	NIC_AUTONEG_PROBE,
	NIC_AUTONEG_RESTART,
	NIC_GET_PAUSE,
	NIC_SET_PAUSE,
	NIC_UNICAST_GET_MODE,
	NIC_UNICAST_SET_MODE,
	NIC_MULTICAST_GET_MODE,
	NIC_MULTICAST_SET_MODE,
	NIC_BROADCAST_GET_MODE,
	NIC_BROADCAST_SET_MODE,
	NIC_DEFECTIVE_GET_MODE,
	NIC_DEFECTIVE_SET_MODE,
	NIC_BLOCKED_SOURCES_GET,
	NIC_BLOCKED_SOURCES_SET,
	NIC_VLAN_GET_MASK,
	NIC_VLAN_SET_MASK,
	NIC_VLAN_SET_TAG,
	NIC_WOL_VIRTUE_ADD,
	NIC_WOL_VIRTUE_REMOVE,
	NIC_WOL_VIRTUE_PROBE,
	NIC_WOL_VIRTUE_LIST,
	NIC_WOL_VIRTUE_GET_CAPS,
	NIC_WOL_LOAD_INFO,
	NIC_OFFLOAD_PROBE,
	NIC_OFFLOAD_SET,
	NIC_POLL_GET_MODE,
	NIC_POLL_SET_MODE,
	NIC_POLL_NOW
} nic_funcs_t;

/** Send frame from NIC
 *
 * @param[in] dev_sess
 * @param[in] data     Frame data
 * @param[in] size     Frame size in bytes
 *
 * @return EOK If the operation was successfully completed
 *
 */
errno_t nic_send_frame(async_sess_t *dev_sess, void *data, size_t size)
{
	async_exch_t *exch = async_exchange_begin(dev_sess);

	ipc_call_t answer;
	aid_t req = async_send_1(exch, DEV_IFACE_ID(NIC_DEV_IFACE),
	    NIC_SEND_MESSAGE, &answer);
	errno_t retval = async_data_write_start(exch, data, size);

	async_exchange_end(exch);

	if (retval != EOK) {
		async_forget(req);
		return retval;
	}

	async_wait_for(req, &retval);
	return retval;
}

/** Create callback connection from NIC service
 *
 * @param[in] dev_sess
 * @param[in] device_id
 *
 * @return EOK If the operation was successfully completed
 *
 */
errno_t nic_callback_create(async_sess_t *dev_sess, async_port_handler_t cfun,
    void *carg)
{
	ipc_call_t answer;
	errno_t rc;
	errno_t retval;

	async_exch_t *exch = async_exchange_begin(dev_sess);
	aid_t req = async_send_1(exch, DEV_IFACE_ID(NIC_DEV_IFACE),
	    NIC_CALLBACK_CREATE, &answer);

	port_id_t port;
	rc = async_create_callback_port(exch, INTERFACE_NIC_CB, 0, 0,
	    cfun, carg, &port);
	if (rc != EOK) {
		async_forget(req);
		return rc;
	}
	async_exchange_end(exch);

	async_wait_for(req, &retval);
	return retval;
}

/** Get the current state of the device
 *
 * @param[in]  dev_sess
 * @param[out] state    Current state
 *
 * @return EOK If the operation was successfully completed
 *
 */
errno_t nic_get_state(async_sess_t *dev_sess, nic_device_state_t *state)
{
	assert(state);

	sysarg_t _state;

	async_exch_t *exch = async_exchange_begin(dev_sess);
	errno_t rc = async_req_1_1(exch, DEV_IFACE_ID(NIC_DEV_IFACE),
	    NIC_GET_STATE, &_state);
	async_exchange_end(exch);

	*state = (nic_device_state_t) _state;

	return rc;
}

/** Request the device to change its state
 *
 * @param[in] dev_sess
 * @param[in] state    New state
 *
 * @return EOK If the operation was successfully completed
 *
 */
errno_t nic_set_state(async_sess_t *dev_sess, nic_device_state_t state)
{
	async_exch_t *exch = async_exchange_begin(dev_sess);
	errno_t rc = async_req_2_0(exch, DEV_IFACE_ID(NIC_DEV_IFACE),
	    NIC_SET_STATE, state);
	async_exchange_end(exch);

	return rc;
}

/** Request the MAC address of the device
 *
 * @param[in]  dev_sess
 * @param[out] address  Structure with buffer for the address
 *
 * @return EOK If the operation was successfully completed
 *
 */
errno_t nic_get_address(async_sess_t *dev_sess, nic_address_t *address)
{
	assert(address);

	async_exch_t *exch = async_exchange_begin(dev_sess);
	aid_t aid = async_send_1(exch, DEV_IFACE_ID(NIC_DEV_IFACE),
	    NIC_GET_ADDRESS, NULL);
	errno_t rc = async_data_read_start(exch, address, sizeof(nic_address_t));
	async_exchange_end(exch);

	errno_t res;
	async_wait_for(aid, &res);

	if (rc != EOK)
		return rc;

	return res;
}

/** Set the address of the device (e.g. MAC on Ethernet)
 *
 * @param[in] dev_sess
 * @param[in] address  Pointer to the address
 *
 * @return EOK If the operation was successfully completed
 *
 */
errno_t nic_set_address(async_sess_t *dev_sess, const nic_address_t *address)
{
	assert(address);

	async_exch_t *exch = async_exchange_begin(dev_sess);
	aid_t aid = async_send_1(exch, DEV_IFACE_ID(NIC_DEV_IFACE),
	    NIC_SET_ADDRESS, NULL);
	errno_t rc = async_data_write_start(exch, address, sizeof(nic_address_t));
	async_exchange_end(exch);

	errno_t res;
	async_wait_for(aid, &res);

	if (rc != EOK)
		return rc;

	return res;
}

/** Request statistic data about NIC operation.
 *
 * @param[in]  dev_sess
 * @param[out] stats    Structure with the statistics
 *
 * @return EOK If the operation was successfully completed
 *
 */
errno_t nic_get_stats(async_sess_t *dev_sess, nic_device_stats_t *stats)
{
	assert(stats);

	async_exch_t *exch = async_exchange_begin(dev_sess);

	errno_t rc = async_req_1_0(exch, DEV_IFACE_ID(NIC_DEV_IFACE),
	    NIC_GET_STATS);
	if (rc != EOK) {
		async_exchange_end(exch);
		return rc;
	}

	rc = async_data_read_start(exch, stats, sizeof(nic_device_stats_t));

	async_exchange_end(exch);

	return rc;
}

/** Request information about the device.
 *
 * @see nic_device_info_t
 *
 * @param[in]  dev_sess
 * @param[out] device_info Information about the device
 *
 * @return EOK If the operation was successfully completed
 *
 */
errno_t nic_get_device_info(async_sess_t *dev_sess, nic_device_info_t *device_info)
{
	assert(device_info);

	async_exch_t *exch = async_exchange_begin(dev_sess);

	aid_t aid = async_send_1(exch, DEV_IFACE_ID(NIC_DEV_IFACE),
	    NIC_GET_DEVICE_INFO, NULL);
	errno_t rc = async_data_read_start(exch, device_info, sizeof(nic_device_info_t));
	async_exchange_end(exch);

	errno_t res;
	async_wait_for(aid, &res);

	if (rc != EOK)
		return rc;

	return res;
}

/** Request status of the cable (plugged/unplugged)
 *
 * @param[in]  dev_sess
 * @param[out] cable_state Current cable state
 *
 * @return EOK If the operation was successfully completed
 *
 */
errno_t nic_get_cable_state(async_sess_t *dev_sess, nic_cable_state_t *cable_state)
{
	assert(cable_state);

	sysarg_t _cable_state;

	async_exch_t *exch = async_exchange_begin(dev_sess);
	errno_t rc = async_req_1_1(exch, DEV_IFACE_ID(NIC_DEV_IFACE),
	    NIC_GET_CABLE_STATE, &_cable_state);
	async_exchange_end(exch);

	*cable_state = (nic_cable_state_t) _cable_state;

	return rc;
}

/** Request current operation mode.
 *
 * @param[in]  dev_sess
 * @param[out] speed    Current operation speed in Mbps. Can be NULL.
 * @param[out] duplex   Full duplex/half duplex. Can be NULL.
 * @param[out] role     Master/slave/auto. Can be NULL.
 *
 * @return EOK If the operation was successfully completed
 *
 */
errno_t nic_get_operation_mode(async_sess_t *dev_sess, int *speed,
    nic_channel_mode_t *duplex, nic_role_t *role)
{
	sysarg_t _speed;
	sysarg_t _duplex;
	sysarg_t _role;

	async_exch_t *exch = async_exchange_begin(dev_sess);
	errno_t rc = async_req_1_3(exch, DEV_IFACE_ID(NIC_DEV_IFACE),
	    NIC_GET_OPERATION_MODE, &_speed, &_duplex, &_role);
	async_exchange_end(exch);

	if (speed)
		*speed = (int) _speed;

	if (duplex)
		*duplex = (nic_channel_mode_t) _duplex;

	if (role)
		*role = (nic_role_t) _role;

	return rc;
}

/** Set current operation mode.
 *
 * If the NIC has auto-negotiation enabled, this command
 * disables auto-negotiation and sets the operation mode.
 *
 * @param[in] dev_sess
 * @param[in] speed    Operation speed in Mbps
 * @param[in] duplex   Full duplex/half duplex
 * @param[in] role     Master/slave/auto (e.g. in Gbit Ethernet]
 *
 * @return EOK If the operation was successfully completed
 *
 */
errno_t nic_set_operation_mode(async_sess_t *dev_sess, int speed,
    nic_channel_mode_t duplex, nic_role_t role)
{
	async_exch_t *exch = async_exchange_begin(dev_sess);
	errno_t rc = async_req_4_0(exch, DEV_IFACE_ID(NIC_DEV_IFACE),
	    NIC_SET_OPERATION_MODE, (sysarg_t) speed, (sysarg_t) duplex,
	    (sysarg_t) role);
	async_exchange_end(exch);

	return rc;
}

/** Enable auto-negotiation.
 *
 * The advertisement argument can only limit some modes,
 * it can never force the NIC to advertise unsupported modes.
 *
 * The allowed modes are defined in "nic/eth_phys.h" in the C library.
 *
 * @param[in] dev_sess
 * @param[in] advertisement Allowed advertised modes. Use 0 for all modes.
 *
 * @return EOK If the operation was successfully completed
 *
 */
errno_t nic_autoneg_enable(async_sess_t *dev_sess, uint32_t advertisement)
{
	async_exch_t *exch = async_exchange_begin(dev_sess);
	errno_t rc = async_req_2_0(exch, DEV_IFACE_ID(NIC_DEV_IFACE),
	    NIC_AUTONEG_ENABLE, (sysarg_t) advertisement);
	async_exchange_end(exch);

	return rc;
}

/** Disable auto-negotiation.
 *
 * @param[in] dev_sess
 *
 * @return EOK If the operation was successfully completed
 *
 */
errno_t nic_autoneg_disable(async_sess_t *dev_sess)
{
	async_exch_t *exch = async_exchange_begin(dev_sess);
	errno_t rc = async_req_1_0(exch, DEV_IFACE_ID(NIC_DEV_IFACE),
	    NIC_AUTONEG_DISABLE);
	async_exchange_end(exch);

	return rc;
}

/** Probe current state of auto-negotiation.
 *
 * Modes are defined in the "nic/eth_phys.h" in the C library.
 *
 * @param[in]  dev_sess
 * @param[out] our_advertisement   Modes advertised by this NIC.
 *                                 Can be NULL.
 * @param[out] their_advertisement Modes advertised by the other side.
 *                                 Can be NULL.
 * @param[out] result              General state of auto-negotiation.
 *                                 Can be NULL.
 * @param[out]  their_result       State of other side auto-negotiation.
 *                                 Can be NULL.
 *
 * @return EOK If the operation was successfully completed
 *
 */
errno_t nic_autoneg_probe(async_sess_t *dev_sess, uint32_t *our_advertisement,
    uint32_t *their_advertisement, nic_result_t *result,
    nic_result_t *their_result)
{
	sysarg_t _our_advertisement;
	sysarg_t _their_advertisement;
	sysarg_t _result;
	sysarg_t _their_result;

	async_exch_t *exch = async_exchange_begin(dev_sess);
	errno_t rc = async_req_1_4(exch, DEV_IFACE_ID(NIC_DEV_IFACE),
	    NIC_AUTONEG_PROBE, &_our_advertisement, &_their_advertisement,
	    &_result, &_their_result);
	async_exchange_end(exch);

	if (our_advertisement)
		*our_advertisement = (uint32_t) _our_advertisement;

	if (*their_advertisement)
		*their_advertisement = (uint32_t) _their_advertisement;

	if (result)
		*result = (nic_result_t) _result;

	if (their_result)
		*their_result = (nic_result_t) _their_result;

	return rc;
}

/** Restart the auto-negotiation process.
 *
 * @param[in] dev_sess
 *
 * @return EOK If the operation was successfully completed
 *
 */
errno_t nic_autoneg_restart(async_sess_t *dev_sess)
{
	async_exch_t *exch = async_exchange_begin(dev_sess);
	errno_t rc = async_req_1_0(exch, DEV_IFACE_ID(NIC_DEV_IFACE),
	    NIC_AUTONEG_RESTART);
	async_exchange_end(exch);

	return rc;
}

/** Query party's sending and reception of the PAUSE frame.
 *
 * @param[in]  dev_sess
 * @param[out] we_send    This NIC sends the PAUSE frame (true/false)
 * @param[out] we_receive This NIC receives the PAUSE frame (true/false)
 * @param[out] pause      The time set to transmitted PAUSE frames.
 *
 * @return EOK If the operation was successfully completed
 *
 */
errno_t nic_get_pause(async_sess_t *dev_sess, nic_result_t *we_send,
    nic_result_t *we_receive, uint16_t *pause)
{
	sysarg_t _we_send;
	sysarg_t _we_receive;
	sysarg_t _pause;

	async_exch_t *exch = async_exchange_begin(dev_sess);
	errno_t rc = async_req_1_3(exch, DEV_IFACE_ID(NIC_DEV_IFACE),
	    NIC_GET_PAUSE, &_we_send, &_we_receive, &_pause);
	async_exchange_end(exch);

	if (we_send)
		*we_send = _we_send;

	if (we_receive)
		*we_receive = _we_receive;

	if (pause)
		*pause = _pause;

	return rc;
}

/** Control sending and reception of the PAUSE frame.
 *
 * @param[in] dev_sess
 * @param[in] allow_send    Allow sending the PAUSE frame (true/false)
 * @param[in] allow_receive Allow reception of the PAUSE frame (true/false)
 * @param[in] pause         Pause length in 512 bit units written
 *                          to transmitted frames. The value 0 means
 *                          auto value (the best). If the requested
 *                          time cannot be set the driver is allowed
 *                          to set the nearest supported value.
 *
 * @return EOK If the operation was successfully completed
 *
 */
errno_t nic_set_pause(async_sess_t *dev_sess, int allow_send, int allow_receive,
    uint16_t pause)
{
	async_exch_t *exch = async_exchange_begin(dev_sess);
	errno_t rc = async_req_4_0(exch, DEV_IFACE_ID(NIC_DEV_IFACE),
	    NIC_SET_PAUSE, allow_send, allow_receive, pause);
	async_exchange_end(exch);

	return rc;
}

/** Retrieve current settings of unicast frames reception.
 *
 * Note: In case of mode != NIC_UNICAST_LIST the contents of
 * address_list and address_count are undefined.
 *
 * @param[in]   dev_sess
 * @param[out]  mode          Current operation mode
 * @param[in]   max_count     Maximal number of addresses that could
 *                            be written into the list buffer.
 * @param[out]  address_list  Buffer for the list (array). Can be NULL.
 * @param[out]  address_count Number of addresses in the list before
 *                            possible truncation due to the max_count.
 *
 * @return EOK If the operation was successfully completed
 *
 */
errno_t nic_unicast_get_mode(async_sess_t *dev_sess, nic_unicast_mode_t *mode,
    size_t max_count, nic_address_t *address_list, size_t *address_count)
{
	assert(mode);

	sysarg_t _mode;
	sysarg_t _address_count;

	if (!address_list)
		max_count = 0;

	async_exch_t *exch = async_exchange_begin(dev_sess);

	errno_t rc = async_req_2_2(exch, DEV_IFACE_ID(NIC_DEV_IFACE),
	    NIC_UNICAST_GET_MODE, max_count, &_mode, &_address_count);
	if (rc != EOK) {
		async_exchange_end(exch);
		return rc;
	}

	*mode = (nic_unicast_mode_t) _mode;
	if (address_count)
		*address_count = (size_t) _address_count;

	if ((max_count) && (_address_count))
		rc = async_data_read_start(exch, address_list,
		    max_count * sizeof(nic_address_t));

	async_exchange_end(exch);

	return rc;
}

/** Set which unicast frames are received.
 *
 * @param[in] dev_sess
 * @param[in] mode          Current operation mode
 * @param[in] address_list  The list of addresses. Can be NULL.
 * @param[in] address_count Number of addresses in the list.
 *
 * @return EOK If the operation was successfully completed
 *
 */
errno_t nic_unicast_set_mode(async_sess_t *dev_sess, nic_unicast_mode_t mode,
    const nic_address_t *address_list, size_t address_count)
{
	if (address_list == NULL)
		address_count = 0;

	async_exch_t *exch = async_exchange_begin(dev_sess);

	aid_t message_id = async_send_3(exch, DEV_IFACE_ID(NIC_DEV_IFACE),
	    NIC_UNICAST_SET_MODE, (sysarg_t) mode, address_count, NULL);

	errno_t rc;
	if (address_count)
		rc = async_data_write_start(exch, address_list,
		    address_count * sizeof(nic_address_t));
	else
		rc = EOK;

	async_exchange_end(exch);

	errno_t res;
	async_wait_for(message_id, &res);

	if (rc != EOK)
		return rc;

	return res;
}

/** Retrieve current settings of multicast frames reception.
 *
 * Note: In case of mode != NIC_MULTICAST_LIST the contents of
 * address_list and address_count are undefined.
 *
 * @param[in]  dev_sess
 * @param[out] mode          Current operation mode
 * @param[in]  max_count     Maximal number of addresses that could
 *                           be written into the list buffer.
 * @param[out] address_list  Buffer for the list (array). Can be NULL.
 * @param[out] address_count Number of addresses in the list before
 *                           possible truncation due to the max_count.
 *                           Can be NULL.
 *
 * @return EOK If the operation was successfully completed
 *
 */
errno_t nic_multicast_get_mode(async_sess_t *dev_sess, nic_multicast_mode_t *mode,
    size_t max_count, nic_address_t *address_list, size_t *address_count)
{
	assert(mode);

	sysarg_t _mode;

	if (!address_list)
		max_count = 0;

	async_exch_t *exch = async_exchange_begin(dev_sess);

	sysarg_t ac;
	errno_t rc = async_req_2_2(exch, DEV_IFACE_ID(NIC_DEV_IFACE),
	    NIC_MULTICAST_GET_MODE, max_count, &_mode, &ac);
	if (rc != EOK) {
		async_exchange_end(exch);
		return rc;
	}

	*mode = (nic_multicast_mode_t) _mode;
	if (address_count)
		*address_count = (size_t) ac;

	if ((max_count) && (ac))
		rc = async_data_read_start(exch, address_list,
		    max_count * sizeof(nic_address_t));

	async_exchange_end(exch);
	return rc;
}

/** Set which multicast frames are received.
 *
 * @param[in] dev_sess
 * @param[in] mode          Current operation mode
 * @param[in] address_list  The list of addresses. Can be NULL.
 * @param[in] address_count Number of addresses in the list.
 *
 * @return EOK If the operation was successfully completed
 *
 */
errno_t nic_multicast_set_mode(async_sess_t *dev_sess, nic_multicast_mode_t mode,
    const nic_address_t *address_list, size_t address_count)
{
	if (address_list == NULL)
		address_count = 0;

	async_exch_t *exch = async_exchange_begin(dev_sess);

	aid_t message_id = async_send_3(exch, DEV_IFACE_ID(NIC_DEV_IFACE),
	    NIC_MULTICAST_SET_MODE, (sysarg_t) mode, address_count, NULL);

	errno_t rc;
	if (address_count)
		rc = async_data_write_start(exch, address_list,
		    address_count * sizeof(nic_address_t));
	else
		rc = EOK;

	async_exchange_end(exch);

	errno_t res;
	async_wait_for(message_id, &res);

	if (rc != EOK)
		return rc;

	return res;
}

/** Determine if broadcast packets are received.
 *
 * @param[in]  dev_sess
 * @param[out] mode     Current operation mode
 *
 * @return EOK If the operation was successfully completed
 *
 */
errno_t nic_broadcast_get_mode(async_sess_t *dev_sess, nic_broadcast_mode_t *mode)
{
	assert(mode);

	sysarg_t _mode;

	async_exch_t *exch = async_exchange_begin(dev_sess);
	errno_t rc = async_req_1_1(exch, DEV_IFACE_ID(NIC_DEV_IFACE),
	    NIC_BROADCAST_GET_MODE, &_mode);
	async_exchange_end(exch);

	*mode = (nic_broadcast_mode_t) _mode;

	return rc;
}

/** Set whether broadcast packets are received.
 *
 * @param[in] dev_sess
 * @param[in] mode     Current operation mode
 *
 * @return EOK If the operation was successfully completed
 *
 */
errno_t nic_broadcast_set_mode(async_sess_t *dev_sess, nic_broadcast_mode_t mode)
{
	async_exch_t *exch = async_exchange_begin(dev_sess);
	errno_t rc = async_req_2_0(exch, DEV_IFACE_ID(NIC_DEV_IFACE),
	    NIC_BROADCAST_SET_MODE, mode);
	async_exchange_end(exch);

	return rc;
}

/** Determine if defective (erroneous) packets are received.
 *
 * @param[in]  dev_sess
 * @param[out] mode     Bitmask specifying allowed errors
 *
 * @return EOK If the operation was successfully completed
 *
 */
errno_t nic_defective_get_mode(async_sess_t *dev_sess, uint32_t *mode)
{
	assert(mode);

	sysarg_t _mode;

	async_exch_t *exch = async_exchange_begin(dev_sess);
	errno_t rc = async_req_1_1(exch, DEV_IFACE_ID(NIC_DEV_IFACE),
	    NIC_DEFECTIVE_GET_MODE, &_mode);
	async_exchange_end(exch);

	*mode = (uint32_t) _mode;

	return rc;
}

/** Set whether defective (erroneous) packets are received.
 *
 * @param[in]  dev_sess
 * @param[out] mode     Bitmask specifying allowed errors
 *
 * @return EOK If the operation was successfully completed
 *
 */
errno_t nic_defective_set_mode(async_sess_t *dev_sess, uint32_t mode)
{
	async_exch_t *exch = async_exchange_begin(dev_sess);
	errno_t rc = async_req_2_0(exch, DEV_IFACE_ID(NIC_DEV_IFACE),
	    NIC_DEFECTIVE_SET_MODE, mode);
	async_exchange_end(exch);

	return rc;
}

/** Retrieve the currently blocked source MAC addresses.
 *
 * @param[in]  dev_sess
 * @param[in]  max_count     Maximal number of addresses that could
 *                           be written into the list buffer.
 * @param[out] address_list  Buffer for the list (array). Can be NULL.
 * @param[out] address_count Number of addresses in the list before
 *                           possible truncation due to the max_count.
 *
 * @return EOK If the operation was successfully completed
 *
 */
errno_t nic_blocked_sources_get(async_sess_t *dev_sess, size_t max_count,
    nic_address_t *address_list, size_t *address_count)
{
	if (!address_list)
		max_count = 0;

	async_exch_t *exch = async_exchange_begin(dev_sess);

	sysarg_t ac;
	errno_t rc = async_req_2_1(exch, DEV_IFACE_ID(NIC_DEV_IFACE),
	    NIC_BLOCKED_SOURCES_GET, max_count, &ac);
	if (rc != EOK) {
		async_exchange_end(exch);
		return rc;
	}

	if (address_count)
		*address_count = (size_t) ac;

	if ((max_count) && (ac))
		rc = async_data_read_start(exch, address_list,
		    max_count * sizeof(nic_address_t));

	async_exchange_end(exch);
	return rc;
}

/** Set which source MACs are blocked
 *
 * @param[in] dev_sess
 * @param[in] address_list  The list of addresses. Can be NULL.
 * @param[in] address_count Number of addresses in the list.
 *
 * @return EOK If the operation was successfully completed
 *
 */
errno_t nic_blocked_sources_set(async_sess_t *dev_sess,
    const nic_address_t *address_list, size_t address_count)
{
	if (address_list == NULL)
		address_count = 0;

	async_exch_t *exch = async_exchange_begin(dev_sess);

	aid_t message_id = async_send_2(exch, DEV_IFACE_ID(NIC_DEV_IFACE),
	    NIC_BLOCKED_SOURCES_SET, address_count, NULL);

	errno_t rc;
	if (address_count)
		rc = async_data_write_start(exch, address_list,
		    address_count * sizeof(nic_address_t));
	else
		rc = EOK;

	async_exchange_end(exch);

	errno_t res;
	async_wait_for(message_id, &res);

	if (rc != EOK)
		return rc;

	return res;
}

/** Request current VLAN filtering mask.
 *
 * @param[in]  dev_sess
 * @param[out] stats    Structure with the statistics
 *
 * @return EOK If the operation was successfully completed
 *
 */
errno_t nic_vlan_get_mask(async_sess_t *dev_sess, nic_vlan_mask_t *mask)
{
	assert(mask);

	async_exch_t *exch = async_exchange_begin(dev_sess);
	errno_t rc = async_req_1_0(exch, DEV_IFACE_ID(NIC_DEV_IFACE),
	    NIC_VLAN_GET_MASK);
	if (rc != EOK) {
		async_exchange_end(exch);
		return rc;
	}

	rc = async_data_read_start(exch, mask, sizeof(nic_vlan_mask_t));
	async_exchange_end(exch);

	return rc;
}

/** Set the mask used for VLAN filtering.
 *
 * If NULL, VLAN filtering is disabled.
 *
 * @param[in] dev_sess
 * @param[in] mask     Pointer to mask structure or NULL to disable.
 *
 * @return EOK If the operation was successfully completed
 *
 */
errno_t nic_vlan_set_mask(async_sess_t *dev_sess, const nic_vlan_mask_t *mask)
{
	async_exch_t *exch = async_exchange_begin(dev_sess);

	aid_t message_id = async_send_2(exch, DEV_IFACE_ID(NIC_DEV_IFACE),
	    NIC_VLAN_SET_MASK, mask != NULL, NULL);

	errno_t rc;
	if (mask != NULL)
		rc = async_data_write_start(exch, mask, sizeof(nic_vlan_mask_t));
	else
		rc = EOK;

	async_exchange_end(exch);

	errno_t res;
	async_wait_for(message_id, &res);

	if (rc != EOK)
		return rc;

	return res;
}

/** Set VLAN (802.1q) tag.
 *
 * Set whether the tag is to be signaled in offload info and
 * if the tag should be stripped from received frames and added
 * to sent frames automatically. Not every combination of add
 * and strip must be supported.
 *
 * @param[in] dev_sess
 * @param[in] tag      VLAN priority (top 3 bits) and
 *                     the VLAN tag (bottom 12 bits)
 * @param[in] add      Add the VLAN tag automatically (boolean)
 * @param[in] strip    Strip the VLAN tag automatically (boolean)
 *
 * @return EOK If the operation was successfully completed
 *
 */
errno_t nic_vlan_set_tag(async_sess_t *dev_sess, uint16_t tag, bool add, bool strip)
{
	async_exch_t *exch = async_exchange_begin(dev_sess);
	errno_t rc = async_req_4_0(exch, DEV_IFACE_ID(NIC_DEV_IFACE),
	    NIC_VLAN_SET_TAG, (sysarg_t) tag, (sysarg_t) add, (sysarg_t) strip);
	async_exchange_end(exch);

	return rc;
}

/** Add new Wake-On-LAN virtue.
 *
 * @param[in]  dev_sess
 * @param[in]  type     Type of the virtue
 * @param[in]  data     Data required for this virtue
 *                      (depends on type)
 * @param[in]  length   Length of the data
 * @param[out] id       Identifier of the new virtue
 *
 * @return EOK If the operation was successfully completed
 *
 */
errno_t nic_wol_virtue_add(async_sess_t *dev_sess, nic_wv_type_t type,
    const void *data, size_t length, nic_wv_id_t *id)
{
	assert(id);

	bool send_data = ((data != NULL) && (length != 0));
	async_exch_t *exch = async_exchange_begin(dev_sess);

	ipc_call_t result;
	aid_t message_id = async_send_3(exch, DEV_IFACE_ID(NIC_DEV_IFACE),
	    NIC_WOL_VIRTUE_ADD, (sysarg_t) type, send_data, &result);

	errno_t res;
	if (send_data) {
		errno_t rc = async_data_write_start(exch, data, length);
		if (rc != EOK) {
			async_exchange_end(exch);
			async_wait_for(message_id, &res);
			return rc;
		}
	}

	async_exchange_end(exch);
	async_wait_for(message_id, &res);

	*id = ipc_get_arg1(&result);
	return res;
}

/** Remove Wake-On-LAN virtue.
 *
 * @param[in] dev_sess
 * @param[in] id       Virtue identifier
 *
 * @return EOK If the operation was successfully completed
 *
 */
errno_t nic_wol_virtue_remove(async_sess_t *dev_sess, nic_wv_id_t id)
{
	async_exch_t *exch = async_exchange_begin(dev_sess);
	errno_t rc = async_req_2_0(exch, DEV_IFACE_ID(NIC_DEV_IFACE),
	    NIC_WOL_VIRTUE_REMOVE, (sysarg_t) id);
	async_exchange_end(exch);

	return rc;
}

/** Get information about virtue.
 *
 * @param[in]  dev_sess
 * @param[in]  id         Virtue identifier
 * @param[out] type       Type of the filter. Can be NULL.
 * @param[out] max_length Size of the data buffer.
 * @param[out] data       Buffer for data used when the
 *                        virtue was created. Can be NULL.
 * @param[out] length     Length of the data. Can be NULL.
 *
 * @return EOK If the operation was successfully completed
 *
 */
errno_t nic_wol_virtue_probe(async_sess_t *dev_sess, nic_wv_id_t id,
    nic_wv_type_t *type, size_t max_length, void *data, size_t *length)
{
	sysarg_t _type;
	sysarg_t _length;

	if (data == NULL)
		max_length = 0;

	async_exch_t *exch = async_exchange_begin(dev_sess);

	errno_t rc = async_req_3_2(exch, DEV_IFACE_ID(NIC_DEV_IFACE),
	    NIC_WOL_VIRTUE_PROBE, (sysarg_t) id, max_length,
	    &_type, &_length);
	if (rc != EOK) {
		async_exchange_end(exch);
		return rc;
	}

	if (type)
		*type = _type;

	if (length)
		*length = _length;

	if ((max_length) && (_length != 0))
		rc = async_data_read_start(exch, data, max_length);

	async_exchange_end(exch);
	return rc;
}

/** Get a list of all virtues of the specified type.
 *
 * When NIC_WV_NONE is specified as the virtue type the function
 * lists virtues of all types.
 *
 * @param[in]  dev_sess
 * @param[in]  type      Type of the virtues
 * @param[in]  max_count Maximum number of ids that can be
 *                       written into the list buffer.
 * @param[out] id_list   Buffer for to the list of virtue ids.
 *                       Can be NULL.
 * @param[out] id_count  Number of virtue identifiers in the list
 *                       before possible truncation due to the
 *                       max_count. Can be NULL.
 *
 * @return EOK If the operation was successfully completed
 *
 */
errno_t nic_wol_virtue_list(async_sess_t *dev_sess, nic_wv_type_t type,
    size_t max_count, nic_wv_id_t *id_list, size_t *id_count)
{
	if (id_list == NULL)
		max_count = 0;

	async_exch_t *exch = async_exchange_begin(dev_sess);

	sysarg_t count;
	errno_t rc = async_req_3_1(exch, DEV_IFACE_ID(NIC_DEV_IFACE),
	    NIC_WOL_VIRTUE_LIST, (sysarg_t) type, max_count, &count);

	if (id_count)
		*id_count = (size_t) count;

	if ((rc != EOK) || (!max_count)) {
		async_exchange_end(exch);
		return rc;
	}

	rc = async_data_read_start(exch, id_list,
	    max_count * sizeof(nic_wv_id_t));

	async_exchange_end(exch);
	return rc;
}

/** Get number of virtues that can be enabled yet.
 *
 * Count: < 0 => Virtue of this type can be never used
 *        = 0 => No more virtues can be enabled
 *        > 0 => #count virtues can be enabled yet
 *
 * @param[in]  dev_sess
 * @param[in]  type     Virtue type
 * @param[out] count    Number of virtues
 *
 * @return EOK If the operation was successfully completed
 *
 */
errno_t nic_wol_virtue_get_caps(async_sess_t *dev_sess, nic_wv_type_t type,
    int *count)
{
	assert(count);

	sysarg_t _count;

	async_exch_t *exch = async_exchange_begin(dev_sess);
	errno_t rc = async_req_2_1(exch, DEV_IFACE_ID(NIC_DEV_IFACE),
	    NIC_WOL_VIRTUE_GET_CAPS, (sysarg_t) type, &_count);
	async_exchange_end(exch);

	*count = (int) _count;
	return rc;
}

/** Load the frame that issued the wakeup.
 *
 * The NIC can support only matched_type,  only part of the frame
 * can be available or not at all. Sometimes even the type can be
 * uncertain -- in this case the matched_type contains NIC_WV_NONE.
 *
 * Frame_length can be greater than max_length, but at most max_length
 * bytes will be copied into the frame buffer.
 *
 * Note: Only the type of the filter can be detected, not the concrete
 * filter, because the driver is probably not running when the wakeup
 * is issued.
 *
 * @param[in]  dev_sess
 * @param[out] matched_type Type of the filter that issued wakeup.
 * @param[in]  max_length   Size of the buffer
 * @param[out] frame        Buffer for the frame. Can be NULL.
 * @param[out] frame_length Length of the stored frame. Can be NULL.
 *
 * @return EOK If the operation was successfully completed
 *
 */
errno_t nic_wol_load_info(async_sess_t *dev_sess, nic_wv_type_t *matched_type,
    size_t max_length, uint8_t *frame, size_t *frame_length)
{
	assert(matched_type);

	sysarg_t _matched_type;
	sysarg_t _frame_length;

	if (frame == NULL)
		max_length = 0;

	async_exch_t *exch = async_exchange_begin(dev_sess);

	errno_t rc = async_req_2_2(exch, DEV_IFACE_ID(NIC_DEV_IFACE),
	    NIC_WOL_LOAD_INFO, max_length, &_matched_type, &_frame_length);
	if (rc != EOK) {
		async_exchange_end(exch);
		return rc;
	}

	*matched_type = (nic_wv_type_t) _matched_type;
	if (frame_length)
		*frame_length = (size_t) _frame_length;

	if ((max_length != 0) && (_frame_length != 0))
		rc = async_data_read_start(exch, frame, max_length);

	async_exchange_end(exch);
	return rc;
}

/** Probe supported options and current setting of offload computations
 *
 * @param[in]  dev_sess
 * @param[out] supported Supported offload options
 * @param[out] active    Currently active offload options
 *
 * @return EOK If the operation was successfully completed
 *
 */
errno_t nic_offload_probe(async_sess_t *dev_sess, uint32_t *supported,
    uint32_t *active)
{
	assert(supported);
	assert(active);

	sysarg_t _supported;
	sysarg_t _active;

	async_exch_t *exch = async_exchange_begin(dev_sess);
	errno_t rc = async_req_1_2(exch, DEV_IFACE_ID(NIC_DEV_IFACE),
	    NIC_OFFLOAD_PROBE, &_supported, &_active);
	async_exchange_end(exch);

	*supported = (uint32_t) _supported;
	*active = (uint32_t) _active;
	return rc;
}

/** Set which offload computations can be performed on the NIC.
 *
 * @param[in] dev_sess
 * @param[in] mask     Mask for the options (only those set here will be set)
 * @param[in] active   Which options should be enabled and which disabled
 *
 * @return EOK If the operation was successfully completed
 *
 */
errno_t nic_offload_set(async_sess_t *dev_sess, uint32_t mask, uint32_t active)
{
	async_exch_t *exch = async_exchange_begin(dev_sess);
	errno_t rc = async_req_3_0(exch, DEV_IFACE_ID(NIC_DEV_IFACE),
	    NIC_AUTONEG_RESTART, (sysarg_t) mask, (sysarg_t) active);
	async_exchange_end(exch);

	return rc;
}

/** Query the current interrupt/poll mode of the NIC
 *
 * @param[in]  dev_sess
 * @param[out] mode     Current poll mode
 * @param[out] period   Period used in periodic polling.
 *                      Can be NULL.
 *
 * @return EOK If the operation was successfully completed
 *
 */
errno_t nic_poll_get_mode(async_sess_t *dev_sess, nic_poll_mode_t *mode,
    struct timespec *period)
{
	assert(mode);

	sysarg_t _mode;

	async_exch_t *exch = async_exchange_begin(dev_sess);

	errno_t rc = async_req_2_1(exch, DEV_IFACE_ID(NIC_DEV_IFACE),
	    NIC_POLL_GET_MODE, period != NULL, &_mode);
	if (rc != EOK) {
		async_exchange_end(exch);
		return rc;
	}

	*mode = (nic_poll_mode_t) _mode;

	if (period != NULL)
		rc = async_data_read_start(exch, period, sizeof(struct timespec));

	async_exchange_end(exch);
	return rc;
}

/** Set the interrupt/poll mode of the NIC.
 *
 * @param[in] dev_sess
 * @param[in] mode     New poll mode
 * @param[in] period   Period used in periodic polling. Can be NULL.
 *
 * @return EOK If the operation was successfully completed
 *
 */
errno_t nic_poll_set_mode(async_sess_t *dev_sess, nic_poll_mode_t mode,
    const struct timespec *period)
{
	async_exch_t *exch = async_exchange_begin(dev_sess);

	aid_t message_id = async_send_3(exch, DEV_IFACE_ID(NIC_DEV_IFACE),
	    NIC_POLL_SET_MODE, (sysarg_t) mode, period != NULL, NULL);

	errno_t rc;
	if (period)
		rc = async_data_write_start(exch, period, sizeof(struct timespec));
	else
		rc = EOK;

	async_exchange_end(exch);

	errno_t res;
	async_wait_for(message_id, &res);

	if (rc != EOK)
		return rc;

	return res;
}

/** Request the driver to poll the NIC.
 *
 * @param[in] dev_sess
 *
 * @return EOK If the operation was successfully completed
 *
 */
errno_t nic_poll_now(async_sess_t *dev_sess)
{
	async_exch_t *exch = async_exchange_begin(dev_sess);
	errno_t rc = async_req_1_0(exch, DEV_IFACE_ID(NIC_DEV_IFACE), NIC_POLL_NOW);
	async_exchange_end(exch);

	return rc;
}

static void remote_nic_send_frame(ddf_fun_t *dev, void *iface,
    ipc_call_t *call)
{
	nic_iface_t *nic_iface = (nic_iface_t *) iface;
	assert(nic_iface->send_frame);

	void *data;
	size_t size;
	errno_t rc;

	rc = async_data_write_accept(&data, false, 0, 0, 0, &size);
	if (rc != EOK) {
		async_answer_0(call, EINVAL);
		return;
	}

	rc = nic_iface->send_frame(dev, data, size);
	async_answer_0(call, rc);
	free(data);
}

static void remote_nic_callback_create(ddf_fun_t *dev, void *iface,
    ipc_call_t *call)
{
	nic_iface_t *nic_iface = (nic_iface_t *) iface;
	assert(nic_iface->callback_create);

	errno_t rc = nic_iface->callback_create(dev);
	async_answer_0(call, rc);
}

static void remote_nic_get_state(ddf_fun_t *dev, void *iface,
    ipc_call_t *call)
{
	nic_iface_t *nic_iface = (nic_iface_t *) iface;
	assert(nic_iface->get_state);

	nic_device_state_t state = NIC_STATE_MAX;

	errno_t rc = nic_iface->get_state(dev, &state);
	async_answer_1(call, rc, state);
}

static void remote_nic_set_state(ddf_fun_t *dev, void *iface,
    ipc_call_t *call)
{
	nic_iface_t *nic_iface = (nic_iface_t *) iface;
	assert(nic_iface->set_state);

	nic_device_state_t state = (nic_device_state_t) ipc_get_arg2(call);

	errno_t rc = nic_iface->set_state(dev, state);
	async_answer_0(call, rc);
}

static void remote_nic_get_address(ddf_fun_t *dev, void *iface,
    ipc_call_t *call)
{
	nic_iface_t *nic_iface = (nic_iface_t *) iface;
	assert(nic_iface->get_address);

	nic_address_t address;
	memset(&address, 0, sizeof(nic_address_t));

	errno_t rc = nic_iface->get_address(dev, &address);
	if (rc == EOK) {
		ipc_call_t data;
		size_t max_len;

		/* All errors will be translated into EPARTY anyway */
		if (!async_data_read_receive(&data, &max_len)) {
			async_answer_0(&data, EINVAL);
			async_answer_0(call, EINVAL);
			return;
		}

		if (max_len != sizeof(nic_address_t)) {
			async_answer_0(&data, ELIMIT);
			async_answer_0(call, ELIMIT);
			return;
		}

		async_data_read_finalize(&data, &address,
		    sizeof(nic_address_t));
	}

	async_answer_0(call, rc);
}

static void remote_nic_set_address(ddf_fun_t *dev, void *iface,
    ipc_call_t *call)
{
	nic_iface_t *nic_iface = (nic_iface_t *) iface;

	ipc_call_t data;
	size_t length;
	if (!async_data_write_receive(&data, &length)) {
		async_answer_0(&data, EINVAL);
		async_answer_0(call, EINVAL);
		return;
	}

	if (length > sizeof(nic_address_t)) {
		async_answer_0(&data, ELIMIT);
		async_answer_0(call, ELIMIT);
		return;
	}

	nic_address_t address;
	if (async_data_write_finalize(&data, &address, length) != EOK) {
		async_answer_0(call, EINVAL);
		return;
	}

	if (nic_iface->set_address != NULL) {
		errno_t rc = nic_iface->set_address(dev, &address);
		async_answer_0(call, rc);
	} else
		async_answer_0(call, ENOTSUP);
}

static void remote_nic_get_stats(ddf_fun_t *dev, void *iface,
    ipc_call_t *call)
{
	nic_iface_t *nic_iface = (nic_iface_t *) iface;
	if (nic_iface->get_stats == NULL) {
		async_answer_0(call, ENOTSUP);
		return;
	}

	nic_device_stats_t stats;
	memset(&stats, 0, sizeof(nic_device_stats_t));

	errno_t rc = nic_iface->get_stats(dev, &stats);
	if (rc == EOK) {
		ipc_call_t data;
		size_t max_len;
		if (!async_data_read_receive(&data, &max_len)) {
			async_answer_0(&data, EINVAL);
			async_answer_0(call, EINVAL);
			return;
		}

		if (max_len < sizeof(nic_device_stats_t)) {
			async_answer_0(&data, ELIMIT);
			async_answer_0(call, ELIMIT);
			return;
		}

		async_data_read_finalize(&data, &stats,
		    sizeof(nic_device_stats_t));
	}

	async_answer_0(call, rc);
}

static void remote_nic_get_device_info(ddf_fun_t *dev, void *iface,
    ipc_call_t *call)
{
	nic_iface_t *nic_iface = (nic_iface_t *) iface;
	if (nic_iface->get_device_info == NULL) {
		async_answer_0(call, ENOTSUP);
		return;
	}

	nic_device_info_t info;
	memset(&info, 0, sizeof(nic_device_info_t));

	errno_t rc = nic_iface->get_device_info(dev, &info);
	if (rc == EOK) {
		ipc_call_t data;
		size_t max_len;
		if (!async_data_read_receive(&data, &max_len)) {
			async_answer_0(&data, EINVAL);
			async_answer_0(call, EINVAL);
			return;
		}

		if (max_len < sizeof (nic_device_info_t)) {
			async_answer_0(&data, ELIMIT);
			async_answer_0(call, ELIMIT);
			return;
		}

		async_data_read_finalize(&data, &info,
		    sizeof(nic_device_info_t));
	}

	async_answer_0(call, rc);
}

static void remote_nic_get_cable_state(ddf_fun_t *dev, void *iface,
    ipc_call_t *call)
{
	nic_iface_t *nic_iface = (nic_iface_t *) iface;
	if (nic_iface->get_cable_state == NULL) {
		async_answer_0(call, ENOTSUP);
		return;
	}

	nic_cable_state_t cs = NIC_CS_UNKNOWN;

	errno_t rc = nic_iface->get_cable_state(dev, &cs);
	async_answer_1(call, rc, (sysarg_t) cs);
}

static void remote_nic_get_operation_mode(ddf_fun_t *dev, void *iface,
    ipc_call_t *call)
{
	nic_iface_t *nic_iface = (nic_iface_t *) iface;
	if (nic_iface->get_operation_mode == NULL) {
		async_answer_0(call, ENOTSUP);
		return;
	}

	int speed = 0;
	nic_channel_mode_t duplex = NIC_CM_UNKNOWN;
	nic_role_t role = NIC_ROLE_UNKNOWN;

	errno_t rc = nic_iface->get_operation_mode(dev, &speed, &duplex, &role);
	async_answer_3(call, rc, (sysarg_t) speed, (sysarg_t) duplex,
	    (sysarg_t) role);
}

static void remote_nic_set_operation_mode(ddf_fun_t *dev, void *iface,
    ipc_call_t *call)
{
	nic_iface_t *nic_iface = (nic_iface_t *) iface;
	if (nic_iface->set_operation_mode == NULL) {
		async_answer_0(call, ENOTSUP);
		return;
	}

	int speed = (int) ipc_get_arg2(call);
	nic_channel_mode_t duplex = (nic_channel_mode_t) ipc_get_arg3(call);
	nic_role_t role = (nic_role_t) ipc_get_arg4(call);

	errno_t rc = nic_iface->set_operation_mode(dev, speed, duplex, role);
	async_answer_0(call, rc);
}

static void remote_nic_autoneg_enable(ddf_fun_t *dev, void *iface,
    ipc_call_t *call)
{
	nic_iface_t *nic_iface = (nic_iface_t *) iface;
	if (nic_iface->autoneg_enable == NULL) {
		async_answer_0(call, ENOTSUP);
		return;
	}

	uint32_t advertisement = (uint32_t) ipc_get_arg2(call);

	errno_t rc = nic_iface->autoneg_enable(dev, advertisement);
	async_answer_0(call, rc);
}

static void remote_nic_autoneg_disable(ddf_fun_t *dev, void *iface,
    ipc_call_t *call)
{
	nic_iface_t *nic_iface = (nic_iface_t *) iface;
	if (nic_iface->autoneg_disable == NULL) {
		async_answer_0(call, ENOTSUP);
		return;
	}

	errno_t rc = nic_iface->autoneg_disable(dev);
	async_answer_0(call, rc);
}

static void remote_nic_autoneg_probe(ddf_fun_t *dev, void *iface,
    ipc_call_t *call)
{
	nic_iface_t *nic_iface = (nic_iface_t *) iface;
	if (nic_iface->autoneg_probe == NULL) {
		async_answer_0(call, ENOTSUP);
		return;
	}

	uint32_t our_adv = 0;
	uint32_t their_adv = 0;
	nic_result_t result = NIC_RESULT_NOT_AVAILABLE;
	nic_result_t their_result = NIC_RESULT_NOT_AVAILABLE;

	errno_t rc = nic_iface->autoneg_probe(dev, &our_adv, &their_adv, &result,
	    &their_result);
	async_answer_4(call, rc, our_adv, their_adv, (sysarg_t) result,
	    (sysarg_t) their_result);
}

static void remote_nic_autoneg_restart(ddf_fun_t *dev, void *iface,
    ipc_call_t *call)
{
	nic_iface_t *nic_iface = (nic_iface_t *) iface;
	if (nic_iface->autoneg_restart == NULL) {
		async_answer_0(call, ENOTSUP);
		return;
	}

	errno_t rc = nic_iface->autoneg_restart(dev);
	async_answer_0(call, rc);
}

static void remote_nic_get_pause(ddf_fun_t *dev, void *iface,
    ipc_call_t *call)
{
	nic_iface_t *nic_iface = (nic_iface_t *) iface;
	if (nic_iface->get_pause == NULL) {
		async_answer_0(call, ENOTSUP);
		return;
	}

	nic_result_t we_send;
	nic_result_t we_receive;
	uint16_t pause;

	errno_t rc = nic_iface->get_pause(dev, &we_send, &we_receive, &pause);
	async_answer_3(call, rc, we_send, we_receive, pause);
}

static void remote_nic_set_pause(ddf_fun_t *dev, void *iface,
    ipc_call_t *call)
{
	nic_iface_t *nic_iface = (nic_iface_t *) iface;
	if (nic_iface->set_pause == NULL) {
		async_answer_0(call, ENOTSUP);
		return;
	}

	int allow_send = (int) ipc_get_arg2(call);
	int allow_receive = (int) ipc_get_arg3(call);
	uint16_t pause = (uint16_t) ipc_get_arg4(call);

	errno_t rc = nic_iface->set_pause(dev, allow_send, allow_receive,
	    pause);
	async_answer_0(call, rc);
}

static void remote_nic_unicast_get_mode(ddf_fun_t *dev, void *iface,
    ipc_call_t *call)
{
	nic_iface_t *nic_iface = (nic_iface_t *) iface;
	if (nic_iface->unicast_get_mode == NULL) {
		async_answer_0(call, ENOTSUP);
		return;
	}

	size_t max_count = ipc_get_arg2(call);
	nic_address_t *address_list = NULL;

	if (max_count != 0) {
		address_list = malloc(max_count * sizeof (nic_address_t));
		if (!address_list) {
			async_answer_0(call, ENOMEM);
			return;
		}
	}

	memset(address_list, 0, max_count * sizeof(nic_address_t));
	nic_unicast_mode_t mode = NIC_UNICAST_DEFAULT;
	size_t address_count = 0;

	errno_t rc = nic_iface->unicast_get_mode(dev, &mode, max_count, address_list,
	    &address_count);

	if ((rc != EOK) || (max_count == 0) || (address_count == 0)) {
		free(address_list);
		async_answer_2(call, rc, mode, address_count);
		return;
	}

	ipc_call_t data;
	size_t max_len;
	if (!async_data_read_receive(&data, &max_len)) {
		async_answer_0(&data, EINVAL);
		async_answer_2(call, rc, mode, address_count);
		free(address_list);
		return;
	}

	if (max_len > address_count * sizeof(nic_address_t))
		max_len = address_count * sizeof(nic_address_t);

	if (max_len > max_count * sizeof(nic_address_t))
		max_len = max_count * sizeof(nic_address_t);

	async_data_read_finalize(&data, address_list, max_len);

	free(address_list);
	async_answer_2(call, rc, mode, address_count);
}

static void remote_nic_unicast_set_mode(ddf_fun_t *dev, void *iface,
    ipc_call_t *call)
{
	nic_iface_t *nic_iface = (nic_iface_t *) iface;

	size_t length;
	nic_unicast_mode_t mode = ipc_get_arg2(call);
	size_t address_count = ipc_get_arg3(call);
	nic_address_t *address_list = NULL;

	if (address_count) {
		ipc_call_t data;
		if (!async_data_write_receive(&data, &length)) {
			async_answer_0(&data, EINVAL);
			async_answer_0(call, EINVAL);
			return;
		}

		if (length != address_count * sizeof(nic_address_t)) {
			async_answer_0(&data, ELIMIT);
			async_answer_0(call, ELIMIT);
			return;
		}

		address_list = malloc(length);
		if (address_list == NULL) {
			async_answer_0(&data, ENOMEM);
			async_answer_0(call, ENOMEM);
			return;
		}

		if (async_data_write_finalize(&data, address_list,
		    length) != EOK) {
			async_answer_0(call, EINVAL);
			free(address_list);
			return;
		}
	}

	if (nic_iface->unicast_set_mode != NULL) {
		errno_t rc = nic_iface->unicast_set_mode(dev, mode, address_list,
		    address_count);
		async_answer_0(call, rc);
	} else
		async_answer_0(call, ENOTSUP);

	free(address_list);
}

static void remote_nic_multicast_get_mode(ddf_fun_t *dev, void *iface,
    ipc_call_t *call)
{
	nic_iface_t *nic_iface = (nic_iface_t *) iface;
	if (nic_iface->multicast_get_mode == NULL) {
		async_answer_0(call, ENOTSUP);
		return;
	}

	size_t max_count = ipc_get_arg2(call);
	nic_address_t *address_list = NULL;

	if (max_count != 0) {
		address_list = malloc(max_count * sizeof(nic_address_t));
		if (!address_list) {
			async_answer_0(call, ENOMEM);
			return;
		}
	}

	memset(address_list, 0, max_count * sizeof(nic_address_t));
	nic_multicast_mode_t mode = NIC_MULTICAST_BLOCKED;
	size_t address_count = 0;

	errno_t rc = nic_iface->multicast_get_mode(dev, &mode, max_count, address_list,
	    &address_count);

	if ((rc != EOK) || (max_count == 0) || (address_count == 0)) {
		free(address_list);
		async_answer_2(call, rc, mode, address_count);
		return;
	}

	ipc_call_t data;
	size_t max_len;
	if (!async_data_read_receive(&data, &max_len)) {
		async_answer_0(&data, EINVAL);
		async_answer_2(call, rc, mode, address_count);
		free(address_list);
		return;
	}

	if (max_len > address_count * sizeof(nic_address_t))
		max_len = address_count * sizeof(nic_address_t);

	if (max_len > max_count * sizeof(nic_address_t))
		max_len = max_count * sizeof(nic_address_t);

	async_data_read_finalize(&data, address_list, max_len);

	free(address_list);
	async_answer_2(call, rc, mode, address_count);
}

static void remote_nic_multicast_set_mode(ddf_fun_t *dev, void *iface,
    ipc_call_t *call)
{
	nic_iface_t *nic_iface = (nic_iface_t *) iface;

	nic_multicast_mode_t mode = ipc_get_arg2(call);
	size_t address_count = ipc_get_arg3(call);
	nic_address_t *address_list = NULL;

	if (address_count) {
		ipc_call_t data;
		size_t length;
		if (!async_data_write_receive(&data, &length)) {
			async_answer_0(&data, EINVAL);
			async_answer_0(call, EINVAL);
			return;
		}

		if (length != address_count * sizeof (nic_address_t)) {
			async_answer_0(&data, ELIMIT);
			async_answer_0(call, ELIMIT);
			return;
		}

		address_list = malloc(length);
		if (address_list == NULL) {
			async_answer_0(&data, ENOMEM);
			async_answer_0(call, ENOMEM);
			return;
		}

		if (async_data_write_finalize(&data, address_list,
		    length) != EOK) {
			async_answer_0(call, EINVAL);
			free(address_list);
			return;
		}
	}

	if (nic_iface->multicast_set_mode != NULL) {
		errno_t rc = nic_iface->multicast_set_mode(dev, mode, address_list,
		    address_count);
		async_answer_0(call, rc);
	} else
		async_answer_0(call, ENOTSUP);

	free(address_list);
}

static void remote_nic_broadcast_get_mode(ddf_fun_t *dev, void *iface,
    ipc_call_t *call)
{
	nic_iface_t *nic_iface = (nic_iface_t *) iface;
	if (nic_iface->broadcast_get_mode == NULL) {
		async_answer_0(call, ENOTSUP);
		return;
	}

	nic_broadcast_mode_t mode = NIC_BROADCAST_ACCEPTED;

	errno_t rc = nic_iface->broadcast_get_mode(dev, &mode);
	async_answer_1(call, rc, mode);
}

static void remote_nic_broadcast_set_mode(ddf_fun_t *dev, void *iface,
    ipc_call_t *call)
{
	nic_iface_t *nic_iface = (nic_iface_t *) iface;
	if (nic_iface->broadcast_set_mode == NULL) {
		async_answer_0(call, ENOTSUP);
		return;
	}

	nic_broadcast_mode_t mode = ipc_get_arg2(call);

	errno_t rc = nic_iface->broadcast_set_mode(dev, mode);
	async_answer_0(call, rc);
}

static void remote_nic_defective_get_mode(ddf_fun_t *dev, void *iface,
    ipc_call_t *call)
{
	nic_iface_t *nic_iface = (nic_iface_t *) iface;
	if (nic_iface->defective_get_mode == NULL) {
		async_answer_0(call, ENOTSUP);
		return;
	}

	uint32_t mode = 0;

	errno_t rc = nic_iface->defective_get_mode(dev, &mode);
	async_answer_1(call, rc, mode);
}

static void remote_nic_defective_set_mode(ddf_fun_t *dev, void *iface,
    ipc_call_t *call)
{
	nic_iface_t *nic_iface = (nic_iface_t *) iface;
	if (nic_iface->defective_set_mode == NULL) {
		async_answer_0(call, ENOTSUP);
		return;
	}

	uint32_t mode = ipc_get_arg2(call);

	errno_t rc = nic_iface->defective_set_mode(dev, mode);
	async_answer_0(call, rc);
}

static void remote_nic_blocked_sources_get(ddf_fun_t *dev, void *iface,
    ipc_call_t *call)
{
	nic_iface_t *nic_iface = (nic_iface_t *) iface;
	if (nic_iface->blocked_sources_get == NULL) {
		async_answer_0(call, ENOTSUP);
		return;
	}

	size_t max_count = ipc_get_arg2(call);
	nic_address_t *address_list = NULL;

	if (max_count != 0) {
		address_list = malloc(max_count * sizeof(nic_address_t));
		if (!address_list) {
			async_answer_0(call, ENOMEM);
			return;
		}
	}

	memset(address_list, 0, max_count * sizeof(nic_address_t));
	size_t address_count = 0;

	errno_t rc = nic_iface->blocked_sources_get(dev, max_count, address_list,
	    &address_count);

	if ((rc != EOK) || (max_count == 0) || (address_count == 0)) {
		async_answer_1(call, rc, address_count);
		free(address_list);
		return;
	}

	ipc_call_t data;
	size_t max_len;
	if (!async_data_read_receive(&data, &max_len)) {
		async_answer_0(&data, EINVAL);
		async_answer_1(call, rc, address_count);
		free(address_list);
		return;
	}

	if (max_len > address_count * sizeof(nic_address_t))
		max_len = address_count * sizeof(nic_address_t);

	if (max_len > max_count * sizeof(nic_address_t))
		max_len = max_count * sizeof(nic_address_t);

	async_data_read_finalize(&data, address_list, max_len);

	free(address_list);
	async_answer_1(call, rc, address_count);
}

static void remote_nic_blocked_sources_set(ddf_fun_t *dev, void *iface,
    ipc_call_t *call)
{
	nic_iface_t *nic_iface = (nic_iface_t *) iface;

	size_t length;
	size_t address_count = ipc_get_arg2(call);
	nic_address_t *address_list = NULL;

	if (address_count) {
		ipc_call_t data;
		if (!async_data_write_receive(&data, &length)) {
			async_answer_0(&data, EINVAL);
			async_answer_0(call, EINVAL);
			return;
		}

		if (length != address_count * sizeof(nic_address_t)) {
			async_answer_0(&data, ELIMIT);
			async_answer_0(call, ELIMIT);
			return;
		}

		address_list = malloc(length);
		if (address_list == NULL) {
			async_answer_0(&data, ENOMEM);
			async_answer_0(call, ENOMEM);
			return;
		}

		if (async_data_write_finalize(&data, address_list,
		    length) != EOK) {
			async_answer_0(call, EINVAL);
			free(address_list);
			return;
		}
	}

	if (nic_iface->blocked_sources_set != NULL) {
		errno_t rc = nic_iface->blocked_sources_set(dev, address_list,
		    address_count);
		async_answer_0(call, rc);
	} else
		async_answer_0(call, ENOTSUP);

	free(address_list);
}

static void remote_nic_vlan_get_mask(ddf_fun_t *dev, void *iface,
    ipc_call_t *call)
{
	nic_iface_t *nic_iface = (nic_iface_t *) iface;
	if (nic_iface->vlan_get_mask == NULL) {
		async_answer_0(call, ENOTSUP);
		return;
	}

	nic_vlan_mask_t vlan_mask;
	memset(&vlan_mask, 0, sizeof(nic_vlan_mask_t));

	errno_t rc = nic_iface->vlan_get_mask(dev, &vlan_mask);
	if (rc == EOK) {
		ipc_call_t data;
		size_t max_len;
		if (!async_data_read_receive(&data, &max_len)) {
			async_answer_0(&data, EINVAL);
			async_answer_0(call, EINVAL);
			return;
		}

		if (max_len != sizeof(nic_vlan_mask_t)) {
			async_answer_0(&data, EINVAL);
			async_answer_0(call, EINVAL);
			return;
		}

		async_data_read_finalize(&data, &vlan_mask, max_len);
	}

	async_answer_0(call, rc);
}

static void remote_nic_vlan_set_mask(ddf_fun_t *dev, void *iface,
    ipc_call_t *call)
{
	nic_iface_t *nic_iface = (nic_iface_t *) iface;

	nic_vlan_mask_t vlan_mask;
	nic_vlan_mask_t *vlan_mask_pointer = NULL;
	bool vlan_mask_set = (bool) ipc_get_arg2(call);

	if (vlan_mask_set) {
		ipc_call_t data;
		size_t length;
		if (!async_data_write_receive(&data, &length)) {
			async_answer_0(&data, EINVAL);
			async_answer_0(call, EINVAL);
			return;
		}

		if (length != sizeof(nic_vlan_mask_t)) {
			async_answer_0(&data, ELIMIT);
			async_answer_0(call, ELIMIT);
			return;
		}

		if (async_data_write_finalize(&data, &vlan_mask,
		    length) != EOK) {
			async_answer_0(call, EINVAL);
			return;
		}

		vlan_mask_pointer = &vlan_mask;
	}

	if (nic_iface->vlan_set_mask != NULL) {
		errno_t rc = nic_iface->vlan_set_mask(dev, vlan_mask_pointer);
		async_answer_0(call, rc);
	} else
		async_answer_0(call, ENOTSUP);
}

static void remote_nic_vlan_set_tag(ddf_fun_t *dev, void *iface,
    ipc_call_t *call)
{
	nic_iface_t *nic_iface = (nic_iface_t *) iface;

	if (nic_iface->vlan_set_tag == NULL) {
		async_answer_0(call, ENOTSUP);
		return;
	}

	uint16_t tag = (uint16_t) ipc_get_arg2(call);
	bool add = (int) ipc_get_arg3(call);
	bool strip = (int) ipc_get_arg4(call);

	errno_t rc = nic_iface->vlan_set_tag(dev, tag, add, strip);
	async_answer_0(call, rc);
}

static void remote_nic_wol_virtue_add(ddf_fun_t *dev, void *iface,
    ipc_call_t *call)
{
	nic_iface_t *nic_iface = (nic_iface_t *) iface;

	int send_data = (int) ipc_get_arg3(call);
	ipc_call_t data;

	if (nic_iface->wol_virtue_add == NULL) {
		if (send_data) {
			async_data_write_receive(&data, NULL);
			async_answer_0(&data, ENOTSUP);
		}

		async_answer_0(call, ENOTSUP);
	}

	size_t length = 0;
	void *virtue = NULL;

	if (send_data) {
		if (!async_data_write_receive(&data, &length)) {
			async_answer_0(&data, EINVAL);
			async_answer_0(call, EINVAL);
			return;
		}

		virtue = malloc(length);
		if (virtue == NULL) {
			async_answer_0(&data, ENOMEM);
			async_answer_0(call, ENOMEM);
			return;
		}

		if (async_data_write_finalize(&data, virtue,
		    length) != EOK) {
			async_answer_0(call, EINVAL);
			free(virtue);
			return;
		}
	}

	nic_wv_id_t id = 0;
	nic_wv_type_t type = (nic_wv_type_t) ipc_get_arg2(call);

	errno_t rc = nic_iface->wol_virtue_add(dev, type, virtue, length, &id);
	async_answer_1(call, rc, (sysarg_t) id);
	free(virtue);
}

static void remote_nic_wol_virtue_remove(ddf_fun_t *dev, void *iface,
    ipc_call_t *call)
{
	nic_iface_t *nic_iface = (nic_iface_t *) iface;

	if (nic_iface->wol_virtue_remove == NULL) {
		async_answer_0(call, ENOTSUP);
		return;
	}

	nic_wv_id_t id = (nic_wv_id_t) ipc_get_arg2(call);

	errno_t rc = nic_iface->wol_virtue_remove(dev, id);
	async_answer_0(call, rc);
}

static void remote_nic_wol_virtue_probe(ddf_fun_t *dev, void *iface,
    ipc_call_t *call)
{
	nic_iface_t *nic_iface = (nic_iface_t *) iface;

	if (nic_iface->wol_virtue_probe == NULL) {
		async_answer_0(call, ENOTSUP);
		return;
	}

	nic_wv_id_t id = (nic_wv_id_t) ipc_get_arg2(call);
	size_t max_length = ipc_get_arg3(call);
	nic_wv_type_t type = NIC_WV_NONE;
	size_t length = 0;
	ipc_call_t data;
	void *virtue = NULL;

	if (max_length != 0) {
		virtue = malloc(max_length);
		if (virtue == NULL) {
			async_answer_0(call, ENOMEM);
			return;
		}
	}

	memset(virtue, 0, max_length);

	errno_t rc = nic_iface->wol_virtue_probe(dev, id, &type, max_length,
	    virtue, &length);

	if ((max_length != 0) && (length != 0)) {
		size_t req_length;
		if (!async_data_read_receive(&data, &req_length)) {
			async_answer_0(&data, EINVAL);
			async_answer_0(call, EINVAL);
			free(virtue);
			return;
		}

		if (req_length > length)
			req_length = length;

		if (req_length > max_length)
			req_length = max_length;

		async_data_read_finalize(&data, virtue, req_length);
	}

	async_answer_2(call, rc, (sysarg_t) type, (sysarg_t) length);
	free(virtue);
}

static void remote_nic_wol_virtue_list(ddf_fun_t *dev, void *iface,
    ipc_call_t *call)
{
	nic_iface_t *nic_iface = (nic_iface_t *) iface;
	if (nic_iface->wol_virtue_list == NULL) {
		async_answer_0(call, ENOTSUP);
		return;
	}

	nic_wv_type_t type = (nic_wv_type_t) ipc_get_arg2(call);
	size_t max_count = ipc_get_arg3(call);
	size_t count = 0;
	nic_wv_id_t *id_list = NULL;
	ipc_call_t data;

	if (max_count != 0) {
		id_list = malloc(max_count * sizeof(nic_wv_id_t));
		if (id_list == NULL) {
			async_answer_0(call, ENOMEM);
			return;
		}
	}

	memset(id_list, 0, max_count * sizeof (nic_wv_id_t));

	errno_t rc = nic_iface->wol_virtue_list(dev, type, max_count, id_list,
	    &count);

	if ((max_count != 0) && (count != 0)) {
		size_t req_length;
		if (!async_data_read_receive(&data, &req_length)) {
			async_answer_0(&data, EINVAL);
			async_answer_0(call, EINVAL);
			free(id_list);
			return;
		}

		if (req_length > count * sizeof(nic_wv_id_t))
			req_length = count * sizeof(nic_wv_id_t);

		if (req_length > max_count * sizeof(nic_wv_id_t))
			req_length = max_count * sizeof(nic_wv_id_t);

		rc = async_data_read_finalize(&data, id_list, req_length);
	}

	async_answer_1(call, rc, (sysarg_t) count);
	free(id_list);
}

static void remote_nic_wol_virtue_get_caps(ddf_fun_t *dev, void *iface,
    ipc_call_t *call)
{
	nic_iface_t *nic_iface = (nic_iface_t *) iface;
	if (nic_iface->wol_virtue_get_caps == NULL) {
		async_answer_0(call, ENOTSUP);
		return;
	}

	int count = -1;
	nic_wv_type_t type = (nic_wv_type_t) ipc_get_arg2(call);

	errno_t rc = nic_iface->wol_virtue_get_caps(dev, type, &count);
	async_answer_1(call, rc, (sysarg_t) count);
}

static void remote_nic_wol_load_info(ddf_fun_t *dev, void *iface,
    ipc_call_t *call)
{
	nic_iface_t *nic_iface = (nic_iface_t *) iface;
	if (nic_iface->wol_load_info == NULL) {
		async_answer_0(call, ENOTSUP);
		return;
	}

	size_t max_length = (size_t) ipc_get_arg2(call);
	size_t frame_length = 0;
	nic_wv_type_t type = NIC_WV_NONE;
	uint8_t *info = NULL;

	if (max_length != 0) {
		info = malloc(max_length);
		if (info == NULL) {
			async_answer_0(call, ENOMEM);
			return;
		}
	}

	memset(info, 0, max_length);

	errno_t rc = nic_iface->wol_load_info(dev, &type, max_length, info,
	    &frame_length);
	if (rc == EOK) {
		ipc_call_t data;
		size_t req_length;
		if (!async_data_read_receive(&data, &req_length)) {
			async_answer_0(&data, EINVAL);
			async_answer_0(call, EINVAL);
			free(info);
			return;
		}

		req_length = req_length > max_length ? max_length : req_length;
		req_length = req_length > frame_length ? frame_length : req_length;
		async_data_read_finalize(&data, info, req_length);
	}

	async_answer_2(call, rc, (sysarg_t) type, (sysarg_t) frame_length);
	free(info);
}

static void remote_nic_offload_probe(ddf_fun_t *dev, void *iface,
    ipc_call_t *call)
{
	nic_iface_t *nic_iface = (nic_iface_t *) iface;
	if (nic_iface->offload_probe == NULL) {
		async_answer_0(call, ENOTSUP);
		return;
	}

	uint32_t supported = 0;
	uint32_t active = 0;

	errno_t rc = nic_iface->offload_probe(dev, &supported, &active);
	async_answer_2(call, rc, supported, active);
}

static void remote_nic_offload_set(ddf_fun_t *dev, void *iface,
    ipc_call_t *call)
{
	nic_iface_t *nic_iface = (nic_iface_t *) iface;
	if (nic_iface->offload_set == NULL) {
		async_answer_0(call, ENOTSUP);
		return;
	}

	uint32_t mask = (uint32_t) ipc_get_arg2(call);
	uint32_t active = (uint32_t) ipc_get_arg3(call);

	errno_t rc = nic_iface->offload_set(dev, mask, active);
	async_answer_0(call, rc);
}

static void remote_nic_poll_get_mode(ddf_fun_t *dev, void *iface,
    ipc_call_t *call)
{
	nic_iface_t *nic_iface = (nic_iface_t *) iface;
	if (nic_iface->poll_get_mode == NULL) {
		async_answer_0(call, ENOTSUP);
		return;
	}

	nic_poll_mode_t mode = NIC_POLL_IMMEDIATE;
	int request_data = ipc_get_arg2(call);
	struct timespec period = {
		.tv_sec = 0,
		.tv_nsec = 0
	};

	errno_t rc = nic_iface->poll_get_mode(dev, &mode, &period);
	if ((rc == EOK) && (request_data)) {
		ipc_call_t data;
		size_t max_len;

		if (!async_data_read_receive(&data, &max_len)) {
			async_answer_0(&data, EINVAL);
			async_answer_0(call, EINVAL);
			return;
		}

		if (max_len != sizeof(struct timespec)) {
			async_answer_0(&data, ELIMIT);
			async_answer_0(call, ELIMIT);
			return;
		}

		async_data_read_finalize(&data, &period,
		    sizeof(struct timespec));
	}

	async_answer_1(call, rc, (sysarg_t) mode);
}

static void remote_nic_poll_set_mode(ddf_fun_t *dev, void *iface,
    ipc_call_t *call)
{
	nic_iface_t *nic_iface = (nic_iface_t *) iface;

	nic_poll_mode_t mode = ipc_get_arg2(call);
	int has_period = ipc_get_arg3(call);
	struct timespec period_buf;
	struct timespec *period = NULL;
	size_t length;

	if (has_period) {
		ipc_call_t data;
		if (!async_data_write_receive(&data, &length)) {
			async_answer_0(&data, EINVAL);
			async_answer_0(call, EINVAL);
			return;
		}

		if (length != sizeof(struct timespec)) {
			async_answer_0(&data, ELIMIT);
			async_answer_0(call, ELIMIT);
			return;
		}

		period = &period_buf;
		if (async_data_write_finalize(&data, period,
		    length) != EOK) {
			async_answer_0(call, EINVAL);
			return;
		}
	}

	if (nic_iface->poll_set_mode != NULL) {
		errno_t rc = nic_iface->poll_set_mode(dev, mode, period);
		async_answer_0(call, rc);
	} else
		async_answer_0(call, ENOTSUP);
}

static void remote_nic_poll_now(ddf_fun_t *dev, void *iface,
    ipc_call_t *call)
{
	nic_iface_t *nic_iface = (nic_iface_t *) iface;
	if (nic_iface->poll_now == NULL) {
		async_answer_0(call, ENOTSUP);
		return;
	}

	errno_t rc = nic_iface->poll_now(dev);
	async_answer_0(call, rc);
}

/** Remote NIC interface operations.
 *
 */
static const remote_iface_func_ptr_t remote_nic_iface_ops[] = {
	[NIC_SEND_MESSAGE] = remote_nic_send_frame,
	[NIC_CALLBACK_CREATE] = remote_nic_callback_create,
	[NIC_GET_STATE] = remote_nic_get_state,
	[NIC_SET_STATE] = remote_nic_set_state,
	[NIC_GET_ADDRESS] = remote_nic_get_address,
	[NIC_SET_ADDRESS] = remote_nic_set_address,
	[NIC_GET_STATS] = remote_nic_get_stats,
	[NIC_GET_DEVICE_INFO] = remote_nic_get_device_info,
	[NIC_GET_CABLE_STATE] = remote_nic_get_cable_state,
	[NIC_GET_OPERATION_MODE] = remote_nic_get_operation_mode,
	[NIC_SET_OPERATION_MODE] = remote_nic_set_operation_mode,
	[NIC_AUTONEG_ENABLE] = remote_nic_autoneg_enable,
	[NIC_AUTONEG_DISABLE] = remote_nic_autoneg_disable,
	[NIC_AUTONEG_PROBE] = remote_nic_autoneg_probe,
	[NIC_AUTONEG_RESTART] = remote_nic_autoneg_restart,
	[NIC_GET_PAUSE] = remote_nic_get_pause,
	[NIC_SET_PAUSE] = remote_nic_set_pause,
	[NIC_UNICAST_GET_MODE] = remote_nic_unicast_get_mode,
	[NIC_UNICAST_SET_MODE] = remote_nic_unicast_set_mode,
	[NIC_MULTICAST_GET_MODE] = remote_nic_multicast_get_mode,
	[NIC_MULTICAST_SET_MODE] = remote_nic_multicast_set_mode,
	[NIC_BROADCAST_GET_MODE] = remote_nic_broadcast_get_mode,
	[NIC_BROADCAST_SET_MODE] = remote_nic_broadcast_set_mode,
	[NIC_DEFECTIVE_GET_MODE] = remote_nic_defective_get_mode,
	[NIC_DEFECTIVE_SET_MODE] = remote_nic_defective_set_mode,
	[NIC_BLOCKED_SOURCES_GET] = remote_nic_blocked_sources_get,
	[NIC_BLOCKED_SOURCES_SET] = remote_nic_blocked_sources_set,
	[NIC_VLAN_GET_MASK] = remote_nic_vlan_get_mask,
	[NIC_VLAN_SET_MASK] = remote_nic_vlan_set_mask,
	[NIC_VLAN_SET_TAG] = remote_nic_vlan_set_tag,
	[NIC_WOL_VIRTUE_ADD] = remote_nic_wol_virtue_add,
	[NIC_WOL_VIRTUE_REMOVE] = remote_nic_wol_virtue_remove,
	[NIC_WOL_VIRTUE_PROBE] = remote_nic_wol_virtue_probe,
	[NIC_WOL_VIRTUE_LIST] = remote_nic_wol_virtue_list,
	[NIC_WOL_VIRTUE_GET_CAPS] = remote_nic_wol_virtue_get_caps,
	[NIC_WOL_LOAD_INFO] = remote_nic_wol_load_info,
	[NIC_OFFLOAD_PROBE] = remote_nic_offload_probe,
	[NIC_OFFLOAD_SET] = remote_nic_offload_set,
	[NIC_POLL_GET_MODE] = remote_nic_poll_get_mode,
	[NIC_POLL_SET_MODE] = remote_nic_poll_set_mode,
	[NIC_POLL_NOW] = remote_nic_poll_now
};

/** Remote NIC interface structure.
 *
 * Interface for processing request from remote
 * clients addressed to the NIC interface.
 *
 */
const remote_iface_t remote_nic_iface = {
	.method_count = ARRAY_SIZE(remote_nic_iface_ops),
	.methods = remote_nic_iface_ops
};

/**
 * @}
 */
