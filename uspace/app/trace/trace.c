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
#include <syscall.h>
#include <ipc/ipc.h>
#include <fibril.h>
#include <errno.h>
#include <udebug.h>
#include <async.h>
#include <task.h>

// Temporary: service and method names
#include "proto.h"
#include <ipc/services.h>
#include "../../srv/vfs/vfs.h"
#include "../../srv/console/console.h"

#include "syscalls.h"
#include "ipcp.h"
#include "errors.h"
#include "trace.h"

#define THBUF_SIZE 64
unsigned thread_hash_buf[THBUF_SIZE];
unsigned n_threads;

int next_thread_id;

int phoneid;
int abort_trace;

unsigned thash;
volatile int paused;

void thread_trace_start(unsigned thread_hash);

static proto_t *proto_console;
static task_id_t task_id;

/** Combination of events/data to print. */
display_mask_t display_mask;

static int task_connect(task_id_t task_id)
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

	n_threads = tb_copied / sizeof(unsigned);

	printf("Threads:");
	for (i = 0; i < n_threads; i++) {
		printf(" [%d] (hash 0x%x)", 1+i, thread_hash_buf[i]);
	}
	printf("\ntotal of %u threads\n", tb_needed/sizeof(unsigned));

	return 0;
}

void val_print(int val, val_type_t v_type)
{
	switch (v_type) {
	case V_VOID:
		printf("<void>");
		break;

	case V_INTEGER:
		printf("%d", val);
		break;

	case V_HASH:
		printf("0x%08x", val);
		break;

	case V_ERRNO:
		if (val >= -15 && val <= 0) {
			printf("%d %s (%s)", val,
			    err_desc[-val].name,
			    err_desc[-val].desc);
		} else {
			printf("%d", val);
		}
		break;
	case V_INT_ERRNO:
		if (val >= -15 && val < 0) {
			printf("%d %s (%s)", val,
			    err_desc[-val].name,
			    err_desc[-val].desc);
		} else {
			printf("%d", val);
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
			default: printf("'\\x%X'"); break;
			}
		}
		break;
	}
}


static void print_sc_retval(int retval, val_type_t val_type)
{
	printf(" -> ");
	val_print(retval, val_type);
	putchar('\n');
}

static void print_sc_args(unsigned *sc_args, int n)
{
	int i;

	putchar('(');
	if (n > 0) printf("%d", sc_args[0]);
	for (i = 1; i < n; i++) {
		printf(", %d", sc_args[i]);
	}
	putchar(')');
}

static void sc_ipc_call_async_fast(unsigned *sc_args, int sc_rc)
{
	ipc_call_t call;
	int phoneid;
	
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

static void sc_ipc_call_async_slow(unsigned *sc_args, int sc_rc)
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

static void sc_ipc_call_sync_fast(unsigned *sc_args)
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

static void sc_ipc_call_sync_slow(unsigned *sc_args)
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

static void sc_ipc_wait(unsigned *sc_args, int sc_rc)
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

static void event_syscall_b(unsigned thread_id, unsigned thread_hash,  unsigned sc_id, int sc_rc)
{
	unsigned sc_args[6];
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

static void event_syscall_e(unsigned thread_id, unsigned thread_hash,  unsigned sc_id, int sc_rc)
{
	unsigned sc_args[6];
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

static void event_thread_b(unsigned hash)
{
	async_serialize_start();
	printf("New thread, hash 0x%x\n", hash);
	async_serialize_end();

	thread_trace_start(hash);
}

static int trace_loop(void *thread_hash_arg)
{
	int rc;
	unsigned ev_type;
	unsigned thread_hash;
	unsigned thread_id;
	unsigned val0, val1;

	thread_hash = (unsigned)thread_hash_arg;
	thread_id = next_thread_id++;

	printf("Start tracing thread [%d] (hash 0x%x)\n", thread_id, thread_hash);

	while (!abort_trace) {

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
				printf("Waiting for resume\n");
				while (paused) {
					usleep(1000000);
					fibril_yield();
					printf(".");
				}
				printf("Resumed\n");
				break;
			case UDEBUG_EVENT_THREAD_B:
				event_thread_b(val0);
				break;
			case UDEBUG_EVENT_THREAD_E:
				printf("Thread 0x%x exited\n", val0);
				abort_trace = 1;
				break;
			default:
				printf("Unknown event type %d\n", ev_type);
				break;
			}
		}

	}

	printf("Finished tracing thread [%d]\n", thread_id);
	return 0;
}

void thread_trace_start(unsigned thread_hash)
{
	fid_t fid;

	thash = thread_hash;

	fid = fibril_create(trace_loop, (void *)thread_hash);
	if (fid == 0) {
		printf("Warning: Failed creating fibril\n");
	}
	fibril_add_ready(fid);
}

static void trace_active_task(task_id_t task_id)
{
	int i;
	int rc;
	int c;

	rc = task_connect(task_id);
	if (rc < 0) {
		printf("Failed to connect to task %lld\n", task_id);
		return;
	}

	printf("Connected to task %lld\n", task_id);

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
			paused = 1;
			rc = udebug_stop(phoneid, thash);
			printf("stop -> %d\n", rc);
		}
		if (c == 'r') {
			paused = 0;
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

	proto_register(SERVICE_VFS, p);

	p = proto_new("console");
	resp_def[0] = V_CHAR;
	o = oper_new("getchar", 0, arg_def, V_INTEGER, 2, resp_def);
	proto_add_oper(p, CONSOLE_GETCHAR, o);

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

	arg_def[0] = V_INTEGER; arg_def[1] = V_INTEGER;
	o = oper_new("set_style", 2, arg_def, V_INTEGER, 0, resp_def);
	proto_add_oper(p, CONSOLE_SET_STYLE, o);
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
			printf("Unexpected event type '%c'\n", *c);
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
		if (argc == 0) return;
		printf("Extra arguments\n");
		print_syntax();
		return -1;
	}

	if (argc < 1) {
		printf("Missing argument\n");
		print_syntax();
		return -1;
	}

	/* Execute the specified command and trace the new task. */
	printf("Spawning '%s' with arguments:\n", *argv);
	{
		char **cp = argv;
		while (*cp) printf("'%s'\n", *cp++);
	}
	task_id = task_spawn(*argv, argv);

	return 0;
}

int main(int argc, char *argv[])
{
	printf("System Call / IPC Tracer\n");

	display_mask = DM_THREAD | DM_SYSTEM | DM_USER;

	if (parse_args(argc, argv) < 0)
		return 1;

	main_init();
	trace_active_task(task_id);

	return 0;
}

/** @}
 */
