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

/** @addtogroup nconfsrv
 * @{
 */
/**
 * @file
 * @brief Network configuration Service
 */

#include <adt/list.h>
#include <async.h>
#include <errno.h>
#include <str_error.h>
#include <fibril_synch.h>
#include <inet/dhcp.h>
#include <inet/inetcfg.h>
#include <io/log.h>
#include <ipc/inet.h>
#include <ipc/services.h>
#include <loc.h>
#include <stdio.h>
#include <stdlib.h>
#include <task.h>
#include "iplink.h"
#include "nconfsrv.h"

#define NAME "nconfsrv"

static void ncs_client_conn(ipc_callid_t iid, ipc_call_t *icall, void *arg);

static errno_t ncs_init(void)
{
	service_id_t sid;
	errno_t rc;

	log_msg(LOG_DEFAULT, LVL_DEBUG, "ncs_init()");

	rc = inetcfg_init();
	if (rc != EOK) {
		log_msg(LOG_DEFAULT, LVL_ERROR, "Error contacting inet "
		    "configuration service.");
		return EIO;
	}

	rc = dhcp_init();
	if (rc != EOK) {
		log_msg(LOG_DEFAULT, LVL_ERROR, "Error contacting dhcp "
		    "configuration service.");
		return EIO;
	}

	async_set_fallback_port_handler(ncs_client_conn, NULL);

	rc = loc_server_register(NAME);
	if (rc != EOK) {
		log_msg(LOG_DEFAULT, LVL_ERROR, "Failed registering server: %s.", str_error(rc));
		return EEXIST;
	}

	rc = loc_service_register(SERVICE_NAME_NETCONF, &sid);
	if (rc != EOK) {
		log_msg(LOG_DEFAULT, LVL_ERROR, "Failed registering service: %s.", str_error(rc));
		return EEXIST;
	}

	rc = ncs_link_discovery_start();
	if (rc != EOK)
		return EEXIST;

	return EOK;
}

static void ncs_client_conn(ipc_callid_t iid, ipc_call_t *icall, void *arg)
{
	async_answer_0(iid, ENOTSUP);
}

int main(int argc, char *argv[])
{
	errno_t rc;

	printf(NAME ": HelenOS Network configuration service\n");

	if (log_init(NAME) != EOK) {
		printf(NAME ": Failed to initialize logging.\n");
		return 1;
	}

	rc = ncs_init();
	if (rc != EOK)
		return 1;

	printf(NAME ": Accepting connections.\n");
	task_retval(0);
	async_manager();

	/* Not reached */
	return 0;
}

/** @}
 */
