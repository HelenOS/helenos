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

/** @addtogroup libc
 * @{
 */

/** @file
 */

#include <errno.h>
#include <libc.h>
#include <stdbool.h>
#include <stdlib.h>
#include <tls.h>
#include <fibril.h>
#include <fibril_synch.h>
#include <task.h>
#include <loader/pcb.h>
#include <vfs/vfs.h>
#include <vfs/inbox.h>
#include <io/kio.h>
#include "private/libc.h"
#include "private/async.h"
#include "private/malloc.h"
#include "private/io.h"
#include "private/fibril.h"

#ifdef CONFIG_RTLD
#include <rtld/rtld.h>
#endif

progsymbols_t __progsymbols;

static bool env_setup;
static fibril_t main_fibril;

void __libc_main(void *pcb_ptr)
{
	__kio_init();

	assert(!__tcb_is_set());

	__pcb = (pcb_t *) pcb_ptr;

	if (__pcb) {
		main_fibril.tcb = __pcb->tcb;
	} else {
		/*
		 * Loaded by kernel, not the loader.
		 * Kernel only supports loading fully static binaries,
		 * so we can do basic initialization without worrying about
		 * dynamic libraries.
		 */

		main_fibril.tcb = tls_make_initial(__progsymbols.elfstart);
	}

	assert(main_fibril.tcb);

	__fibrils_init();
	__fibril_synch_init();

	/* Initialize the fibril. */
	main_fibril.tcb->fibril_data = &main_fibril;
	__tcb_set(main_fibril.tcb);
	fibril_setup(&main_fibril);

	/* Initialize user task run-time environment */
	__malloc_init();

#ifdef CONFIG_RTLD
	if (__pcb != NULL && __pcb->rtld_runtime != NULL) {
		runtime_env = (rtld_t *) __pcb->rtld_runtime;
	} else {
		if (rtld_init_static() != EOK)
			abort();
	}
#endif

	__async_server_init();
	__async_client_init();
	__async_ports_init();

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
	 * C++ Static constructor calls.
	 */

	if (__progsymbols.preinit_array) {
		for (int i = __progsymbols.preinit_array_len - 1; i >= 0; --i)
			__progsymbols.preinit_array[i]();
	}

	if (__progsymbols.init_array) {
		for (int i = __progsymbols.init_array_len - 1; i >= 0; --i)
			__progsymbols.init_array[i]();
	}

	/*
	 * Run main() and set task return value
	 * according the result
	 */
	int retval = __progsymbols.main(argc, argv);
	exit(retval);
}

void __libc_fini(void)
{
	__async_client_fini();
	__async_server_fini();
	__async_ports_fini();

	__fibril_synch_fini();
	__fibrils_fini();

	__malloc_fini();

	__kio_fini();
}

void __libc_exit(int status)
{
	/*
	 * GCC extension __attribute__((destructor)),
	 * C++ destructors are added to __cxa_finalize call
	 * when the respective constructor is called.
	 */

	for (int i = 0; i < __progsymbols.fini_array_len; ++i)
		__progsymbols.fini_array[i]();

	if (env_setup) {
		__stdio_done();
		task_retval(status);
	}

	__SYSCALL1(SYS_TASK_EXIT, false);
	__builtin_unreachable();
}

void __libc_abort(void)
{
	__SYSCALL1(SYS_TASK_EXIT, true);
	__builtin_unreachable();
}

/** @}
 */
