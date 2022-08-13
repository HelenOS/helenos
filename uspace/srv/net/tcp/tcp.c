/*
 * SPDX-FileCopyrightText: 2015 Jiri Svoboda
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup tcp
 * @{
 */

/**
 * @file TCP (Transmission Control Protocol) network module
 */

#include <async.h>
#include <errno.h>
#include <io/log.h>
#include <stdio.h>
#include <task.h>

#include "conn.h"
#include "inet.h"
#include "ncsim.h"
#include "rqueue.h"
#include "service.h"
#include "test.h"
#include "ucall.h"

#define NAME       "tcp"

static tcp_rqueue_cb_t tcp_rqueue_cb = {
	.seg_received = tcp_as_segment_arrived
};

static errno_t tcp_init(void)
{
	errno_t rc;

	log_msg(LOG_DEFAULT, LVL_DEBUG, "tcp_init()");

	rc = tcp_conns_init();
	if (rc != EOK) {
		assert(rc == ENOMEM);
		log_msg(LOG_DEFAULT, LVL_ERROR, "Failed initializing connections");
		return ENOMEM;
	}

	tcp_rqueue_init(&tcp_rqueue_cb);
	tcp_rqueue_fibril_start();

	tcp_ncsim_init();
	tcp_ncsim_fibril_start();

	if (0)
		tcp_test();

	rc = tcp_inet_init();
	if (rc != EOK)
		return ENOENT;

	rc = tcp_service_init();
	if (rc != EOK) {
		log_msg(LOG_DEFAULT, LVL_ERROR, "Failed initializing service.");
		return ENOENT;
	}

	return EOK;
}

int main(int argc, char **argv)
{
	errno_t rc;

	printf(NAME ": TCP (Transmission Control Protocol) network module\n");

	rc = log_init(NAME);
	if (rc != EOK) {
		printf(NAME ": Failed to initialize log.\n");
		return 1;
	}

	rc = tcp_init();
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
