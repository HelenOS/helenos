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


#include <async.h>
#include <errno.h>
#include <ipc/common.h>
#include <ipc/taskman.h>
#include <task.h>
#include <taskman.h>

#include "private/async.h"
#include "private/taskman.h"

async_sess_t *session_taskman = NULL;

/*
 * Private functions
 */

void __task_init(async_sess_t *sess)
{
	assert(session_taskman == NULL);
	session_taskman = sess;
}

async_exch_t *taskman_exchange_begin(void)
{
	assert(session_taskman);

	async_exch_t *exch = async_exchange_begin(session_taskman);
	return exch;
}

void taskman_exchange_end(async_exch_t *exch)
{
	async_exchange_end(exch);
}

/** Wrap PHONE_INITIAL with session and introduce to taskman
 */
async_sess_t *taskman_connect(void)
{
	/*
	 *  EXCHANGE_ATOMIC would require single calls only,
	 *  EXCHANGE_PARALLEL not sure about implementation via multiple phones,
	 * >EXCHANGE_SERIALIZE perhaphs no harm, except the client serialization
	 */
	const exch_mgmt_t mgmt = EXCHANGE_SERIALIZE;
	async_sess_t *sess = create_session(PHONE_INITIAL, mgmt, 0, 0, 0);

	if (sess != NULL) {
		/* Introduce ourselves and ignore answer */
		async_exch_t *exch = async_exchange_begin(sess);
		aid_t req = async_send_0(exch, TASKMAN_NEW_TASK, NULL);
		async_exchange_end(exch);
		
		if (req) {
			async_forget(req);
		}
	}

	return sess;
}

/** Ask taskman to pass/share its NS */
async_sess_t *taskman_session_ns(void)
{
	assert(session_taskman);

	async_exch_t *exch = async_exchange_begin(session_taskman);
	assert(exch);

	async_sess_t *sess = async_connect_me_to(EXCHANGE_ATOMIC,
	    exch, TASKMAN_CONNECT_TO_NS, 0, 0);
	async_exchange_end(exch);

	return sess;
}

/** Ask taskman to connect to (a new) loader instance */
async_sess_t *taskman_session_loader(void)
{
	assert(session_taskman);

	async_exch_t *exch = async_exchange_begin(session_taskman);
	async_sess_t *sess = async_connect_me_to(EXCHANGE_SERIALIZE,
	    exch, TASKMAN_CONNECT_TO_LOADER, 0, 0);
	async_exchange_end(exch);

	return sess;
}

/*
 * Public functions
 */

async_sess_t *taskman_get_session(void)
{
	return session_taskman;
}

/** Introduce as loader to taskman
 *
 * @return EOK on success, otherwise propagated error code
 */
int taskman_intro_loader(void)
{
	assert(session_taskman);

	async_exch_t *exch = async_exchange_begin(session_taskman);
	int rc = async_connect_to_me(
	    exch, TASKMAN_LOADER_CALLBACK, 0, 0, NULL, NULL);
	async_exchange_end(exch);

	return rc;
}

/** Tell taskman we are his NS
 *
 * @return EOK on success, otherwise propagated error code
 */
int taskman_intro_ns(void)
{
	assert(session_taskman);

	async_exch_t *exch = async_exchange_begin(session_taskman);
	aid_t req = async_send_0(exch, TASKMAN_I_AM_NS, NULL);

	int rc = async_connect_to_me(exch, 0, 0, 0, NULL, NULL);
	taskman_exchange_end(exch);

	if (rc != EOK) {
		return rc;
	}

	sysarg_t retval;
	async_wait_for(req, &retval);
	return retval;
}



/** @}
 */
