/*
 * Copyright (c) 2008 Jiri Svoboda
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

#include <udebug.h>
#include <stddef.h>
#include <stdint.h>
#include <abi/ipc/methods.h>
#include <async.h>

errno_t udebug_begin(async_sess_t *sess)
{
	async_exch_t *exch = async_exchange_begin(sess);
	errno_t rc = async_req_1_0(exch, IPC_M_DEBUG, UDEBUG_M_BEGIN);
	async_exchange_end(exch);

	return rc;
}

errno_t udebug_end(async_sess_t *sess)
{
	async_exch_t *exch = async_exchange_begin(sess);
	errno_t rc = async_req_1_0(exch, IPC_M_DEBUG, UDEBUG_M_END);
	async_exchange_end(exch);

	return rc;
}

errno_t udebug_set_evmask(async_sess_t *sess, udebug_evmask_t mask)
{
	async_exch_t *exch = async_exchange_begin(sess);
	errno_t rc = async_req_2_0(exch, IPC_M_DEBUG, UDEBUG_M_SET_EVMASK, mask);
	async_exchange_end(exch);

	return rc;
}

errno_t udebug_thread_read(async_sess_t *sess, void *buffer, size_t n,
    size_t *copied, size_t *needed)
{
	sysarg_t a_copied, a_needed;

	async_exch_t *exch = async_exchange_begin(sess);
	errno_t rc = async_req_3_3(exch, IPC_M_DEBUG, UDEBUG_M_THREAD_READ,
	    (sysarg_t) buffer, n, NULL, &a_copied, &a_needed);
	async_exchange_end(exch);

	*copied = (size_t) a_copied;
	*needed = (size_t) a_needed;

	return rc;
}

errno_t udebug_name_read(async_sess_t *sess, void *buffer, size_t n,
    size_t *copied, size_t *needed)
{
	sysarg_t a_copied, a_needed;

	async_exch_t *exch = async_exchange_begin(sess);
	errno_t rc = async_req_3_3(exch, IPC_M_DEBUG, UDEBUG_M_NAME_READ,
	    (sysarg_t) buffer, n, NULL, &a_copied, &a_needed);
	async_exchange_end(exch);

	*copied = (size_t) a_copied;
	*needed = (size_t) a_needed;

	return rc;
}

errno_t udebug_areas_read(async_sess_t *sess, void *buffer, size_t n,
    size_t *copied, size_t *needed)
{
	sysarg_t a_copied, a_needed;

	async_exch_t *exch = async_exchange_begin(sess);
	errno_t rc = async_req_3_3(exch, IPC_M_DEBUG, UDEBUG_M_AREAS_READ,
	    (sysarg_t) buffer, n, NULL, &a_copied, &a_needed);
	async_exchange_end(exch);

	*copied = (size_t) a_copied;
	*needed = (size_t) a_needed;

	return rc;
}

errno_t udebug_mem_read(async_sess_t *sess, void *buffer, uintptr_t addr, size_t n)
{
	async_exch_t *exch = async_exchange_begin(sess);
	errno_t rc = async_req_4_0(exch, IPC_M_DEBUG, UDEBUG_M_MEM_READ,
	    (sysarg_t) buffer, addr, n);
	async_exchange_end(exch);

	return rc;
}

errno_t udebug_args_read(async_sess_t *sess, thash_t tid, sysarg_t *buffer)
{
	async_exch_t *exch = async_exchange_begin(sess);
	errno_t rc = async_req_3_0(exch, IPC_M_DEBUG, UDEBUG_M_ARGS_READ,
	    tid, (sysarg_t) buffer);
	async_exchange_end(exch);

	return rc;
}

errno_t udebug_regs_read(async_sess_t *sess, thash_t tid, void *buffer)
{
	async_exch_t *exch = async_exchange_begin(sess);
	errno_t rc = async_req_3_0(exch, IPC_M_DEBUG, UDEBUG_M_REGS_READ,
	    tid, (sysarg_t) buffer);
	async_exchange_end(exch);

	return rc;
}

errno_t udebug_go(async_sess_t *sess, thash_t tid, udebug_event_t *ev_type,
    sysarg_t *val0, sysarg_t *val1)
{
	sysarg_t a_ev_type;

	async_exch_t *exch = async_exchange_begin(sess);
	errno_t rc = async_req_2_3(exch, IPC_M_DEBUG, UDEBUG_M_GO,
	    tid, &a_ev_type, val0, val1);
	async_exchange_end(exch);

	*ev_type = a_ev_type;
	return rc;
}

errno_t udebug_stop(async_sess_t *sess, thash_t tid)
{
	async_exch_t *exch = async_exchange_begin(sess);
	errno_t rc = async_req_2_0(exch, IPC_M_DEBUG, UDEBUG_M_STOP, tid);
	async_exchange_end(exch);

	return rc;
}

/** @}
 */
