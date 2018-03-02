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
 * @brief HelenOS C library
 * @{
 * @}
 */

/** @addtogroup libc generic
 * @ingroup lc
 * @{
 */

/** @file
 */

#include <errno.h>
#include <libc.h>
#include <stdlib.h>
#include <tls.h>
#include <fibril.h>
#include <task.h>
#include <loader/pcb.h>
#include <vfs/vfs.h>
#include <vfs/inbox.h>
#include "private/libc.h"
#include "private/async.h"
#include "private/malloc.h"
#include "private/io.h"

#ifdef FUTEX_UPGRADABLE
#include <rcu.h>
#endif

#ifdef CONFIG_RTLD
#include <rtld/rtld.h>
#endif


static bool env_setup = false;

void __main(void *pcb_ptr)
{
	/* Initialize user task run-time environment */
	__malloc_init();

	/* Save the PCB pointer */
	__pcb = (pcb_t *) pcb_ptr;

#ifdef CONFIG_RTLD
	if (__pcb != NULL && __pcb->rtld_runtime != NULL) {
		runtime_env = (rtld_t *) __pcb->rtld_runtime;
	} else {
		if (rtld_init_static() != EOK)
			abort();
	}
#endif

	fibril_t *fibril = fibril_setup();
	if (fibril == NULL)
		abort();

	__tcb_set(fibril->tcb);


#ifdef FUTEX_UPGRADABLE
	rcu_register_fibril();
#endif

	__async_init();

	/* The basic run-time environment is setup */
	env_setup = true;

	int argc;
	char **argv;

	/*
	 * Get command line arguments and initialize
	 * standard input and output
	 */
	if (__pcb == NULL) {
		argc = 0;
		argv = NULL;
		__stdio_init();
	} else {
		argc = __pcb->argc;
		argv = __pcb->argv;
		__inbox_init(__pcb->inbox, __pcb->inbox_entries);
		__stdio_init();
		vfs_root_set(inbox_get("root"));
		(void) vfs_cwd_set(__pcb->cwd);
	}

	/*
	 * Run main() and set task return value
	 * according the result
	 */
	int retval = main(argc, argv);
	exit(retval);
}

void exit(int status)
{
	if (env_setup) {
		__stdio_done();
		task_retval(status);
		fibril_teardown(__tcb_get()->fibril_data, false);
	}

	__SYSCALL1(SYS_TASK_EXIT, false);

	/* Unreachable */
	while (1);
}

void abort(void)
{
	__SYSCALL1(SYS_TASK_EXIT, true);

	/* Unreachable */
	while (1);
}

/** @}
 */
