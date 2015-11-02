/*
 * Copyright (c) 2011 Martin Decky
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

#include <ns.h>
#include <ipc/ns.h>
#include <async.h>
#include <macros.h>
#include <errno.h>
#include "private/ns.h"

int service_register(service_t service)
{
	async_exch_t *exch = async_exchange_begin(session_ns);
	int rc = async_connect_to_me(exch, 0, service, 0);
	async_exchange_end(exch);
	
	return rc;
}


async_sess_t *service_connect(service_t service, iface_t iface, sysarg_t arg3)
{
	async_exch_t *exch = async_exchange_begin(session_ns);
	if (!exch)
		return NULL;
	
	async_sess_t *sess =
	    async_connect_me_to_iface(exch, iface, service, arg3);
	async_exchange_end(exch);
	
	if (!sess)
		return NULL;
	
	/*
	 * FIXME Ugly hack to work around limitation of implementing
	 * parallel exchanges using multiple connections. Shift out
	 * first argument for non-initial connections.
	 */
	async_sess_args_set(sess, iface, arg3, 0);
	
	return sess;
}

async_sess_t *service_connect_blocking(service_t service, iface_t iface,
    sysarg_t arg3)
{
	async_exch_t *exch = async_exchange_begin(session_ns);
	async_sess_t *sess =
	    async_connect_me_to_blocking_iface(exch, iface, service, arg3);
	async_exchange_end(exch);
	
	if (!sess)
		return NULL;
	
	/*
	 * FIXME Ugly hack to work around limitation of implementing
	 * parallel exchanges using multiple connections. Shift out
	 * first argument for non-initial connections.
	 */
	async_sess_args_set(sess, iface, arg3, 0);
	
	return sess;
}


int ns_ping(void)
{
	async_exch_t *exch = async_exchange_begin(session_ns);
	int rc = async_req_0_0(exch, NS_PING);
	async_exchange_end(exch);
	
	return rc;
}

int ns_intro(task_id_t id)
{
	async_exch_t *exch = async_exchange_begin(session_ns);
	int rc = async_req_2_0(exch, NS_ID_INTRO, LOWER32(id), UPPER32(id));
	async_exchange_end(exch);
	
	return rc;
}

/** @}
 */
