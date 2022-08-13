/*
 * SPDX-FileCopyrightText: 2013 Jiri Svoboda
 *
 * SPDX-License-Identifier: BSD-3-Clause
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

static void ncs_client_conn(ipc_call_t *icall, void *arg);

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

static void ncs_client_conn(ipc_call_t *icall, void *arg)
{
	async_answer_0(icall, ENOTSUP);
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
