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
#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>
#include <str_error.h>
#include <inttypes.h>
#include <fibril.h>
#include <errno.h>
#include <udebug.h>
#include <async.h>
#include <task.h>
#include <mem.h>
#include <str.h>
#include <loader/loader.h>
#include <io/console.h>
#include <io/keycode.h>
#include <fibril_synch.h>
#include <vfs/vfs.h>

#include <libc.h>

/* Temporary: service and method names */
#include "proto.h"
#include <ipc/services.h>
#include <ipc/vfs.h>
#include <ipc/console.h>

#include "syscalls.h"
#include "ipcp.h"
#include "trace.h"

#define THBUF_SIZE 64
uintptr_t thread_hash_buf[THBUF_SIZE];
int n_threads;

int next_thread_id;

ipc_call_t thread_ipc_req[THBUF_SIZE];

async_sess_t *sess;
bool abort_trace;

uintptr_t thash;
static bool paused;
static fibril_condvar_t state_cv;
static fibril_mutex_t state_lock;

static bool cev_valid;
static kbd_event_t cev;

void thread_trace_start(uintptr_t thread_hash);

static task_id_t task_id;
static loader_t *task_ldr;
static bool task_wait_for;

/** Combination of events/data to print. */
display_mask_t display_mask;

static errno_t program_run_fibril(void *arg);
static errno_t cev_fibril(void *arg);

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

static void cev_fibril_start(void)
{
	fid_t fid;

	fid = fibril_create(cev_fibril, NULL);
	if (fid == 0) {
		printf("Error creating fibril\n");
		exit(1);
	}

	fibril_add_ready(fid);
}

static errno_t program_run_fibril(void *arg)
{
	errno_t rc;

	/*
	 * This must be done in background as it will block until
	 * we let the task reply to this call.
	 */
	rc = loader_run(task_ldr);
	if (rc != EOK) {
		printf("Error running program\n");
		exit(1);
	}

	task_ldr = NULL;

	printf("program_run_fibril exiting\n");
	return 0;
}


static errno_t connect_task(task_id_t task_id)
{
	async_sess_t *ksess = async_connect_kbox(task_id);

	if (!ksess) {
		if (errno == ENOTSUP) {
			printf("You do not have userspace debugging support "
			    "compiled in the kernel.\n");
			printf("Compile kernel with 'Support for userspace debuggers' "
			    "(CONFIG_UDEBUG) enabled.\n");
			return errno;
		}

		printf("Error connecting\n");
		printf("ipc_connect_task(%" PRIu64 ") -> %s ", task_id, str_error_name(errno));
		return errno;
	}

	errno_t rc = udebug_begin(ksess);
	if (rc != EOK) {
		printf("udebug_begin() -> %s\n", str_error_name(rc));
		return rc;
	}

	rc = udebug_set_evmask(ksess, UDEBUG_EM_ALL);
	if (rc != EOK) {
		printf("udebug_set_evmask(0x%x) -> %s\n ", UDEBUG_EM_ALL, str_error_name(rc));
		return rc;
	}

	sess = ksess;
	return 0;
}

static errno_t get_thread_list(void)
{
	errno_t rc;
	size_t tb_copied;
	size_t tb_needed;
	int i;

	rc = udebug_thread_read(sess, thread_hash_buf,
		THBUF_SIZE*sizeof(unsigned), &tb_copied, &tb_needed);
	if (rc != EOK) {
		printf("udebug_thread_read() -> %s\n", str_error_name(rc));
		return rc;
	}

	n_threads = tb_copied / sizeof(uintptr_t);

	printf("Threads:");
	for (i = 0; i < n_threads; i++) {
		printf(" [%d] (hash %p)", 1 + i, (void *) thread_hash_buf[i]);
	}
	printf("\ntotal of %zu threads\n", tb_needed / sizeof(uintptr_t));

	return 0;
}

void val_print(sysarg_t val, val_type_t v_type)
{
	long sval;

	sval = (long) val;

	switch (v_type) {
	case V_VOID:
		printf("<void>");
		break;

	case V_INTEGER:
		printf("%ld", sval);
		break;

	case V_HASH:
	case V_PTR:
		printf("%p", (void *) val);
		break;

	case V_ERRNO:
		if (sval >= -15 && sval <= 0) {
			printf("%ld %s (%s)", sval,
			    str_error_name((errno_t) sval),
			    str_error((errno_t) sval));
		} else {
			printf("%ld", sval);
		}
		break;
	case V_INT_ERRNO:
		if (sval >= -15 && sval < 0) {
			printf("%ld %s (%s)", sval,
			    str_error_name((errno_t) sval),
			    str_error((errno_t) sval));
		} else {
			printf("%ld", sval);
		}
		break;

	case V_CHAR:
		if (sval >= 0x20 && sval < 0x7f) {
			printf("'%c'", (char) sval);
		} else {
			switch (sval) {
			case '\a': printf("'\\a'"); break;
			case '\b': printf("'\\b'"); break;
			case '\n': printf("'\\n'"); break;
			case '\r': printf("'\\r'"); break;
			case '\t': printf("'\\t'"); break;
			case '\\': printf("'\\\\'"); break;
			default: printf("'\\x%02" PRIxn "'", val); break;
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
	if (n > 0) printf("%" PRIun, sc_args[0]);
	for (i = 1; i < n; i++) {
		printf(", %" PRIun, sc_args[i]);
	}
	putchar(')');
}

static void sc_ipc_call_async_fast(sysarg_t *sc_args, errno_t sc_rc)
{
	ipc_call_t call;
	sysarg_t phoneid;

	if (sc_rc != EOK)
		return;

	phoneid = sc_args[0];

	IPC_SET_IMETHOD(call, sc_args[1]);
	IPC_SET_ARG1(call, sc_args[2]);
	IPC_SET_ARG2(call, sc_args[3]);
	IPC_SET_ARG3(call, sc_args[4]);
	IPC_SET_ARG4(call, sc_args[5]);
	IPC_SET_ARG5(call, 0);

	ipcp_call_out(phoneid, &call, 0);
}

static void sc_ipc_call_async_slow(sysarg_t *sc_args, errno_t sc_rc)
{
	ipc_call_t call;
	errno_t rc;

	if (sc_rc != EOK)
		return;

	memset(&call, 0, sizeof(call));
	rc = udebug_mem_read(sess, &call.args, sc_args[1], sizeof(call.args));

	if (rc == EOK) {
		ipcp_call_out(sc_args[0], &call, 0);
	}
}

static void sc_ipc_wait(sysarg_t *sc_args, int sc_rc)
{
	ipc_call_t call;
	errno_t rc;

	if (sc_rc == 0) return;

	memset(&call, 0, sizeof(call));
	rc = udebug_mem_read(sess, &call, sc_args[0], sizeof(call));

	if (rc == EOK)
		ipcp_call_in(&call, sc_rc);
}

static void event_syscall_b(unsigned thread_id, uintptr_t thread_hash,
    unsigned sc_id, sysarg_t sc_rc)
{
	sysarg_t sc_args[6];
	errno_t rc;

	/* Read syscall arguments */
	rc = udebug_args_read(sess, thread_hash, sc_args);

	if (rc != EOK) {
		printf("error\n");
		return;
	}

	if ((display_mask & DM_SYSCALL) != 0) {
		/* Print syscall name and arguments */
		if (syscall_desc_defined(sc_id)) {
			printf("%s", syscall_desc[sc_id].name);
			print_sc_args(sc_args, syscall_desc[sc_id].n_args);
		}
		else {
			printf("unknown_syscall<%d>", sc_id);
			print_sc_args(sc_args, 6);
		}
	}
}

static void event_syscall_e(unsigned thread_id, uintptr_t thread_hash,
    unsigned sc_id, sysarg_t sc_rc)
{
	sysarg_t sc_args[6];
	int rv_type;
	errno_t rc;

	/* Read syscall arguments */
	rc = udebug_args_read(sess, thread_hash, sc_args);

//	printf("[%d] ", thread_id);

	if (rc != EOK) {
		printf("error\n");
		return;
	}

	if ((display_mask & DM_SYSCALL) != 0) {
		/* Print syscall return value */
		if (syscall_desc_defined(sc_id))
			rv_type = syscall_desc[sc_id].rv_type;
		else
			rv_type = V_PTR;
		print_sc_retval(sc_rc, rv_type);
	}

	switch (sc_id) {
	case SYS_IPC_CALL_ASYNC_FAST:
		sc_ipc_call_async_fast(sc_args, (errno_t) sc_rc);
		break;
	case SYS_IPC_CALL_ASYNC_SLOW:
		sc_ipc_call_async_slow(sc_args, (errno_t) sc_rc);
		break;
	case SYS_IPC_WAIT:
		sc_ipc_wait(sc_args, sc_rc);
		break;
	default:
		break;
	}
}

static void event_thread_b(uintptr_t hash)
{
	printf("New thread, hash %p\n", (void *) hash);
	thread_trace_start(hash);
}

static errno_t trace_loop(void *thread_hash_arg)
{
	errno_t rc;
	unsigned ev_type;
	uintptr_t thread_hash;
	unsigned thread_id;
	sysarg_t val0, val1;

	thread_hash = (uintptr_t)thread_hash_arg;
	thread_id = next_thread_id++;
	if (thread_id >= THBUF_SIZE) {
		printf("Too many threads.\n");
		return ELIMIT;
	}

	printf("Start tracing thread [%u] (hash %p).\n",
	    thread_id, (void *) thread_hash);

	while (!abort_trace) {

		fibril_mutex_lock(&state_lock);
		if (paused) {
			printf("Thread [%u] paused. Press R to resume.\n",
			    thread_id);

			while (paused)
				fibril_condvar_wait(&state_cv, &state_lock);

			printf("Thread [%u] resumed.\n", thread_id);
		}
		fibril_mutex_unlock(&state_lock);

		/* Run thread until an event occurs */
		rc = udebug_go(sess, thread_hash,
		    &ev_type, &val0, &val1);

//		printf("rc = %d, ev_type=%d\n", rc, ev_type);
		if (ev_type == UDEBUG_EVENT_FINISHED) {
			/* Done tracing this thread */
			break;
		}

		if (rc == EOK) {
			switch (ev_type) {
			case UDEBUG_EVENT_SYSCALL_B:
				event_syscall_b(thread_id, thread_hash, val0, (int)val1);
				break;
			case UDEBUG_EVENT_SYSCALL_E:
				event_syscall_e(thread_id, thread_hash, val0, (int)val1);
				break;
			case UDEBUG_EVENT_STOP:
				printf("Stop event\n");
				fibril_mutex_lock(&state_lock);
				paused = true;
				fibril_mutex_unlock(&state_lock);
				break;
			case UDEBUG_EVENT_THREAD_B:
				event_thread_b(val0);
				break;
			case UDEBUG_EVENT_THREAD_E:
				printf("Thread %" PRIun " exited.\n", val0);
				fibril_mutex_lock(&state_lock);
				abort_trace = true;
				fibril_condvar_broadcast(&state_cv);
				fibril_mutex_unlock(&state_lock);
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

static loader_t *preload_task(const char *path, char **argv,
    task_id_t *task_id)
{
	loader_t *ldr;
	errno_t rc;

	/* Spawn a program loader */
	ldr = loader_connect();
	if (ldr == NULL)
		return NULL;

	/* Get task ID. */
	rc = loader_get_task_id(ldr, task_id);
	if (rc != EOK)
		goto error;

	/* Send program. */
	rc = loader_set_program_path(ldr, path);
	if (rc != EOK)
		goto error;

	/* Send arguments */
	rc = loader_set_args(ldr, (const char **) argv);
	if (rc != EOK)
		goto error;

	/* Send default files */
	int fd_root;
	int fd_stdin;
	int fd_stdout;
	int fd_stderr;

	fd_root = vfs_root();
	if (fd_root >= 0) {
		rc = loader_add_inbox(ldr, "root", fd_root);
		vfs_put(fd_root);
		if (rc != EOK)
			goto error;
	}

	if ((stdin != NULL) && (vfs_fhandle(stdin, &fd_stdin) == EOK)) {
		rc = loader_add_inbox(ldr, "stdin", fd_stdin);
		if (rc != EOK)
			goto error;
	}

	if ((stdout != NULL) && (vfs_fhandle(stdout, &fd_stdout) == EOK)) {
		rc = loader_add_inbox(ldr, "stdout", fd_stdout);
		if (rc != EOK)
			goto error;
	}

	if ((stderr != NULL) && (vfs_fhandle(stderr, &fd_stderr) == EOK)) {
		rc = loader_add_inbox(ldr, "stderr", fd_stderr);
		if (rc != EOK)
			goto error;
	}

	/* Load the program. */
	rc = loader_load_program(ldr);
	if (rc != EOK)
		goto error;

	/* Success */
	return ldr;

	/* Error exit */
error:
	loader_abort(ldr);
	return NULL;
}

static errno_t cev_fibril(void *arg)
{
	cons_event_t event;

	(void) arg;

	console_ctrl_t *console = console_init(stdin, stdout);

	while (true) {
		fibril_mutex_lock(&state_lock);
		while (cev_valid)
			fibril_condvar_wait(&state_cv, &state_lock);
		fibril_mutex_unlock(&state_lock);

		if (!console_get_event(console, &event))
			return EINVAL;

		if (event.type == CEV_KEY) {
			fibril_mutex_lock(&state_lock);
			cev = event.ev.key;
			cev_valid = true;
			fibril_condvar_broadcast(&state_cv);
			fibril_mutex_unlock(&state_lock);
		}
	}
}

static void trace_task(task_id_t task_id)
{
	kbd_event_t ev;
	bool done;
	int i;
	errno_t rc;

	ipcp_init();

	rc = get_thread_list();
	if (rc != EOK) {
		printf("Failed to get thread list (%s)\n", str_error(rc));
		return;
	}

	abort_trace = false;

	for (i = 0; i < n_threads; i++) {
		thread_trace_start(thread_hash_buf[i]);
	}

	done = false;

	while (!done) {
		fibril_mutex_lock(&state_lock);
		while (!cev_valid && !abort_trace)
			fibril_condvar_wait(&state_cv, &state_lock);
		fibril_mutex_unlock(&state_lock);

		ev = cev;

		fibril_mutex_lock(&state_lock);
		cev_valid = false;
		fibril_condvar_broadcast(&state_cv);
		fibril_mutex_unlock(&state_lock);

		if (abort_trace)
			break;

		if (ev.type != KEY_PRESS)
			continue;

		switch (ev.key) {
		case KC_Q:
			done = true;
			break;
		case KC_P:
			printf("Pause...\n");
			rc = udebug_stop(sess, thash);
			if (rc != EOK)
				printf("Error: stop -> %s\n", str_error_name(rc));
			break;
		case KC_R:
			fibril_mutex_lock(&state_lock);
			paused = false;
			fibril_condvar_broadcast(&state_cv);
			fibril_mutex_unlock(&state_lock);
			printf("Resume...\n");
			break;
		default:
			break;
		}
	}

	printf("\nTerminate debugging session...\n");
	abort_trace = true;
	udebug_end(sess);
	async_hangup(sess);

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
	paused = false;
	cev_valid = false;

	fibril_mutex_initialize(&state_lock);
	fibril_condvar_initialize(&state_cv);

	proto_init();

	p = proto_new("vfs");
	o = oper_new("read", 3, arg_def, V_ERRNO, 1, resp_def);
	proto_add_oper(p, VFS_IN_READ, o);
	o = oper_new("write", 3, arg_def, V_ERRNO, 1, resp_def);
	proto_add_oper(p, VFS_IN_WRITE, o);
	o = oper_new("vfs_resize", 5, arg_def, V_ERRNO, 0, resp_def);
	proto_add_oper(p, VFS_IN_RESIZE, o);
	o = oper_new("vfs_stat", 1, arg_def, V_ERRNO, 0, resp_def);
	proto_add_oper(p, VFS_IN_STAT, o);
	o = oper_new("vfs_put", 1, arg_def, V_ERRNO, 0, resp_def);
	proto_add_oper(p, VFS_IN_PUT, o);
	o = oper_new("vfs_mount", 2, arg_def, V_ERRNO, 0, resp_def);
	proto_add_oper(p, VFS_IN_MOUNT, o);
/*	o = oper_new("unmount", 0, arg_def);
	proto_add_oper(p, VFS_IN_UNMOUNT, o);*/
	o = oper_new("vfs_sync", 1, arg_def, V_ERRNO, 0, resp_def);
	proto_add_oper(p, VFS_IN_SYNC, o);
	o = oper_new("rename", 0, arg_def, V_ERRNO, 0, resp_def);
	proto_add_oper(p, VFS_IN_RENAME, o);
	o = oper_new("vfs_statfs", 0, arg_def, V_ERRNO, 0, resp_def);
	proto_add_oper(p, VFS_IN_STATFS, o);
	o = oper_new("vfs_walk", 2, arg_def, V_INT_ERRNO, 0, resp_def);
	proto_add_oper(p, VFS_IN_WALK, o);
	o = oper_new("vfs_open", 2, arg_def, V_ERRNO, 0, resp_def);
	proto_add_oper(p, VFS_IN_OPEN, o);
	o = oper_new("vfs_unlink", 3, arg_def, V_ERRNO, 0, resp_def);
	proto_add_oper(p, VFS_IN_UNLINK, o);

	proto_register(SERVICE_VFS, p);
}

static void print_syntax(void)
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

static display_mask_t parse_display_mask(const char *text)
{
	display_mask_t dm = 0;
	const char *c = text;

	while (*c) {
		switch (*c) {
		case 't':
			dm = dm | DM_THREAD;
			break;
		case 's':
			dm = dm | DM_SYSCALL;
			break;
		case 'i':
			dm = dm | DM_IPC;
			break;
		case 'p':
			dm = dm | DM_SYSTEM | DM_USER;
			break;
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
	char *err_p;

	task_id = 0;

	--argc;
	++argv;

	while (argc > 0) {
		char *arg = *argv;
		if (arg[0] == '+') {
			display_mask = parse_display_mask(&arg[1]);
		} else if (arg[0] == '-') {
			if (arg[1] == 't') {
				/* Trace an already running task */
				--argc;
				++argv;
				task_id = strtol(*argv, &err_p, 10);
				task_ldr = NULL;
				task_wait_for = false;
				if (*err_p) {
					printf("Task ID syntax error\n");
					print_syntax();
					return -1;
				}
			} else {
				printf("Uknown option '%c'\n", arg[0]);
				print_syntax();
				return -1;
			}
		} else {
			break;
		}

		--argc;
		++argv;
	}

	if (task_id != 0) {
		if (argc == 0)
			return 0;
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

	char **cp = argv;
	while (*cp)
		printf("'%s'\n", *cp++);

	task_ldr = preload_task(*argv, argv, &task_id);
	task_wait_for = true;

	return 0;
}

int main(int argc, char *argv[])
{
	errno_t rc;
	task_exit_t texit;
	int retval;

	printf("System Call / IPC Tracer\n");
	printf("Controls: Q - Quit, P - Pause, R - Resume\n");

	display_mask = DM_THREAD | DM_SYSTEM | DM_USER;

	if (parse_args(argc, argv) < 0)
		return 1;

	main_init();

	rc = connect_task(task_id);
	if (rc != EOK) {
		printf("Failed connecting to task %" PRIu64 ".\n", task_id);
		return 1;
	}

	printf("Connected to task %" PRIu64 ".\n", task_id);

	if (task_ldr != NULL)
		program_run();

	cev_fibril_start();
	trace_task(task_id);

	if (task_wait_for) {
		printf("Waiting for task to exit.\n");

		rc = task_wait_task_id(task_id, &texit, &retval);
		if (rc != EOK) {
			printf("Failed waiting for task.\n");
			return -1;
		}

		if (texit == TASK_EXIT_NORMAL) {
			printf("Task exited normally, return value %d.\n",
			    retval);
		} else {
			printf("Task exited unexpectedly.\n");
		}
	}

	return 0;
}

/** @}
 */
