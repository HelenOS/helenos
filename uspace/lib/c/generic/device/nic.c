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

/** @addtogroup libc
 * @{
 */
/**
 * @file
 * @brief Client-side RPC stubs for DDF NIC
 */

#include <ipc/dev_iface.h>
#include <assert.h>
#include <device/nic.h>
#include <errno.h>
#include <async.h>
#include <malloc.h>
#include <stdio.h>
#include <ipc/services.h>

/** Send frame from NIC
 *
 * @param[in] dev_sess
 * @param[in] data     Frame data
 * @param[in] size     Frame size in bytes
 *
 * @return EOK If the operation was successfully completed
 *
 */
int nic_send_frame(async_sess_t *dev_sess, void *data, size_t size)
{
	async_exch_t *exch = async_exchange_begin(dev_sess);
	
	ipc_call_t answer;
	aid_t req = async_send_1(exch, DEV_IFACE_ID(NIC_DEV_IFACE),
	    NIC_SEND_MESSAGE, &answer);
	sysarg_t retval = async_data_write_start(exch, data, size);
	
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
int nic_callback_create(async_sess_t *dev_sess, async_client_conn_t cfun,
    void *carg)
{
	ipc_call_t answer;
	int rc;
	sysarg_t retval;
	
	async_exch_t *exch = async_exchange_begin(dev_sess);
	aid_t req = async_send_1(exch, DEV_IFACE_ID(NIC_DEV_IFACE),
	    NIC_CALLBACK_CREATE, &answer);
	
	rc = async_connect_to_me(exch, 0, 0, 0, cfun, carg);
	if (rc != EOK) {
		async_forget(req);
		return rc;
	}
	async_exchange_end(exch);
	
	async_wait_for(req, &retval);
	return (int) retval;
}

/** Get the current state of the device
 *
 * @param[in]  dev_sess
 * @param[out] state    Current state
 *
 * @return EOK If the operation was successfully completed
 *
 */
int nic_get_state(async_sess_t *dev_sess, nic_device_state_t *state)
{
	assert(state);
	
	sysarg_t _state;
	
	async_exch_t *exch = async_exchange_begin(dev_sess);
	int rc = async_req_1_1(exch, DEV_IFACE_ID(NIC_DEV_IFACE),
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
int nic_set_state(async_sess_t *dev_sess, nic_device_state_t state)
{
	async_exch_t *exch = async_exchange_begin(dev_sess);
	int rc = async_req_2_0(exch, DEV_IFACE_ID(NIC_DEV_IFACE),
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
int nic_get_address(async_sess_t *dev_sess, nic_address_t *address)
{
	assert(address);
	
	async_exch_t *exch = async_exchange_begin(dev_sess);
	aid_t aid = async_send_1(exch, DEV_IFACE_ID(NIC_DEV_IFACE),
	    NIC_GET_ADDRESS, NULL);
	int rc = async_data_read_start(exch, address, sizeof(nic_address_t));
	async_exchange_end(exch);
	
	sysarg_t res;
	async_wait_for(aid, &res);
	
	if (rc != EOK)
		return rc;
	
	return (int) res;
}

/** Set the address of the device (e.g. MAC on Ethernet)
 *
 * @param[in] dev_sess
 * @param[in] address  Pointer to the address
 *
 * @return EOK If the operation was successfully completed
 *
 */
int nic_set_address(async_sess_t *dev_sess, const nic_address_t *address)
{
	assert(address);
	
	async_exch_t *exch = async_exchange_begin(dev_sess);
	aid_t aid = async_send_1(exch, DEV_IFACE_ID(NIC_DEV_IFACE),
	    NIC_SET_ADDRESS, NULL);
	int rc = async_data_write_start(exch, address, sizeof(nic_address_t));
	async_exchange_end(exch);
	
	sysarg_t res;
	async_wait_for(aid, &res);
	
	if (rc != EOK)
		return rc;
	
	return (int) res;
}

/** Request statistic data about NIC operation.
 *
 * @param[in]  dev_sess
 * @param[out] stats    Structure with the statistics
 *
 * @return EOK If the operation was successfully completed
 *
 */
int nic_get_stats(async_sess_t *dev_sess, nic_device_stats_t *stats)
{
	assert(stats);
	
	async_exch_t *exch = async_exchange_begin(dev_sess);
	
	int rc = async_req_1_0(exch, DEV_IFACE_ID(NIC_DEV_IFACE),
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
int nic_get_device_info(async_sess_t *dev_sess, nic_device_info_t *device_info)
{
	assert(device_info);
	
	async_exch_t *exch = async_exchange_begin(dev_sess);
	
	int rc = async_req_1_0(exch, DEV_IFACE_ID(NIC_DEV_IFACE),
	    NIC_GET_DEVICE_INFO);
	if (rc != EOK) {
		async_exchange_end(exch);
		return rc;
	}
	
	rc = async_data_read_start(exch, device_info, sizeof(nic_device_info_t));
	
	async_exchange_end(exch);
	
	return rc;
}

/** Request status of the cable (plugged/unplugged)
 *
 * @param[in]  dev_sess
 * @param[out] cable_state Current cable state
 *
 * @return EOK If the operation was successfully completed
 *
 */
int nic_get_cable_state(async_sess_t *dev_sess, nic_cable_state_t *cable_state)
{
	assert(cable_state);
	
	sysarg_t _cable_state;
	
	async_exch_t *exch = async_exchange_begin(dev_sess);
	int rc = async_req_1_1(exch, DEV_IFACE_ID(NIC_DEV_IFACE),
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
int nic_get_operation_mode(async_sess_t *dev_sess, int *speed,
   nic_channel_mode_t *duplex, nic_role_t *role)
{
	sysarg_t _speed;
	sysarg_t _duplex;
	sysarg_t _role;
	
	async_exch_t *exch = async_exchange_begin(dev_sess);
	int rc = async_req_1_3(exch, DEV_IFACE_ID(NIC_DEV_IFACE),
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
int nic_set_operation_mode(async_sess_t *dev_sess, int speed,
    nic_channel_mode_t duplex, nic_role_t role)
{
	async_exch_t *exch = async_exchange_begin(dev_sess);
	int rc = async_req_4_0(exch, DEV_IFACE_ID(NIC_DEV_IFACE),
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
int nic_autoneg_enable(async_sess_t *dev_sess, uint32_t advertisement)
{
	async_exch_t *exch = async_exchange_begin(dev_sess);
	int rc = async_req_2_0(exch, DEV_IFACE_ID(NIC_DEV_IFACE),
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
int nic_autoneg_disable(async_sess_t *dev_sess)
{
	async_exch_t *exch = async_exchange_begin(dev_sess);
	int rc = async_req_1_0(exch, DEV_IFACE_ID(NIC_DEV_IFACE),
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
int nic_autoneg_probe(async_sess_t *dev_sess, uint32_t *our_advertisement,
    uint32_t *their_advertisement, nic_result_t *result,
    nic_result_t *their_result)
{
	sysarg_t _our_advertisement;
	sysarg_t _their_advertisement;
	sysarg_t _result;
	sysarg_t _their_result;
	
	async_exch_t *exch = async_exchange_begin(dev_sess);
	int rc = async_req_1_4(exch, DEV_IFACE_ID(NIC_DEV_IFACE),
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
int nic_autoneg_restart(async_sess_t *dev_sess)
{
	async_exch_t *exch = async_exchange_begin(dev_sess);
	int rc = async_req_1_0(exch, DEV_IFACE_ID(NIC_DEV_IFACE),
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
int nic_get_pause(async_sess_t *dev_sess, nic_result_t *we_send,
    nic_result_t *we_receive, uint16_t *pause)
{
	sysarg_t _we_send;
	sysarg_t _we_receive;
	sysarg_t _pause;
	
	async_exch_t *exch = async_exchange_begin(dev_sess);
	int rc = async_req_1_3(exch, DEV_IFACE_ID(NIC_DEV_IFACE),
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
int nic_set_pause(async_sess_t *dev_sess, int allow_send, int allow_receive,
    uint16_t pause)
{
	async_exch_t *exch = async_exchange_begin(dev_sess);
	int rc = async_req_4_0(exch, DEV_IFACE_ID(NIC_DEV_IFACE),
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
int nic_unicast_get_mode(async_sess_t *dev_sess, nic_unicast_mode_t *mode,
    size_t max_count, nic_address_t *address_list, size_t *address_count)
{
	assert(mode);
	
	sysarg_t _mode;
	sysarg_t _address_count;
	
	if (!address_list)
		max_count = 0;
	
	async_exch_t *exch = async_exchange_begin(dev_sess);
	
	int rc = async_req_2_2(exch, DEV_IFACE_ID(NIC_DEV_IFACE),
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
int nic_unicast_set_mode(async_sess_t *dev_sess, nic_unicast_mode_t mode,
    const nic_address_t *address_list, size_t address_count)
{
	if (address_list == NULL)
		address_count = 0;
	
	async_exch_t *exch = async_exchange_begin(dev_sess);
	
	aid_t message_id = async_send_3(exch, DEV_IFACE_ID(NIC_DEV_IFACE),
	    NIC_UNICAST_SET_MODE, (sysarg_t) mode, address_count, NULL);
	
	int rc;
	if (address_count)
		rc = async_data_write_start(exch, address_list,
		    address_count * sizeof(nic_address_t));
	else
		rc = EOK;
	
	async_exchange_end(exch);
	
	sysarg_t res;
	async_wait_for(message_id, &res);
	
	if (rc != EOK)
		return rc;
	
	return (int) res;
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
int nic_multicast_get_mode(async_sess_t *dev_sess, nic_multicast_mode_t *mode,
    size_t max_count, nic_address_t *address_list, size_t *address_count)
{
	assert(mode);
	
	sysarg_t _mode;
	
	if (!address_list)
		max_count = 0;
	
	async_exch_t *exch = async_exchange_begin(dev_sess);
	
	sysarg_t ac;
	int rc = async_req_2_2(exch, DEV_IFACE_ID(NIC_DEV_IFACE),
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
int nic_multicast_set_mode(async_sess_t *dev_sess, nic_multicast_mode_t mode,
    const nic_address_t *address_list, size_t address_count)
{
	if (address_list == NULL)
		address_count = 0;
	
	async_exch_t *exch = async_exchange_begin(dev_sess);
	
	aid_t message_id = async_send_3(exch, DEV_IFACE_ID(NIC_DEV_IFACE),
	    NIC_MULTICAST_SET_MODE, (sysarg_t) mode, address_count, NULL);
	
	int rc;
	if (address_count)
		rc = async_data_write_start(exch, address_list,
		    address_count * sizeof(nic_address_t));
	else
		rc = EOK;
	
	async_exchange_end(exch);
	
	sysarg_t res;
	async_wait_for(message_id, &res);
	
	if (rc != EOK)
		return rc;
	
	return (int) res;
}

/** Determine if broadcast packets are received.
 *
 * @param[in]  dev_sess
 * @param[out] mode     Current operation mode
 *
 * @return EOK If the operation was successfully completed
 *
 */
int nic_broadcast_get_mode(async_sess_t *dev_sess, nic_broadcast_mode_t *mode)
{
	assert(mode);
	
	sysarg_t _mode;
	
	async_exch_t *exch = async_exchange_begin(dev_sess);
	int rc = async_req_1_1(exch, DEV_IFACE_ID(NIC_DEV_IFACE),
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
int nic_broadcast_set_mode(async_sess_t *dev_sess, nic_broadcast_mode_t mode)
{
	async_exch_t *exch = async_exchange_begin(dev_sess);
	int rc = async_req_2_0(exch, DEV_IFACE_ID(NIC_DEV_IFACE),
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
int nic_defective_get_mode(async_sess_t *dev_sess, uint32_t *mode)
{
	assert(mode);
	
	sysarg_t _mode;
	
	async_exch_t *exch = async_exchange_begin(dev_sess);
	int rc = async_req_1_1(exch, DEV_IFACE_ID(NIC_DEV_IFACE),
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
int nic_defective_set_mode(async_sess_t *dev_sess, uint32_t mode)
{
	async_exch_t *exch = async_exchange_begin(dev_sess);
	int rc = async_req_2_0(exch, DEV_IFACE_ID(NIC_DEV_IFACE),
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
int nic_blocked_sources_get(async_sess_t *dev_sess, size_t max_count,
    nic_address_t *address_list, size_t *address_count)
{
	if (!address_list)
		max_count = 0;
	
	async_exch_t *exch = async_exchange_begin(dev_sess);
	
	sysarg_t ac;
	int rc = async_req_2_1(exch, DEV_IFACE_ID(NIC_DEV_IFACE),
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
int nic_blocked_sources_set(async_sess_t *dev_sess,
    const nic_address_t *address_list, size_t address_count)
{
	if (address_list == NULL)
		address_count = 0;
	
	async_exch_t *exch = async_exchange_begin(dev_sess);
	
	aid_t message_id = async_send_2(exch, DEV_IFACE_ID(NIC_DEV_IFACE),
	    NIC_BLOCKED_SOURCES_SET, address_count, NULL);
	
	int rc;
	if (address_count)
		rc = async_data_write_start(exch, address_list,
			address_count * sizeof(nic_address_t));
	else
		rc = EOK;
	
	async_exchange_end(exch);
	
	sysarg_t res;
	async_wait_for(message_id, &res);
	
	if (rc != EOK)
		return rc;
	
	return (int) res;
}

/** Request current VLAN filtering mask.
 *
 * @param[in]  dev_sess
 * @param[out] stats    Structure with the statistics
 *
 * @return EOK If the operation was successfully completed
 *
 */
int nic_vlan_get_mask(async_sess_t *dev_sess, nic_vlan_mask_t *mask)
{
	assert(mask);
	
	async_exch_t *exch = async_exchange_begin(dev_sess);
	int rc = async_req_1_0(exch, DEV_IFACE_ID(NIC_DEV_IFACE),
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
int nic_vlan_set_mask(async_sess_t *dev_sess, const nic_vlan_mask_t *mask)
{
	async_exch_t *exch = async_exchange_begin(dev_sess);
	
	aid_t message_id = async_send_2(exch, DEV_IFACE_ID(NIC_DEV_IFACE),
	    NIC_VLAN_SET_MASK, mask != NULL, NULL);
	
	int rc;
	if (mask != NULL)
		rc = async_data_write_start(exch, mask, sizeof(nic_vlan_mask_t));
	else
		rc = EOK;
	
	async_exchange_end(exch);
	
	sysarg_t res;
	async_wait_for(message_id, &res);
	
	if (rc != EOK)
		return rc;
	
	return (int) res;
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
int nic_vlan_set_tag(async_sess_t *dev_sess, uint16_t tag, bool add, bool strip)
{
	async_exch_t *exch = async_exchange_begin(dev_sess);
	int rc = async_req_4_0(exch, DEV_IFACE_ID(NIC_DEV_IFACE),
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
int nic_wol_virtue_add(async_sess_t *dev_sess, nic_wv_type_t type,
    const void *data, size_t length, nic_wv_id_t *id)
{
	assert(id);
	
	bool send_data = ((data != NULL) && (length != 0));
	async_exch_t *exch = async_exchange_begin(dev_sess);
	
	ipc_call_t result;
	aid_t message_id = async_send_3(exch, DEV_IFACE_ID(NIC_DEV_IFACE),
	    NIC_WOL_VIRTUE_ADD, (sysarg_t) type, send_data, &result);
	
	sysarg_t res;
	if (send_data) {
		int rc = async_data_write_start(exch, data, length);
		if (rc != EOK) {
			async_exchange_end(exch);
			async_wait_for(message_id, &res);
			return rc;
		}
	}
	
	async_exchange_end(exch);
	async_wait_for(message_id, &res);
	
	*id = IPC_GET_ARG1(result);
	return (int) res;
}

/** Remove Wake-On-LAN virtue.
 *
 * @param[in] dev_sess
 * @param[in] id       Virtue identifier
 *
 * @return EOK If the operation was successfully completed
 *
 */
int nic_wol_virtue_remove(async_sess_t *dev_sess, nic_wv_id_t id)
{
	async_exch_t *exch = async_exchange_begin(dev_sess);
	int rc = async_req_2_0(exch, DEV_IFACE_ID(NIC_DEV_IFACE),
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
int nic_wol_virtue_probe(async_sess_t *dev_sess, nic_wv_id_t id,
    nic_wv_type_t *type, size_t max_length, void *data, size_t *length)
{
	sysarg_t _type;
	sysarg_t _length;
	
	if (data == NULL)
		max_length = 0;
	
	async_exch_t *exch = async_exchange_begin(dev_sess);
	
	int rc = async_req_3_2(exch, DEV_IFACE_ID(NIC_DEV_IFACE),
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
int nic_wol_virtue_list(async_sess_t *dev_sess, nic_wv_type_t type,
    size_t max_count, nic_wv_id_t *id_list, size_t *id_count)
{
	if (id_list == NULL)
		max_count = 0;
	
	async_exch_t *exch = async_exchange_begin(dev_sess);
	
	sysarg_t count;
	int rc = async_req_3_1(exch, DEV_IFACE_ID(NIC_DEV_IFACE),
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
int nic_wol_virtue_get_caps(async_sess_t *dev_sess, nic_wv_type_t type,
    int *count)
{
	assert(count);
	
	sysarg_t _count;
	
	async_exch_t *exch = async_exchange_begin(dev_sess);
	int rc = async_req_2_1(exch, DEV_IFACE_ID(NIC_DEV_IFACE),
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
int nic_wol_load_info(async_sess_t *dev_sess, nic_wv_type_t *matched_type,
    size_t max_length, uint8_t *frame, size_t *frame_length)
{
	assert(matched_type);
	
	sysarg_t _matched_type;
	sysarg_t _frame_length;
	
	if (frame == NULL)
		max_length = 0;
	
	async_exch_t *exch = async_exchange_begin(dev_sess);
	
	int rc = async_req_2_2(exch, DEV_IFACE_ID(NIC_DEV_IFACE),
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
int nic_offload_probe(async_sess_t *dev_sess, uint32_t *supported,
    uint32_t *active)
{
	assert(supported);
	assert(active);
	
	sysarg_t _supported;
	sysarg_t _active;
	
	async_exch_t *exch = async_exchange_begin(dev_sess);
	int rc = async_req_1_2(exch, DEV_IFACE_ID(NIC_DEV_IFACE),
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
int nic_offload_set(async_sess_t *dev_sess, uint32_t mask, uint32_t active)
{
	async_exch_t *exch = async_exchange_begin(dev_sess);
	int rc = async_req_3_0(exch, DEV_IFACE_ID(NIC_DEV_IFACE),
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
int nic_poll_get_mode(async_sess_t *dev_sess, nic_poll_mode_t *mode,
    struct timeval *period)
{
	assert(mode);
	
	sysarg_t _mode;
	
	async_exch_t *exch = async_exchange_begin(dev_sess);
	
	int rc = async_req_2_1(exch, DEV_IFACE_ID(NIC_DEV_IFACE),
	    NIC_POLL_GET_MODE, period != NULL, &_mode);
	if (rc != EOK) {
		async_exchange_end(exch);
		return rc;
	}
	
	*mode = (nic_poll_mode_t) _mode;
	
	if (period != NULL)
		rc = async_data_read_start(exch, period, sizeof(struct timeval));
	
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
int nic_poll_set_mode(async_sess_t *dev_sess, nic_poll_mode_t mode,
    const struct timeval *period)
{
	async_exch_t *exch = async_exchange_begin(dev_sess);
	
	aid_t message_id = async_send_3(exch, DEV_IFACE_ID(NIC_DEV_IFACE),
	    NIC_POLL_SET_MODE, (sysarg_t) mode, period != NULL, NULL);
	
	int rc;
	if (period)
		rc = async_data_write_start(exch, period, sizeof(struct timeval));
	else
		rc = EOK;
	
	async_exchange_end(exch);
	
	sysarg_t res;
	async_wait_for(message_id, &res);
	
	if (rc != EOK)
		return rc;
	
	return (int) res;
}

/** Request the driver to poll the NIC.
 *
 * @param[in] dev_sess
 *
 * @return EOK If the operation was successfully completed
 *
 */
int nic_poll_now(async_sess_t *dev_sess)
{
	async_exch_t *exch = async_exchange_begin(dev_sess);
	int rc = async_req_1_0(exch, DEV_IFACE_ID(NIC_DEV_IFACE), NIC_POLL_NOW);
	async_exchange_end(exch);
	
	return rc;
}

/** @}
 */
