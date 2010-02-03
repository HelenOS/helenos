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
#include <ipc/ns.h>
#include <macros.h>
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

/** Current working directory */
static char *cwd = NULL;

/** Number of arguments */
static int argc = 0;
/** Argument vector */
static char **argv = NULL;
/** Buffer holding all arguments */
static char *arg_buf = NULL;

/** Number of preset files */
static int filc = 0;
/** Preset files vector */
static fdi_node_t **filv = NULL;
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
	
	if (!async_data_read_receive(&callid, &len)) {
		ipc_answer_0(callid, EINVAL);
		ipc_answer_0(rid, EINVAL);
		return;
	}
	
	if (len > sizeof(task_id))
		len = sizeof(task_id);
	
	async_data_read_finalize(callid, &task_id, len);
	ipc_answer_0(rid, EOK);
}

/** Receive a call setting the current working directory.
 *
 * @param rid
 * @param request
 */
static void ldr_set_cwd(ipc_callid_t rid, ipc_call_t *request)
{
	char *buf;
	int rc = async_string_receive(&buf, 0, NULL);
	
	if (rc == EOK) {
		if (cwd != NULL)
			free(cwd);
		
		cwd = buf;
	}
	
	ipc_answer_0(rid, rc);
}

/** Receive a call setting pathname of the program to execute.
 *
 * @param rid
 * @param request
 */
static void ldr_set_pathname(ipc_callid_t rid, ipc_call_t *request)
{
	char *buf;
	int rc = async_string_receive(&buf, 0, NULL);
	
	if (rc == EOK) {
		if (pathname != NULL)
			free(pathname);
		
		pathname = buf;
	}
	
	ipc_answer_0(rid, rc);
}

/** Receive a call setting arguments of the program to execute.
 *
 * @param rid
 * @param request
 */
static void ldr_set_args(ipc_callid_t rid, ipc_call_t *request)
{
	char *buf;
	size_t buf_size;
	int rc = async_string_receive(&buf, 0, &buf_size);
	
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
			ipc_answer_0(rid, ENOMEM);
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
		
		argc = count;
		arg_buf = buf;
		argv = _argv;
	}
	
	ipc_answer_0(rid, rc);
}

/** Receive a call setting preset files of the program to execute.
 *
 * @param rid
 * @param request
 */
static void ldr_set_files(ipc_callid_t rid, ipc_call_t *request)
{
	fdi_node_t *buf;
	size_t buf_size;
	int rc = async_data_receive(&buf, 0, 0, sizeof(fdi_node_t), &buf_size);
	
	if (rc == EOK) {
		int count = buf_size / sizeof(fdi_node_t);
		
		/*
		 * Allocate new filv
		 */
		fdi_node_t **_filv = (fdi_node_t *) malloc((count + 1) * sizeof(fdi_node_t *));
		if (_filv == NULL) {
			free(buf);
			ipc_answer_0(rid, ENOMEM);
			return;
		}
		
		/*
		 * Fill the new filv with argument pointers
		 */
		int i;
		for (i = 0; i < count; i++)
			_filv[i] = &buf[i];
		
		_filv[count] = NULL;
		
		/*
		 * Copy temporary data to global variables
		 */
		if (fil_buf != NULL)
			free(fil_buf);
		
		if (filv != NULL)
			free(filv);
		
		filc = count;
		fil_buf = buf;
		filv = _filv;
	}
	
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
	
	pcb.cwd = cwd;
	
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
		DPRINTF("Entry point: %p\n", interp_info.entry);
		
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
		case LOADER_SET_CWD:
			ldr_set_cwd(callid, &call);
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
	task_id_t id;
	int rc;

	connected = false;

	/* Introduce this task to the NS (give it our task ID). */
	id = task_get_id();
	rc = async_req_2_0(PHONE_NS, NS_ID_INTRO, LOWER32(id), UPPER32(id));
	if (rc != EOK)
		return -1;

	/* Set a handler of incomming connections. */
	async_set_client_connection(ldr_connection);
	
	/* Register at naming service. */
	if (ipc_connect_to_me(PHONE_NS, SERVICE_LOAD, 0, 0, &phonead) != 0) 
		return -2;

	async_manager();
	
	/* Never reached */
	return 0;
}

/** @}
 */
