/*
 * Copyright (c) 2015 Michal Koutny
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
/** @file
 */

#include <errno.h>
#include <ipc/taskman.h>
#include <taskman.h>

#include <stdio.h>

//TODO better filename?
#include "private/ns.h"

static int taskman_ask_callback(async_sess_t *session_tm)
{
	async_exch_t *exch = async_exchange_begin(session_tm);
	int rc = async_connect_to_me(
	    exch, TASKMAN_LOADER_CALLBACK, 0, 0, NULL, NULL);
	async_exchange_end(exch);

	return rc;
}

static async_sess_t *taskman_connect_to_ns(async_sess_t *session_tm)
{
	async_exch_t *exch = async_exchange_begin(session_tm);
	async_sess_t *session_ns = async_connect_me_to(EXCHANGE_ATOMIC,
	    exch, TASKMAN_LOADER_TO_NS, 0, 0);
	async_exchange_end(exch);

	return session_ns;
}

/** Set up phones upon being spawned by taskman
 *
 * Assumes primary session exists that is connected to taskman.
 * After handshake, taskman is connected to us (see, it's opposite) and broker
 * session is set up according to taskman.
 *
 *
 * @return Session to broker (naming service) or NULL (sets errno).
 */
async_sess_t *taskman_handshake(void)
{
	printf("%s:%i\n", __func__, __LINE__);

	int rc = taskman_ask_callback(session_primary);
	if (rc != EOK) {
		errno = rc;
		return NULL;
	}

	async_sess_t *session_ns = taskman_connect_to_ns(session_primary);
	if (session_ns == NULL) {
		errno = ENOENT;
	}

	return session_ns;
}

/** @}
 */
