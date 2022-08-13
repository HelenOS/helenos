/*
 * SPDX-FileCopyrightText: 2012 Jiri Svoboda
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup udp
 * @{
 */

/**
 * @file UDP (User Datagram Protocol) service
 */

#include <async.h>
#include <errno.h>
#include <io/log.h>
#include <stdio.h>
#include <task.h>

#include "assoc.h"
#include "service.h"
#include "udp_inet.h"

#define NAME       "udp"

static udp_assocs_dep_t udp_assocs_dep = {
	.get_srcaddr = udp_get_srcaddr,
	.transmit_msg = udp_transmit_msg
};

static errno_t udp_init(void)
{
	errno_t rc;

	log_msg(LOG_DEFAULT, LVL_DEBUG, "udp_init()");

	rc = udp_assocs_init(&udp_assocs_dep);
	if (rc != EOK) {
		log_msg(LOG_DEFAULT, LVL_ERROR, "Failed initializing associations.");
		return ENOMEM;
	}

	rc = udp_inet_init();
	if (rc != EOK) {
		log_msg(LOG_DEFAULT, LVL_ERROR, "Failed connecting to internet service.");
		return ENOENT;
	}

	rc = udp_service_init();
	if (rc != EOK) {
		log_msg(LOG_DEFAULT, LVL_ERROR, "Failed initializing UDP service.");
		return ENOENT;
	}

	return EOK;
}

int main(int argc, char **argv)
{
	errno_t rc;

	printf(NAME ": UDP (User Datagram Protocol) service\n");

	rc = log_init(NAME);
	if (rc != EOK) {
		printf(NAME ": Failed to initialize log.\n");
		return 1;
	}

	rc = udp_init();
	if (rc != EOK)
		return 1;

	printf(NAME ": Accepting connections.\n");
	task_retval(0);
	async_manager();

	/* Not reached */
	return 0;
}

/**
 * @}
 */
