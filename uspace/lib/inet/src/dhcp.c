/*
 * Copyright (c) 2013 Jiri Svoboda
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
