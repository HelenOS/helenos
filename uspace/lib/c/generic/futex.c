/*
 * Copyright (c) 2008 Jakub Jermar
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

#include <futex.h>
#include <atomic.h>
#include <libarch/barrier.h>
#include <libc.h>
#include <sys/types.h>

/** Initialize futex counter.
 *
 * @param futex Futex.
 * @param val   Initialization value.
 *
 */
void futex_initialize(futex_t *futex, int val)
{
	atomic_set(futex, val);
}

/** Try to down the futex.
 *
 * @param futex Futex.
 *
 * @return Non-zero if the futex was acquired.
 * @return Zero if the futex was not acquired.
 *
 */
int futex_trydown(futex_t *futex)
{
	int rc;

	rc = cas(futex, 1, 0);
	CS_ENTER_BARRIER();

	return rc;
}

/** Down the futex.
 *
 * @param futex Futex.
 *
 * @return ENOENT if there is no such virtual address.
 * @return Zero in the uncontended case.
 * @return Otherwise one of ESYNCH_OK_ATOMIC or ESYNCH_OK_BLOCKED.
 *
 */
int futex_down(futex_t *futex)
{
	atomic_signed_t nv;

	nv = (atomic_signed_t) atomic_predec(futex);
	CS_ENTER_BARRIER();
	if (nv < 0)
		return __SYSCALL1(SYS_FUTEX_SLEEP, (sysarg_t) &futex->count);
	
	return 0;
}

/** Up the futex.
 *
 * @param futex Futex.
 *
 * @return ENOENT if there is no such virtual address.
 * @return Zero in the uncontended case.
 *
 */
int futex_up(futex_t *futex)
{
	CS_LEAVE_BARRIER();

	if ((atomic_signed_t) atomic_postinc(futex) < 0)
		return __SYSCALL1(SYS_FUTEX_WAKEUP, (sysarg_t) &futex->count);
	
	return 0;
}

/** @}
 */
