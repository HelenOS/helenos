/*
 * SPDX-FileCopyrightText: 2008 Jiri Svoboda
 *
 * SPDX-License-Identifier: BSD-3-Clause
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
