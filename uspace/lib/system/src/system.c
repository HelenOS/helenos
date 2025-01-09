/*
 * Copyright (c) 2024 Jiri Svoboda
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

/** @addtogroup libsystem
 * @{
 */
/** @file System control service interface
 */

#include <async.h>
#include <assert.h>
#include <system.h>
#include <errno.h>
#include <fibril_synch.h>
#include <ipc/system.h>
#include <loc.h>
#include <stdlib.h>
#include "../private/system.h"

static errno_t system_callback_create(system_t *);
static void system_cb_conn(ipc_call_t *, void *);

/** Open system control service.
 *
 * @param svcname Service name
 * @param cb Pointer to callback structure
 * @param arg Argument passed to callback functions
 * @param rsystem Place to store pointer to system control service object.
 *
 * @return EOK on success or an error code
 */
errno_t system_open(const char *svcname, system_cb_t *cb, void *arg,
    system_t **rsystem)
{
	service_id_t system_svc;
	async_sess_t *sess;
	system_t *system;
	errno_t rc;

	system = calloc(1, sizeof(system_t));
	if (system == NULL)
		return ENOMEM;

	fibril_mutex_initialize(&system->lock);
	fibril_condvar_initialize(&system->cv);

	rc = loc_service_get_id(svcname, &system_svc, IPC_FLAG_BLOCKING);
	if (rc != EOK)
		goto error;

	sess = loc_service_connect(system_svc, INTERFACE_SYSTEM,
	    IPC_FLAG_BLOCKING);
	if (sess == NULL)
		goto error;

	system->sess = sess;
	system->cb = cb;
	system->cb_arg = arg;

	rc = system_callback_create(system);
	if (rc != EOK) {
		async_hangup(sess);
		goto error;
	}

	*rsystem = system;
	return EOK;
error:
	free(system);
	return rc;
}

/** Close system control service.
 *
 * @param system System control service
 */
void system_close(system_t *system)
{
	fibril_mutex_lock(&system->lock);
	async_hangup(system->sess);
	system->sess = NULL;

	/* Wait for callback handler to terminate */

	while (!system->cb_done)
		fibril_condvar_wait(&system->cv, &system->lock);
	fibril_mutex_unlock(&system->lock);

	free(system);
}

/** Create callback connection from system control service.
 *
 * @param system System control service
 * @return EOK on success or an error code
 */
static errno_t system_callback_create(system_t *system)
{
	async_exch_t *exch = async_exchange_begin(system->sess);

	aid_t req = async_send_0(exch, SYSTEM_CALLBACK_CREATE, NULL);

	port_id_t port;
	errno_t rc = async_create_callback_port(exch, INTERFACE_SYSTEM_CB, 0, 0,
	    system_cb_conn, system, &port);

	async_exchange_end(exch);

	if (rc != EOK)
		return rc;

	errno_t retval;
	async_wait_for(req, &retval);

	return retval;
}

/** Shut the system down.
 *
 * This function is asynchronous. It returns immediately with success
 * if the system started shutting down. Once shutdown is completed,
 * the @c shutdown_complete callback is executed. If the shutdown fails,
 * the @c shutdown_fail callback is executed.
 *
 * @param system System control service
 * @return EOK on succes or an error code
 */
errno_t system_shutdown(system_t *system)
{
	async_exch_t *exch = async_exchange_begin(system->sess);
	errno_t rc = async_req_0_0(exch, SYSTEM_SHUTDOWN);
	async_exchange_end(exch);

	return rc;
}

/** System shutdown completed.
 *
 * @param system System control service
 * @param icall Call data
 */
static void system_shutdown_complete(system_t *system, ipc_call_t *icall)
{
	if (system->cb != NULL && system->cb->shutdown_complete != NULL)
		system->cb->shutdown_complete(system->cb_arg);

	async_answer_0(icall, EOK);
}

/** System shutdown failed.
 *
 * @param system System control service
 * @param icall Call data
 */
static void system_shutdown_failed(system_t *system, ipc_call_t *icall)
{
	if (system->cb != NULL && system->cb->shutdown_complete != NULL)
		system->cb->shutdown_failed(system->cb_arg);

	async_answer_0(icall, EOK);
}

/** Callback connection handler.
 *
 * @param icall Connect call data
 * @param arg   Argument, system_t *
 */
static void system_cb_conn(ipc_call_t *icall, void *arg)
{
	system_t *system = (system_t *) arg;

	while (true) {
		ipc_call_t call;
		async_get_call(&call);

		if (!ipc_get_imethod(&call)) {
			/* Hangup */
			async_answer_0(&call, EOK);
			goto out;
		}

		switch (ipc_get_imethod(&call)) {
		case SYSTEM_SHUTDOWN_COMPLETE:
			system_shutdown_complete(system, &call);
			break;
		case SYSTEM_SHUTDOWN_FAILED:
			system_shutdown_failed(system, &call);
			break;
		default:
			async_answer_0(&call, ENOTSUP);
			break;
		}
	}

out:
	fibril_mutex_lock(&system->lock);
	system->cb_done = true;
	fibril_mutex_unlock(&system->lock);
	fibril_condvar_broadcast(&system->cv);
}

/** @}
 */
