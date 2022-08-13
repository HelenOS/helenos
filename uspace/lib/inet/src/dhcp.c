/*
 * SPDX-FileCopyrightText: 2013 Jiri Svoboda
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup libinet
 * @{
 */
/** @file
 */

#include <async.h>
#include <assert.h>
#include <errno.h>
#include <inet/dhcp.h>
#include <ipc/dhcp.h>
#include <ipc/services.h>
#include <loc.h>
#include <stdlib.h>

static async_sess_t *dhcp_sess = NULL;

errno_t dhcp_init(void)
{
	service_id_t dhcp_svc;
	errno_t rc;

	assert(dhcp_sess == NULL);

	rc = loc_service_get_id(SERVICE_NAME_DHCP, &dhcp_svc,
	    IPC_FLAG_BLOCKING);
	if (rc != EOK)
		return ENOENT;

	dhcp_sess = loc_service_connect(dhcp_svc, INTERFACE_DHCP,
	    IPC_FLAG_BLOCKING);
	if (dhcp_sess == NULL)
		return ENOENT;

	return EOK;
}

errno_t dhcp_link_add(sysarg_t link_id)
{
	async_exch_t *exch = async_exchange_begin(dhcp_sess);

	errno_t rc = async_req_1_0(exch, DHCP_LINK_ADD, link_id);
	async_exchange_end(exch);

	return rc;
}

errno_t dhcp_link_remove(sysarg_t link_id)
{
	async_exch_t *exch = async_exchange_begin(dhcp_sess);

	errno_t rc = async_req_1_0(exch, DHCP_LINK_REMOVE, link_id);
	async_exchange_end(exch);

	return rc;
}

errno_t dhcp_discover(sysarg_t link_id)
{
	async_exch_t *exch = async_exchange_begin(dhcp_sess);

	errno_t rc = async_req_1_0(exch, DHCP_DISCOVER, link_id);
	async_exchange_end(exch);

	return rc;
}

/** @}
 */
