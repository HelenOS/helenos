/*
 * Copyright (c) 2005 Martin Decky
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

/** @addtogroup lc Libc
 * @brief	HelenOS C library
 * @{
 * @}
 */
/** @addtogroup libc generic
 * @ingroup lc
 * @{
 */
/** @file
 */ 

#include <libc.h>
#include <unistd.h>
#include <thread.h>
#include <malloc.h>
#include <psthread.h>
#include <io/stream.h>
#include <ipc/ipc.h>
#include <async.h>
#include <as.h>

extern char _heap;

void _exit(int status)
{
	thread_exit(status);
}

void __main(void)
{
	psthread_data_t *pt;

	(void) as_area_create(&_heap, 1, AS_AREA_WRITE | AS_AREA_READ);
	_async_init();
	pt = psthread_setup();
	__tcb_set(pt->tcb);
}

void __io_init(void)
{
	open("stdin", 0);
	open("stdout", 0);
	open("stderr", 0);
}

void __exit(void)
{
	psthread_teardown(__tcb_get()->pst_data);
	_exit(0);
}

/** @}
 */
