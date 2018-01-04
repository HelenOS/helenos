/*
 * Copyright (c) 2015 Jiri Svoboda
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

	if (0) tcp_test();

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
