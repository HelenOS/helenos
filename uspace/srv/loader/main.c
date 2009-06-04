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
 * @brief Loads and runs programs from VFS.
 * @{
 */
/**
 * @file
 * @brief Loads and runs programs from VFS.
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
#include <unistd.h>
#include <bool.h>
#include <fcntl.h>
#include <sys/types.h>
#include <ipc/ipc.h>
#include <ipc/services.h>
#include <ipc/loader.h>
#include <loader/pcb.h>
#include <errno.h>
#include <async.h>
#include <string.h>
#include <as.h>

#include <elf.h>
#include <elf_load.h>

#define DPRINTF(...)

/** Pathname of the file that will be loaded */
static char *pathname = NULL;

/** The Program control block */
static pcb_t pcb;

/** Number of arguments */
static int argc = 0;
/** Argument vector */
static char **argv = NULL;
/** Buffer holding all arguments */
static char *arg_buf = NULL;

/** Number of preset files */
static int filc = 0;
/** Preset files vector */
static char **filv = NULL;
/** Buffer holding all preset files */
static fdi_node_t *fil_buf = NULL;

static elf_info_t prog_info;
static elf_info_t interp_info;

static bool is_dyn_linked;

/** Used to limit number of connections to one. */
static bool connected;

static void ldr_get_taskid(ipc_callid_t rid, ipc_call_t *request)
{
	ipc_callid_t callid;
	task_id_t task_id;
	size_t len;
	
	task_id = task_get_id();
	
	if (!ipc_data_read_receive(&callid, &len)) {
		ipc_answer_0(callid, EINVAL);
		ipc_answer_0(rid, EINVAL);
		return;
	}
	
	if (len > sizeof(task_id))
		len = sizeof(task_id);
	
	ipc_data_read_finalize(callid, &task_id, len);
	ipc_answer_0(rid, EOK);
}


/** Receive a call setting pathname of the program to execute.
 *
 * @param rid
 * @param request
 */
static void ldr_set_pathname(ipc_callid_t rid, ipc_call_t *request)
{
	ipc_callid_t callid;
	size_t len;
	char *name_buf;
	
	if (!ipc_data_write_receive(&callid, &len)) {
		ipc_answer_0(callid, EINVAL);
		ipc_answer_0(rid, EINVAL);
		return;
	}
	
	name_buf = malloc(len + 1);
	if (!name_buf) {
		ipc_answer_0(callid, ENOMEM);
		ipc_answer_0(rid, ENOMEM);
		return;
	}
	
	ipc_data_write_finalize(callid, name_buf, len);
	ipc_answer_0(rid, EOK);
	
	if (pathname != NULL) {
		free(pathname);
		pathname = NULL;
	}
	
	name_buf[len] = '\0';
	pathname = name_buf;
}

/** Receive a call setting arguments of the program to execute.
 *
 * @param rid
 * @param request
 */
static void ldr_set_args(ipc_callid_t rid, ipc_call_t *request)
{
	ipc_callid_t callid;
	size_t buf_size, arg_size;
	char *p;
	int n;
	
	if (!ipc_data_write_receive(&callid, &buf_size)) {
		ipc_answer_0(callid, EINVAL);
		ipc_answer_0(rid, EINVAL);
		return;
	}
	
	if (arg_buf != NULL) {
		free(arg_buf);
		arg_buf = NULL;
	}
	
	if (argv != NULL) {
		free(argv);
		argv = NULL;
	}
	
	arg_buf = malloc(buf_size + 1);
	if (!arg_buf) {
		ipc_answer_0(callid, ENOMEM);
		ipc_answer_0(rid, ENOMEM);
		return;
	}
	
	ipc_data_write_finalize(callid, arg_buf, buf_size);
	
	arg_buf[buf_size] = '\0';
	
	/*
	 * Count number of arguments
	 */
	p = arg_buf;
	n = 0;
	while (p < arg_buf + buf_size) {
		arg_size = str_size(p);
		p = p + arg_size + 1;
		++n;
	}
	
	/* Allocate argv */
	argv = malloc((n + 1) * sizeof(char *));
	
	if (argv == NULL) {
		free(arg_buf);
		ipc_answer_0(rid, ENOMEM);
		return;
	}

	/*
	 * Fill argv with argument pointers
	 */
	p = arg_buf;
	n = 0;
	while (p < arg_buf + buf_size) {
		argv[n] = p;
		
		arg_size = str_size(p);
		p = p + arg_size + 1;
		++n;
	}
	
	argc = n;
	argv[n] = NULL;

	ipc_answer_0(rid, EOK);
}

/** Receive a call setting preset files of the program to execute.
 *
 * @param rid
 * @param request
 */
static void ldr_set_files(ipc_callid_t rid, ipc_call_t *request)
{
	ipc_callid_t callid;
	size_t buf_size;
	if (!ipc_data_write_receive(&callid, &buf_size)) {
		ipc_answer_0(callid, EINVAL);
		ipc_answer_0(rid, EINVAL);
		return;
	}
	
	if ((buf_size % sizeof(fdi_node_t)) != 0) {
		ipc_answer_0(callid, EINVAL);
		ipc_answer_0(rid, EINVAL);
		return;
	}
	
	if (fil_buf != NULL) {
		free(fil_buf);
		fil_buf = NULL;
	}
	
	if (filv != NULL) {
		free(filv);
		filv = NULL;
	}
	
	fil_buf = malloc(buf_size);
	if (!fil_buf) {
		ipc_answer_0(callid, ENOMEM);
		ipc_answer_0(rid, ENOMEM);
		return;
	}
	
	ipc_data_write_finalize(callid, fil_buf, buf_size);
	
	int count = buf_size / sizeof(fdi_node_t);
	
	/* Allocate filvv */
	filv = malloc((count + 1) * sizeof(fdi_node_t *));
	
	if (filv == NULL) {
		free(fil_buf);
		ipc_answer_0(rid, ENOMEM);
		return;
	}
	
	/*
	 * Fill filv with argument pointers
	 */
	int i;
	for (i = 0; i < count; i++)
		filv[i] = &fil_buf[i];
	
	filc = count;
	filv[count] = NULL;
	
	ipc_answer_0(rid, EOK);
}

/** Load the previously selected program.
 *
 * @param rid
 * @param request
 * @return 0 on success, !0 on error.
 */
static int ldr_load(ipc_callid_t rid, ipc_call_t *request)
{
	int rc;
	
	rc = elf_load_file(pathname, 0, &prog_info);
	if (rc != EE_OK) {
		DPRINTF("Failed to load executable '%s'.\n", pathname);
		ipc_answer_0(rid, EINVAL);
		return 1;
	}
	
	elf_create_pcb(&prog_info, &pcb);
	
	pcb.argc = argc;
	pcb.argv = argv;
	
	pcb.filc = filc;
	pcb.filv = filv;
	
	if (prog_info.interp == NULL) {
		/* Statically linked program */
		is_dyn_linked = false;
		ipc_answer_0(rid, EOK);
		return 0;
	}
	
	rc = elf_load_file(prog_info.interp, 0, &interp_info);
	if (rc != EE_OK) {
		DPRINTF("Failed to load interpreter '%s.'\n",
		    prog_info.interp);
		ipc_answer_0(rid, EINVAL);
		return 1;
	}
	
	is_dyn_linked = true;
	ipc_answer_0(rid, EOK);
	
	return 0;
}


/** Run the previously loaded program.
 *
 * @param rid
 * @param request
 * @return 0 on success, !0 on error.
 */
static void ldr_run(ipc_callid_t rid, ipc_call_t *request)
{
	const char *cp;
	
	/* Set the task name. */
	cp = str_rchr(pathname, '/');
	cp = (cp == NULL) ? pathname : (cp + 1);
	task_set_name(cp);
	
	if (is_dyn_linked == true) {
		/* Dynamically linked program */
		DPRINTF("Run ELF interpreter.\n");
		DPRINTF("Entry point: 0x%lx\n", interp_info.entry);
		
		ipc_answer_0(rid, EOK);
		elf_run(&interp_info, &pcb);
	} else {
		/* Statically linked program */
		ipc_answer_0(rid, EOK);
		elf_run(&prog_info, &pcb);
	}
	
	/* Not reached */
}

/** Handle loader connection.
 *
 * Receive and carry out commands (of which the last one should be
 * to execute the loaded program).
 */
static void ldr_connection(ipc_callid_t iid, ipc_call_t *icall)
{
	ipc_callid_t callid;
	ipc_call_t call;
	int retval;
	
	/* Already have a connection? */
	if (connected) {
		ipc_answer_0(iid, ELIMIT);
		return;
	}
	
	connected = true;
	
	/* Accept the connection */
	ipc_answer_0(iid, EOK);
	
	/* Ignore parameters, the connection is already open */
	(void) iid;
	(void) icall;
	
	while (1) {
		callid = async_get_call(&call);
		
		switch (IPC_GET_METHOD(call)) {
		case IPC_M_PHONE_HUNGUP:
			exit(0);
		case LOADER_GET_TASKID:
			ldr_get_taskid(callid, &call);
			continue;
		case LOADER_SET_PATHNAME:
			ldr_set_pathname(callid, &call);
			continue;
		case LOADER_SET_ARGS:
			ldr_set_args(callid, &call);
			continue;
		case LOADER_SET_FILES:
			ldr_set_files(callid, &call);
			continue;
		case LOADER_LOAD:
			ldr_load(callid, &call);
			continue;
		case LOADER_RUN:
			ldr_run(callid, &call);
			/* Not reached */
		default:
			retval = ENOENT;
			break;
		}
		if ((callid & IPC_CALLID_NOTIFICATION) == 0 &&
		    IPC_GET_METHOD(call) != IPC_M_PHONE_HUNGUP) {
			DPRINTF("Responding EINVAL to method %d.\n",
			    IPC_GET_METHOD(call));
			ipc_answer_0(callid, EINVAL);
		}
	}
}

/** Program loader main function.
 */
int main(int argc, char *argv[])
{
	ipcarg_t phonead;
	
	connected = false;
	
	/* Set a handler of incomming connections. */
	async_set_client_connection(ldr_connection);
	
	/* Register at naming service. */
	if (ipc_connect_to_me(PHONE_NS, SERVICE_LOAD, 0, 0, &phonead) != 0) 
		return -1;
	
	async_manager();
	
	/* Never reached */
	return 0;
}

/** @}
 */
