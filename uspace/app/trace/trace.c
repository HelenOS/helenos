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

/** @addtogroup trace
 * @{
 */
/** @file
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <ipc/ipc.h>
#include <fibril.h>
#include <errno.h>
#include <udebug.h>
#include <async.h>
#include <task.h>
#include <loader/loader.h>

#include <libc.h>

// Temporary: service and method names
#include "proto.h"
#include <ipc/services.h>
#include "../../srv/vfs/vfs.h"
#include <ipc/console.h>

#include "syscalls.h"
#include "ipcp.h"
#include "errors.h"
#include "trace.h"

#define THBUF_SIZE 64
uintptr_t thread_hash_buf[THBUF_SIZE];
int n_threads;

int next_thread_id;

int phoneid;
int abort_trace;

uintptr_t thash;
volatile int paused;

void thread_trace_start(uintptr_t thread_hash);

static proto_t *proto_console;
static task_id_t task_id;
static loader_t *task_ldr;

/** Combination of events/data to print. */
display_mask_t display_mask;

static int program_run_fibril(void *arg);

static void program_run(void)
{
	fid_t fid;

	fid = fibril_create(program_run_fibril, NULL);
	if (fid == 0) {
		printf("Error creating fibril\n");
		exit(1);
	}

	fibril_add_ready(fid);
}

static int program_run_fibril(void *arg)
{
	int rc;

	/*
	 * This must be done in background as it will block until
	 * we let the task reply to this call.
	 */
	rc = loader_run(task_ldr);
	if (rc != 0) {
		printf("Error running program\n");
		exit(1);
	}

	free(task_ldr);
	task_ldr = NULL;

	printf("program_run_fibril exiting\n");
	return 0;
}


static int connect_task(task_id_t task_id)
{
	int rc;

	rc = ipc_connect_kbox(task_id);

	if (rc == ENOTSUP) {
		printf("You do not have userspace debugging support "
		    "compiled in the kernel.\n");
		printf("Compile kernel with 'Support for userspace debuggers' "
		    "(CONFIG_UDEBUG) enabled.\n");
		return rc;
	}

	if (rc < 0) {
		printf("Error connecting\n");
		printf("ipc_connect_task(%lld) -> %d ", task_id, rc);
		return rc;
	}

	phoneid = rc;

	rc = udebug_begin(phoneid);
	if (rc < 0) {
		printf("udebug_begin() -> %d\n", rc);
		return rc;
	}

	rc = udebug_set_evmask(phoneid, UDEBUG_EM_ALL);
	if (rc < 0) {
		printf("udebug_set_evmask(0x%x) -> %d\n ", UDEBUG_EM_ALL, rc);
		return rc;
	}

	return 0;
}

static int get_thread_list(void)
{
	int rc;
	size_t tb_copied;
	size_t tb_needed;
	int i;

	rc = udebug_thread_read(phoneid, thread_hash_buf,
		THBUF_SIZE*sizeof(unsigned), &tb_copied, &tb_needed);
	if (rc < 0) {
		printf("udebug_thread_read() -> %d\n", rc);
		return rc;
	}

	n_threads = tb_copied / sizeof(uintptr_t);

	printf("Threads:");
	for (i = 0; i < n_threads; i++) {
		printf(" [%d] (hash 0x%lx)", 1+i, thread_hash_buf[i]);
	}
	printf("\ntotal of %u threads\n", tb_needed / sizeof(uintptr_t));

	return 0;
}

void val_print(sysarg_t val, val_type_t v_type)
{
	switch (v_type) {
	case V_VOID:
		printf("<void>");
		break;

	case V_INTEGER:
		printf("%ld", val);
		break;

	case V_HASH:
	case V_PTR:
		printf("0x%08lx", val);
		break;

	case V_ERRNO:
		if (val >= -15 && val <= 0) {
			printf("%ld %s (%s)", val,
			    err_desc[-val].name,
			    err_desc[-val].desc);
		} else {
			printf("%ld", val);
		}
		break;
	case V_INT_ERRNO:
		if (val >= -15 && val < 0) {
			printf("%ld %s (%s)", val,
			    err_desc[-val].name,
			    err_desc[-val].desc);
		} else {
			printf("%ld", val);
		}
		break;

	case V_CHAR:
		if (val >= 0x20 && val < 0x7f) {
			printf("'%c'", val);
		} else {
			switch (val) {
			case '\a': printf("'\\a'"); break;
			case '\b': printf("'\\b'"); break;
			case '\n': printf("'\\n'"); break;
			case '\r': printf("'\\r'"); break;
			case '\t': printf("'\\t'"); break;
			case '\\': printf("'\\\\'"); break;
			default: printf("'\\x%02lX'", val); break;
			}
		}
		break;
	}
}


static void print_sc_retval(sysarg_t retval, val_type_t val_type)
{
	printf(" -> ");
	val_print(retval, val_type);
	putchar('\n');
}

static void print_sc_args(sysarg_t *sc_args, int n)
{
	int i;

	putchar('(');
	if (n > 0) printf("%ld", sc_args[0]);
	for (i = 1; i < n; i++) {
		printf(", %ld", sc_args[i]);
	}
	putchar(')');
}

static void sc_ipc_call_async_fast(sysarg_t *sc_args, sysarg_t sc_rc)
{
	ipc_call_t call;
	ipcarg_t phoneid;
	
	if (sc_rc == IPC_CALLRET_FATAL || sc_rc == IPC_CALLRET_TEMPORARY)
		return;

	phoneid = sc_args[0];

	IPC_SET_METHOD(call, sc_args[1]);
	IPC_SET_ARG1(call, sc_args[2]);
	IPC_SET_ARG2(call, sc_args[3]);
	IPC_SET_ARG3(call, sc_args[4]);
	IPC_SET_ARG4(call, sc_args[5]);
	IPC_SET_ARG5(call, 0);

	ipcp_call_out(phoneid, &call, sc_rc);
}

static void sc_ipc_call_async_slow(sysarg_t *sc_args, sysarg_t sc_rc)
{
	ipc_call_t call;
	int rc;

	if (sc_rc == IPC_CALLRET_FATAL || sc_rc == IPC_CALLRET_TEMPORARY)
		return;

	memset(&call, 0, sizeof(call));
	rc = udebug_mem_read(phoneid, &call.args, sc_args[1], sizeof(call.args));

	if (rc >= 0) {
		ipcp_call_out(sc_args[0], &call, sc_rc);
	}
}

static void sc_ipc_call_sync_fast(sysarg_t *sc_args)
{
	ipc_call_t question, reply;
	int rc;
	int phoneidx;

//	printf("sc_ipc_call_sync_fast()\n");
	phoneidx = sc_args[0];

	IPC_SET_METHOD(question, sc_args[1]);
	IPC_SET_ARG1(question, sc_args[2]);
	IPC_SET_ARG2(question, sc_args[3]);
	IPC_SET_ARG3(question, sc_args[4]);
	IPC_SET_ARG4(question, 0);
	IPC_SET_ARG5(question, 0);

//	printf("memset\n");
	memset(&reply, 0, sizeof(reply));
//	printf("udebug_mem_read(phone=%d, buffer_ptr=%u, src_addr=%d, n=%d\n",
//		phoneid, &reply.args, sc_args[5], sizeof(reply.args));
	rc = udebug_mem_read(phoneid, &reply.args, sc_args[5], sizeof(reply.args));
//	printf("dmr->%d\n", rc);
	if (rc < 0) return;

//	printf("call ipc_call_sync\n");
	ipcp_call_sync(phoneidx, &question, &reply);
}

static void sc_ipc_call_sync_slow(sysarg_t *sc_args)
{
	ipc_call_t question, reply;
	int rc;

	memset(&question, 0, sizeof(question));
	rc = udebug_mem_read(phoneid, &question.args, sc_args[1], sizeof(question.args));
	printf("dmr->%d\n", rc);
	if (rc < 0) return;

	memset(&reply, 0, sizeof(reply));
	rc = udebug_mem_read(phoneid, &reply.args, sc_args[2], sizeof(reply.args));
	printf("dmr->%d\n", rc);
	if (rc < 0) return;

	ipcp_call_sync(sc_args[0], &question, &reply);
}

static void sc_ipc_wait(sysarg_t *sc_args, int sc_rc)
{
	ipc_call_t call;
	int rc;

	if (sc_rc == 0) return;

	memset(&call, 0, sizeof(call));
	rc = udebug_mem_read(phoneid, &call, sc_args[0], sizeof(call));
//	printf("udebug_mem_read(phone %d, dest %d, app-mem src %d, size %d -> %d\n",
//		phoneid, (int)&call, sc_args[0], sizeof(call), rc);

	if (rc >= 0) {
		ipcp_call_in(&call, sc_rc);
	}
}

static void event_syscall_b(unsigned thread_id, uintptr_t thread_hash,
    unsigned sc_id, sysarg_t sc_rc)
{
	sysarg_t sc_args[6];
	int rc;

	/* Read syscall arguments */
	rc = udebug_args_read(phoneid, thread_hash, sc_args);

	async_serialize_start();

//	printf("[%d] ", thread_id);

	if (rc < 0) {
		printf("error\n");
		async_serialize_end();
		return;
	}

	if ((display_mask & DM_SYSCALL) != 0) {
		/* Print syscall name and arguments */
		printf("%s", syscall_desc[sc_id].name);
		print_sc_args(sc_args, syscall_desc[sc_id].n_args);
	}

	async_serialize_end();
}

static void event_syscall_e(unsigned thread_id, uintptr_t thread_hash,
    unsigned sc_id, sysarg_t sc_rc)
{
	sysarg_t sc_args[6];
	int rv_type;
	int rc;

	/* Read syscall arguments */
	rc = udebug_args_read(phoneid, thread_hash, sc_args);

	async_serialize_start();

//	printf("[%d] ", thread_id);

	if (rc < 0) {
		printf("error\n");
		async_serialize_end();
		return;
	}

	if ((display_mask & DM_SYSCALL) != 0) {
		/* Print syscall return value */
		rv_type = syscall_desc[sc_id].rv_type;
		print_sc_retval(sc_rc, rv_type);
	}

	switch (sc_id) {
	case SYS_IPC_CALL_ASYNC_FAST:
		sc_ipc_call_async_fast(sc_args, sc_rc);
		break;
	case SYS_IPC_CALL_ASYNC_SLOW:
		sc_ipc_call_async_slow(sc_args, sc_rc);
		break;
	case SYS_IPC_CALL_SYNC_FAST:
		sc_ipc_call_sync_fast(sc_args);
		break;
	case SYS_IPC_CALL_SYNC_SLOW:
		sc_ipc_call_sync_slow(sc_args);
		break;
	case SYS_IPC_WAIT:
		sc_ipc_wait(sc_args, sc_rc);
		break;
	default:
		break;
	}

	async_serialize_end();
}

static void event_thread_b(uintptr_t hash)
{
	async_serialize_start();
	printf("New thread, hash 0x%lx\n", hash);
	async_serialize_end();

	thread_trace_start(hash);
}

static int trace_loop(void *thread_hash_arg)
{
	int rc;
	unsigned ev_type;
	uintptr_t thread_hash;
	unsigned thread_id;
	sysarg_t val0, val1;

	thread_hash = (uintptr_t)thread_hash_arg;
	thread_id = next_thread_id++;

	printf("Start tracing thread [%d] (hash 0x%lx).\n", thread_id, thread_hash);

	while (!abort_trace) {

		if (paused) {
			printf("Press R to resume (and be patient).\n");
			while (paused) {
				usleep(1000000);
				fibril_yield();
				printf(".");
			}
			printf("Resumed\n");
		}

		/* Run thread until an event occurs */
		rc = udebug_go(phoneid, thread_hash,
		    &ev_type, &val0, &val1);

//		printf("rc = %d, ev_type=%d\n", rc, ev_type);
		if (ev_type == UDEBUG_EVENT_FINISHED) {
			/* Done tracing this thread */
			break;
		}

		if (rc >= 0) {
			switch (ev_type) {
			case UDEBUG_EVENT_SYSCALL_B:
				event_syscall_b(thread_id, thread_hash, val0, (int)val1);
				break;
			case UDEBUG_EVENT_SYSCALL_E:
				event_syscall_e(thread_id, thread_hash, val0, (int)val1);
				break;
			case UDEBUG_EVENT_STOP:
				printf("Stop event\n");
				break;
			case UDEBUG_EVENT_THREAD_B:
				event_thread_b(val0);
				break;
			case UDEBUG_EVENT_THREAD_E:
				printf("Thread 0x%lx exited.\n", val0);
				abort_trace = 1;
				break;
			default:
				printf("Unknown event type %d.\n", ev_type);
				break;
			}
		}

	}

	printf("Finished tracing thread [%d].\n", thread_id);
	return 0;
}

void thread_trace_start(uintptr_t thread_hash)
{
	fid_t fid;

	thash = thread_hash;

	fid = fibril_create(trace_loop, (void *)thread_hash);
	if (fid == 0) {
		printf("Warning: Failed creating fibril\n");
	}
	fibril_add_ready(fid);
}

static loader_t *preload_task(const char *path, char *const argv[],
    task_id_t *task_id)
{
	loader_t *ldr;
	int rc;

	/* Spawn a program loader */	
	ldr = loader_connect();
	if (ldr == NULL)
		return 0;

	/* Get task ID. */
	rc = loader_get_task_id(ldr, task_id);
	if (rc != EOK)
		goto error;

	/* Send program pathname */
	rc = loader_set_pathname(ldr, path);
	if (rc != EOK)
		goto error;

	/* Send arguments */
	rc = loader_set_args(ldr, argv);
	if (rc != EOK)
		goto error;

	/* Load the program. */
	rc = loader_load_program(ldr);
	if (rc != EOK)
		goto error;

	/* Success */
	return ldr;

	/* Error exit */
error:
	loader_abort(ldr);
	free(ldr);
	return NULL;
}

static void trace_task(task_id_t task_id)
{
	int i;
	int rc;
	int c;

	ipcp_init();

	/* 
	 * User apps now typically have console on phone 3.
	 * (Phones 1 and 2 are used by the loader).
	 */
	ipcp_connection_set(3, 0, proto_console);

	rc = get_thread_list();
	if (rc < 0) {
		printf("Failed to get thread list (error %d)\n", rc);
		return;
	}

	abort_trace = 0;

	for (i = 0; i < n_threads; i++) {
		thread_trace_start(thread_hash_buf[i]);
	}

	while(1) {
		c = getchar();
		if (c == 'q') break;
		if (c == 'p') {
			printf("Pause...\n");
			paused = 1;
			rc = udebug_stop(phoneid, thash);
			printf("stop -> %d\n", rc);
		}
		if (c == 'r') {
			paused = 0;
			printf("Resume...\n");
		}
	}

	printf("\nTerminate debugging session...\n");
	abort_trace = 1;
	udebug_end(phoneid);
	ipc_hangup(phoneid);

	ipcp_cleanup();

	printf("Done\n");
	return;
}

static void main_init(void)
{
	proto_t *p;
	oper_t *o;

	val_type_t arg_def[OPER_MAX_ARGS] = {
		V_INTEGER,
		V_INTEGER,
		V_INTEGER,
		V_INTEGER,
		V_INTEGER		
	};

	val_type_t resp_def[OPER_MAX_ARGS] = {
		V_INTEGER,
		V_INTEGER,
		V_INTEGER,
		V_INTEGER,
		V_INTEGER
	};

	next_thread_id = 1;
	paused = 0;

	proto_init();

	p = proto_new("vfs");
	o = oper_new("read", 1, arg_def, V_ERRNO, 1, resp_def);
	proto_add_oper(p, VFS_READ, o);
	o = oper_new("write", 1, arg_def, V_ERRNO, 1, resp_def);
	proto_add_oper(p, VFS_WRITE, o);
	o = oper_new("truncate", 5, arg_def, V_ERRNO, 0, resp_def);
	proto_add_oper(p, VFS_TRUNCATE, o);
	o = oper_new("mount", 2, arg_def, V_ERRNO, 0, resp_def);
	proto_add_oper(p, VFS_MOUNT, o);
/*	o = oper_new("unmount", 0, arg_def);
	proto_add_oper(p, VFS_UNMOUNT, o);*/
	o = oper_new("open", 2, arg_def, V_INT_ERRNO, 0, resp_def);
	proto_add_oper(p, VFS_OPEN, o);
	o = oper_new("close", 1, arg_def, V_ERRNO, 0, resp_def);
	proto_add_oper(p, VFS_CLOSE, o);
	o = oper_new("seek", 3, arg_def, V_ERRNO, 0, resp_def);
	proto_add_oper(p, VFS_SEEK, o);
	o = oper_new("mkdir", 1, arg_def, V_ERRNO, 0, resp_def);
	proto_add_oper(p, VFS_MKDIR, o);
	o = oper_new("unlink", 0, arg_def, V_ERRNO, 0, resp_def);
	proto_add_oper(p, VFS_UNLINK, o);
	o = oper_new("rename", 0, arg_def, V_ERRNO, 0, resp_def);
	proto_add_oper(p, VFS_RENAME, o);

	proto_register(SERVICE_VFS, p);

	p = proto_new("console");
	resp_def[0] = V_INTEGER; resp_def[1] = V_INTEGER;
	resp_def[2] = V_INTEGER; resp_def[3] = V_CHAR;
	o = oper_new("getkey", 0, arg_def, V_ERRNO, 4, resp_def);
	proto_add_oper(p, CONSOLE_GETKEY, o);

	arg_def[0] = V_CHAR;
	o = oper_new("putchar", 1, arg_def, V_VOID, 0, resp_def);
	proto_add_oper(p, CONSOLE_PUTCHAR, o);
	o = oper_new("clear", 0, arg_def, V_VOID, 0, resp_def);
	proto_add_oper(p, CONSOLE_CLEAR, o);

	arg_def[0] = V_INTEGER; arg_def[1] = V_INTEGER;
	o = oper_new("goto", 2, arg_def, V_VOID, 0, resp_def);
	proto_add_oper(p, CONSOLE_GOTO, o);

	resp_def[0] = V_INTEGER; resp_def[1] = V_INTEGER;
	o = oper_new("getsize", 0, arg_def, V_INTEGER, 2, resp_def);
	proto_add_oper(p, CONSOLE_GETSIZE, o);
	o = oper_new("flush", 0, arg_def, V_VOID, 0, resp_def);
	proto_add_oper(p, CONSOLE_FLUSH, o);

	arg_def[0] = V_INTEGER;
	o = oper_new("set_style", 1, arg_def, V_VOID, 0, resp_def);
	proto_add_oper(p, CONSOLE_SET_STYLE, o);
	arg_def[0] = V_INTEGER; arg_def[1] = V_INTEGER; arg_def[2] = V_INTEGER;
	o = oper_new("set_color", 3, arg_def, V_VOID, 0, resp_def);
	proto_add_oper(p, CONSOLE_SET_COLOR, o);
	arg_def[0] = V_INTEGER; arg_def[1] = V_INTEGER;
	o = oper_new("set_rgb_color", 2, arg_def, V_VOID, 0, resp_def);
	proto_add_oper(p, CONSOLE_SET_RGB_COLOR, o);
	o = oper_new("cursor_visibility", 1, arg_def, V_VOID, 0, resp_def);
	proto_add_oper(p, CONSOLE_CURSOR_VISIBILITY, o);

	proto_console = p;
	proto_register(SERVICE_CONSOLE, p);
}

static void print_syntax()
{
	printf("Syntax:\n");
	printf("\ttrace [+<events>] <executable> [<arg1> [...]]\n");
	printf("or\ttrace [+<events>] -t <task_id>\n");
	printf("Events: (default is +tp)\n");
	printf("\n");
	printf("\tt ... Thread creation and termination\n");
	printf("\ts ... System calls\n");
	printf("\ti ... Low-level IPC\n");
	printf("\tp ... Protocol level\n");
	printf("\n");
	printf("Examples:\n");
	printf("\ttrace +s /app/tetris\n");
	printf("\ttrace +tsip -t 12\n");
}

static display_mask_t parse_display_mask(char *text)
{
	display_mask_t dm;
	char *c;

	c = text;

	while (*c) {
		switch (*c) {
		case 't': dm = dm | DM_THREAD; break;
		case 's': dm = dm | DM_SYSCALL; break;
		case 'i': dm = dm | DM_IPC; break;
		case 'p': dm = dm | DM_SYSTEM | DM_USER; break;
		default:
			printf("Unexpected event type '%c'.\n", *c);
			exit(1);
		}

		++c;
	}

	return dm;
}

static int parse_args(int argc, char *argv[])
{
	char *arg;
	char *err_p;

	task_id = 0;

	--argc; ++argv;

	while (argc > 0) {
		arg = *argv;
		if (arg[0] == '+') {
			display_mask = parse_display_mask(&arg[1]);
		} else if (arg[0] == '-') {
			if (arg[1] == 't') {
				/* Trace an already running task */
				--argc; ++argv;
				task_id = strtol(*argv, &err_p, 10);
				task_ldr = NULL;
				if (*err_p) {
					printf("Task ID syntax error\n");
					print_syntax();
					return -1;
				}
			} else {
				printf("Uknown option '%s'\n", arg[0]);
				print_syntax();
				return -1;
			}
		} else {
			break;
		}

		--argc; ++argv;
	}

	if (task_id != 0) {
		if (argc == 0) return 0;
		printf("Extra arguments\n");
		print_syntax();
		return -1;
	}

	if (argc < 1) {
		printf("Missing argument\n");
		print_syntax();
		return -1;
	}

	/* Preload the specified program file. */
	printf("Spawning '%s' with arguments:\n", *argv);
	{
		char **cp = argv;
		while (*cp) printf("'%s'\n", *cp++);
	}
	task_ldr = preload_task(*argv, argv, &task_id);

	return 0;
}

int main(int argc, char *argv[])
{
	int rc;

	printf("System Call / IPC Tracer\n");
	printf("Controls: Q - Quit, P - Pause, R - Resume\n");

	display_mask = DM_THREAD | DM_SYSTEM | DM_USER;

	if (parse_args(argc, argv) < 0)
		return 1;

	main_init();

	rc = connect_task(task_id);
	if (rc < 0) {
		printf("Failed connecting to task %lld.\n", task_id);
		return 1;
	}

	printf("Connected to task %lld.\n", task_id);

	if (task_ldr != NULL) {
		program_run();
	}

	trace_task(task_id);

	return 0;
}

/** @}
 */
