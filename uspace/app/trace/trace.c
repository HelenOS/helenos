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

// Temporary: service and method names
#include "proto.h"
#include <ipc/services.h>
#include "../../srv/vfs/vfs.h"
#include "../../srv/console/console.h"

#include "syscalls.h"
#include "ipcp.h"
#include "errors.h"

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
		printf(" [%d] (hash 0x%u)", 1+i, thread_hash_buf[i]);
	}
	printf("\ntotal of %u threads\n", tb_needed/sizeof(unsigned));

	return 0;
}

static void print_sc_retval(int retval, rv_type_t rv_type)
{
	printf (" -> ");
	if (rv_type == RV_INTEGER) {
		printf("%d", retval);
	} else if (rv_type == RV_HASH) {
		printf("0x%08x", retval);
	} else if (rv_type == RV_ERRNO) {
		if (retval >= -15 && retval <= 0) {
			printf("%d %s (%s)", retval,
			    err_desc[retval].name,
			    err_desc[retval].desc);
		} else {
			printf("%d", retval);
		}
	} else if (rv_type == RV_INT_ERRNO) {
		if (retval >= -15 && retval < 0) {
			printf("%d %s (%s)", retval,
			    err_desc[retval].name,
			    err_desc[retval].desc);
		} else {
			printf("%d", retval);
		}
	}
	putchar('\n');
}

static void print_sc_args(unsigned *sc_args, int n)
{
	int i;

	putchar('(');
	if (n > 0) printf("%d", sc_args[0]);
	for (i=1; i<n; i++) {
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

	/* Print syscall name, id and arguments */
	printf("%s", syscall_desc[sc_id].name);
	print_sc_args(sc_args, syscall_desc[sc_id].n_args);

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

	rv_type = syscall_desc[sc_id].rv_type;
	print_sc_retval(sc_rc, rv_type);

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

	printf("Syscall Tracer\n");

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

	next_thread_id = 1;
	paused = 0;

	proto_init();

	p = proto_new("vfs");
	o = oper_new("read");
	proto_add_oper(p, VFS_READ, o);
	o = oper_new("write");
	proto_add_oper(p, VFS_WRITE, o);
	o = oper_new("truncate");
	proto_add_oper(p, VFS_TRUNCATE, o);
	o = oper_new("mount");
	proto_add_oper(p, VFS_MOUNT, o);
	o = oper_new("unmount");
	proto_add_oper(p, VFS_UNMOUNT, o);

	proto_register(SERVICE_VFS, p);

	p = proto_new("console");
	o = oper_new("getchar");
	proto_add_oper(p, CONSOLE_GETCHAR, o);
	o = oper_new("putchar");
	proto_add_oper(p, CONSOLE_PUTCHAR, o);
	o = oper_new("clear");
	proto_add_oper(p, CONSOLE_CLEAR, o);
	o = oper_new("goto");
	proto_add_oper(p, CONSOLE_GOTO, o);
	o = oper_new("getsize");
	proto_add_oper(p, CONSOLE_GETSIZE, o);
	o = oper_new("flush");
	proto_add_oper(p, CONSOLE_FLUSH, o);
	o = oper_new("set_style");
	proto_add_oper(p, CONSOLE_SET_STYLE, o);
	o = oper_new("cursor_visibility");
	proto_add_oper(p, CONSOLE_CURSOR_VISIBILITY, o);
	o = oper_new("flush");
	proto_add_oper(p, CONSOLE_FLUSH, o);

	proto_console = p;
	proto_register(SERVICE_CONSOLE, p);
}

static void print_syntax()
{
	printf("Syntax: trace <task_id>\n");
}

int main(int argc, char *argv[])
{
	task_id_t task_id;
	char *err_p;

	if (argc != 2) {
		printf("Mising argument\n");
		print_syntax();
		return 1;
	}

	task_id = strtol(argv[1], &err_p, 10);

	if (*err_p) {
		printf("Task ID syntax error\n");
		print_syntax();
		return 1;
	}

	main_init();
	trace_active_task(task_id);
}

/** @}
 */
