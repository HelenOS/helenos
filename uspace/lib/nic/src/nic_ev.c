/*
 * SPDX-FileCopyrightText: 2011 Jiri Svoboda
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/**
 * @addtogroup libnic
 * @{
 */
/**
 * @file
 * @brief
 */

#include <async.h>
#include <nic_iface.h>
#include <errno.h>
#include "nic_ev.h"

/** Device address changed. */
errno_t nic_ev_addr_changed(async_sess_t *sess, const nic_address_t *addr)
{
	async_exch_t *exch = async_exchange_begin(sess);

	ipc_call_t answer;
	aid_t req = async_send_0(exch, NIC_EV_ADDR_CHANGED, &answer);
	errno_t retval = async_data_write_start(exch, addr,
	    sizeof(nic_address_t));

	async_exchange_end(exch);

	if (retval != EOK) {
		async_forget(req);
		return retval;
	}

	async_wait_for(req, &retval);
	return retval;
}

/** Device state changed. */
errno_t nic_ev_device_state(async_sess_t *sess, sysarg_t state)
{
	errno_t rc;

	async_exch_t *exch = async_exchange_begin(sess);
	rc = async_req_1_0(exch, NIC_EV_DEVICE_STATE, state);
	async_exchange_end(exch);

	return rc;
}

/** Frame received. */
errno_t nic_ev_received(async_sess_t *sess, void *data, size_t size)
{
	async_exch_t *exch = async_exchange_begin(sess);

	ipc_call_t answer;
	aid_t req = async_send_0(exch, NIC_EV_RECEIVED, &answer);
	errno_t retval = async_data_write_start(exch, data, size);

	async_exchange_end(exch);

	if (retval != EOK) {
		async_forget(req);
		return retval;
	}

	async_wait_for(req, &retval);
	return retval;
}

/** @}
 */
