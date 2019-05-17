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

/** @addtogroup loader
 * @{
 */
/**
 * @file
 *
 * The program loader is a special init binary. Its image is used
 * to create a new task upon a @c task_spawn syscall. The syscall
 * returns the id of a phone connected to the newly created task.
 *
 * The caller uses this phone to send the pathname and various other
 * information to the loader. This is normally done by the C library
 * and completely hidden from applications.
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stddef.h>
#include <ipc/services.h>
#include <ipc/loader.h>
#include <ns.h>
#include <loader/pcb.h>
#include <entry_point.h>
#include <errno.h>
#include <async.h>
#include <str.h>
#include <as.h>
#include <elf/elf.h>
#include <elf/elf_load.h>
#include <vfs/vfs.h>
#include <vfs/inbox.h>
#include <libc.h>

#ifdef CONFIG_RTLD
#include <rtld/rtld.h>
#endif

#define DPRINTF(...) ((void) 0)

/** File that will be loaded */
static char *progname = NULL;
static int program_fd = -1;

/** The Program control block */
static pcb_t pcb;

/** Current working directory */
static char *cwd = NULL;

/** Number of arguments */
static int argc = 0;
/** Argument vector */
static char **argv = NULL;
/** Buffer holding all arguments */
static char *arg_buf = NULL;

/** Inbox entries. */
static struct pcb_inbox_entry inbox[INBOX_MAX_ENTRIES];
static int inbox_entries = 0;

static elf_info_t prog_info;

/** Used to limit number of connections to one. */
static bool connected = false;

static void ldr_get_taskid(ipc_call_t *req)
{
	ipc_call_t call;
	task_id_t task_id;
	size_t len;

	task_id = task_get_id();

	if (!async_data_read_receive(&call, &len)) {
		async_answer_0(&call, EINVAL);
		async_answer_0(req, EINVAL);
		return;
	}

	if (len > sizeof(task_id))
		len = sizeof(task_id);

	DPRINTF("LOADER_GET_TASKID() = %lu\n", (unsigned long) task_id);
	async_data_read_finalize(&call, &task_id, len);
	async_answer_0(req, EOK);
}

/** Receive a call setting the current working directory.
 *
 */
static void ldr_set_cwd(ipc_call_t *req)
{
	char *buf;
	errno_t rc = async_data_write_accept((void **) &buf, true, 0, 0, 0, NULL);

	if (rc == EOK) {
		if (cwd != NULL)
			free(cwd);

		cwd = buf;
	}

	DPRINTF("LOADER_SET_CWD('%s')\n", cwd);
	async_answer_0(req, rc);
}

/** Receive a call setting the program to execute.
 *
 */
static void ldr_set_program(ipc_call_t *req)
{
	ipc_call_t call;
	size_t namesize;
	if (!async_data_write_receive(&call, &namesize)) {
		async_answer_0(req, EINVAL);
		return;
	}

	char *name = malloc(namesize);
	// FIXME: check return value

	errno_t rc = async_data_write_finalize(&call, name, namesize);
	if (rc != EOK) {
		async_answer_0(req, EINVAL);
		return;
	}

	int file;
	rc = vfs_receive_handle(true, &file);
	if (rc != EOK) {
		async_answer_0(req, EINVAL);
		return;
	}

	DPRINTF("LOADER_SET_PROGRAM('%s')\n", name);

	progname = name;
	program_fd = file;
	async_answer_0(req, EOK);
}

/** Receive a call setting arguments of the program to execute.
 *
 */
static void ldr_set_args(ipc_call_t *req)
{
	char *buf;
	size_t buf_size;
	errno_t rc = async_data_write_accept((void **) &buf, true, 0, 0, 0, &buf_size);

	if (rc == EOK) {
		/*
		 * Count number of arguments
		 */
		char *cur = buf;
		int count = 0;

		while (cur < buf + buf_size) {
			size_t arg_size = str_size(cur);
			cur += arg_size + 1;
			count++;
		}

		/*
		 * Allocate new argv
		 */
		char **_argv = (char **) malloc((count + 1) * sizeof(char *));
		if (_argv == NULL) {
			free(buf);
			async_answer_0(req, ENOMEM);
			return;
		}

		/*
		 * Fill the new argv with argument pointers
		 */
		cur = buf;
		count = 0;
		while (cur < buf + buf_size) {
			_argv[count] = cur;

			size_t arg_size = str_size(cur);
			cur += arg_size + 1;
			count++;
		}
		_argv[count] = NULL;

		/*
		 * Copy temporary data to global variables
		 */
		if (arg_buf != NULL)
			free(arg_buf);

		if (argv != NULL)
			free(argv);

		for (int i = 0; i < count; i++)
			DPRINTF("LOADER_SET_ARGS('%s')\n", _argv[i]);

		argc = count;
		arg_buf = buf;
		argv = _argv;
	}

	async_answer_0(req, rc);
}

/** Receive a call setting inbox files of the program to execute.
 *
 */
static void ldr_add_inbox(ipc_call_t *req)
{
	if (inbox_entries == INBOX_MAX_ENTRIES) {
		async_answer_0(req, ERANGE);
		return;
	}

	ipc_call_t call;
	size_t namesize;
	if (!async_data_write_receive(&call, &namesize)) {
		async_answer_0(req, EINVAL);
		return;
	}

	char *name = malloc(namesize);
	errno_t rc = async_data_write_finalize(&call, name, namesize);
	if (rc != EOK) {
		async_answer_0(req, EINVAL);
		return;
	}

	int file;
	rc = vfs_receive_handle(true, &file);
	if (rc != EOK) {
		async_answer_0(req, EINVAL);
		return;
	}

	DPRINTF("LOADER_ADD_INBOX('%s')\n", name);

	/*
	 * We need to set the root early for dynamically linked binaries so
	 * that the loader can use it too.
	 */
	if (str_cmp(name, "root") == 0)
		vfs_root_set(file);

	inbox[inbox_entries].name = name;
	inbox[inbox_entries].file = file;
	inbox_entries++;
	async_answer_0(req, EOK);
}

/** Load the previously selected program.
 *
 * @return 0 on success, !0 on error.
 *
 */
static int ldr_load(ipc_call_t *req)
{
	DPRINTF("LOADER_LOAD()\n");

	errno_t rc = elf_load(program_fd, &prog_info);
	if (rc != EOK) {
		DPRINTF("Failed to load executable for '%s'.\n", progname);
		async_answer_0(req, EINVAL);
		return 1;
	}

	DPRINTF("Loaded.\n");

#ifdef CONFIG_RTLD
	if (prog_info.env) {
		pcb.tcb = rtld_tls_make(prog_info.env);
	} else {
		pcb.tcb = tls_make(prog_info.finfo.base);
	}
#else
	pcb.tcb = tls_make(prog_info.finfo.base);
#endif

	if (!pcb.tcb) {
		DPRINTF("Failed to make TLS for '%s'.\n", progname);
		async_answer_0(req, ENOMEM);
		return 1;
	}

	elf_set_pcb(&prog_info, &pcb);

	DPRINTF("PCB set.\n");

	pcb.cwd = cwd;

	pcb.argc = argc;
	pcb.argv = argv;

	pcb.inbox = inbox;
	pcb.inbox_entries = inbox_entries;

	DPRINTF("Answering.\n");
	async_answer_0(req, EOK);
	return 0;
}

/** Run the previously loaded program.
 *
 * @return 0 on success, !0 on error.
 *
 */
static __attribute__((noreturn)) void ldr_run(ipc_call_t *req)
{
	DPRINTF("Set task name\n");

	/* Set the task name. */
	task_set_name(progname);

	/* Run program */
	DPRINTF("Reply OK\n");
	async_answer_0(req, EOK);

	/*
	 * Wait for the hangup from the other side in order not to leave any
	 * unanswered IPC_M_PHONE_HUNGUP messages behind.
	 */
	async_get_call(req);
	assert(!ipc_get_imethod(req));
	async_answer_0(req, EOK);

	DPRINTF("Jump to entry point at %p\n", pcb.entry);

	__libc_fini();
	__tcb_reset();
	entry_point_jmp(prog_info.finfo.entry, &pcb);

	/* Not reached */
}

/** Handle loader connection.
 *
 * Receive and carry out commands (of which the last one should be
 * to execute the loaded program).
 *
 */
static void ldr_connection(ipc_call_t *icall, void *arg)
{
	/* Already have a connection? */
	if (connected) {
		async_answer_0(icall, ELIMIT);
		return;
	}

	connected = true;

	/* Accept the connection */
	async_accept_0(icall);

	/* Ignore parameters, the connection is already open */
	(void) icall;

	while (true) {
		errno_t retval;
		ipc_call_t call;
		async_get_call(&call);

		if (!ipc_get_imethod(&call)) {
			async_answer_0(&call, EOK);
			exit(0);
		}

		switch (ipc_get_imethod(&call)) {
		case LOADER_GET_TASKID:
			ldr_get_taskid(&call);
			continue;
		case LOADER_SET_CWD:
			ldr_set_cwd(&call);
			continue;
		case LOADER_SET_PROGRAM:
			ldr_set_program(&call);
			continue;
		case LOADER_SET_ARGS:
			ldr_set_args(&call);
			continue;
		case LOADER_ADD_INBOX:
			ldr_add_inbox(&call);
			continue;
		case LOADER_LOAD:
			ldr_load(&call);
			continue;
		case LOADER_RUN:
			ldr_run(&call);
			/* Not reached */
		default:
			retval = EINVAL;
			break;
		}

		async_answer_0(&call, retval);
	}
}

/** Program loader main function.
 */
int main(int argc, char *argv[])
{
	/* Introduce this task to the NS (give it our task ID). */
	task_id_t id = task_get_id();
	errno_t rc = ns_intro(id);
	if (rc != EOK)
		return rc;

	/* Register at naming service. */
	rc = service_register(SERVICE_LOADER, INTERFACE_LOADER,
	    ldr_connection, NULL);
	if (rc != EOK)
		return rc;

	async_manager();

	/* Never reached */
	return 0;
}

/** @}
 */
