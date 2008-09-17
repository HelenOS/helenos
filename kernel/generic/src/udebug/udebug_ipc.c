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

/** @addtogroup generic
 * @{
 */

/**
 * @file
 * @brief	Udebug IPC message handling.
 */
 
#include <proc/task.h>
#include <proc/thread.h>
#include <arch.h>
#include <errno.h>
#include <ipc/ipc.h>
#include <syscall/copy.h>
#include <udebug/udebug.h>
#include <udebug/udebug_ops.h>
#include <udebug/udebug_ipc.h>

int udebug_request_preprocess(call_t *call, phone_t *phone)
{
	switch (IPC_GET_ARG1(call->data)) {
	/* future UDEBUG_M_REGS_WRITE, UDEBUG_M_MEM_WRITE: */
	default:
		break;
	}

	return 0;
}

static void udebug_receive_begin(call_t *call)
{
	int rc;

	rc = udebug_begin(call);
	if (rc < 0) {
		IPC_SET_RETVAL(call->data, rc);
		ipc_answer(&TASK->kernel_box, call);
		return;
	}

	if (rc != 0) {
		IPC_SET_RETVAL(call->data, 0);
		ipc_answer(&TASK->kernel_box, call);
	}
}

static void udebug_receive_end(call_t *call)
{
	int rc;

	rc = udebug_end();

	IPC_SET_RETVAL(call->data, rc);
	ipc_answer(&TASK->kernel_box, call);
}

static void udebug_receive_set_evmask(call_t *call)
{
	int rc;
	udebug_evmask_t mask;

	mask = IPC_GET_ARG2(call->data);
	rc = udebug_set_evmask(mask);

	IPC_SET_RETVAL(call->data, rc);
	ipc_answer(&TASK->kernel_box, call);
}


static void udebug_receive_go(call_t *call)
{
	thread_t *t;
	int rc;

	t = (thread_t *)IPC_GET_ARG2(call->data);

	rc = udebug_go(t, call);
	if (rc < 0) {
		IPC_SET_RETVAL(call->data, rc);
		ipc_answer(&TASK->kernel_box, call);
		return;
	}
}

static void udebug_receive_stop(call_t *call)
{
	thread_t *t;
	int rc;

	t = (thread_t *)IPC_GET_ARG2(call->data);

	rc = udebug_stop(t, call);
	IPC_SET_RETVAL(call->data, rc);
	ipc_answer(&TASK->kernel_box, call);
}

static void udebug_receive_thread_read(call_t *call)
{
	unative_t uspace_addr;
	unative_t to_copy;
	unsigned total_bytes;
	unsigned buf_size;
	void *buffer;
	size_t n;
	int rc;

	uspace_addr = IPC_GET_ARG2(call->data);	/* Destination address */
	buf_size = IPC_GET_ARG3(call->data);	/* Dest. buffer size */

	/*
	 * Read thread list. Variable n will be filled with actual number
	 * of threads times thread-id size.
	 */
	rc = udebug_thread_read(&buffer, buf_size, &n);
	if (rc < 0) {
		IPC_SET_RETVAL(call->data, rc);
		ipc_answer(&TASK->kernel_box, call);
		return;
	}

	total_bytes = n;

	/* Copy MAX(buf_size, total_bytes) bytes */

	if (buf_size > total_bytes)
		to_copy = total_bytes;
	else
		to_copy = buf_size;

	/*
	 * Make use of call->buffer to transfer data to caller's userspace
	 */

	IPC_SET_RETVAL(call->data, 0);
	/* ARG1=dest, ARG2=size as in IPC_M_DATA_READ so that
	   same code in process_answer() can be used 
	   (no way to distinguish method in answer) */
	IPC_SET_ARG1(call->data, uspace_addr);
	IPC_SET_ARG2(call->data, to_copy);

	IPC_SET_ARG3(call->data, total_bytes);
	call->buffer = buffer;

	ipc_answer(&TASK->kernel_box, call);
}

static void udebug_receive_args_read(call_t *call)
{
	thread_t *t;
	unative_t uspace_addr;
	int rc;
	void *buffer;

	t = (thread_t *)IPC_GET_ARG2(call->data);

	rc = udebug_args_read(t, &buffer);
	if (rc != EOK) {
		IPC_SET_RETVAL(call->data, rc);
		ipc_answer(&TASK->kernel_box, call);
		return;
	}

	/*
	 * Make use of call->buffer to transfer data to caller's userspace
	 */

	uspace_addr = IPC_GET_ARG3(call->data);

	IPC_SET_RETVAL(call->data, 0);
	/* ARG1=dest, ARG2=size as in IPC_M_DATA_READ so that
	   same code in process_answer() can be used 
	   (no way to distinguish method in answer) */
	IPC_SET_ARG1(call->data, uspace_addr);
	IPC_SET_ARG2(call->data, 6 * sizeof(unative_t));
	call->buffer = buffer;

	ipc_answer(&TASK->kernel_box, call);
}

static void udebug_receive_mem_read(call_t *call)
{
	unative_t uspace_dst;
	unative_t uspace_src;
	unsigned size;
	void *buffer;
	int rc;

	uspace_dst = IPC_GET_ARG2(call->data);
	uspace_src = IPC_GET_ARG3(call->data);
	size = IPC_GET_ARG4(call->data);

	rc = udebug_mem_read(uspace_src, size, &buffer);
	if (rc < 0) {
		IPC_SET_RETVAL(call->data, rc);
		ipc_answer(&TASK->kernel_box, call);
		return;
	}

	IPC_SET_RETVAL(call->data, 0);
	/* ARG1=dest, ARG2=size as in IPC_M_DATA_READ so that
	   same code in process_answer() can be used 
	   (no way to distinguish method in answer) */
	IPC_SET_ARG1(call->data, uspace_dst);
	IPC_SET_ARG2(call->data, size);
	call->buffer = buffer;

	ipc_answer(&TASK->kernel_box, call);
}

/**
 * Handle a debug call received on the kernel answerbox.
 *
 * This is called by the kbox servicing thread.
 */
void udebug_call_receive(call_t *call)
{
	int debug_method;

	debug_method = IPC_GET_ARG1(call->data);

	if (debug_method != UDEBUG_M_BEGIN) {
		/*
		 * Verify that the sender is this task's debugger.
		 * Note that this is the only thread that could change
		 * TASK->debugger. Therefore no locking is necessary
		 * and the sender can be safely considered valid until
		 * control exits this function.
		 */
		if (TASK->udebug.debugger != call->sender) {
			IPC_SET_RETVAL(call->data, EINVAL);
			ipc_answer(&TASK->kernel_box, call);
			return;	
		}
	}

	switch (debug_method) {
	case UDEBUG_M_BEGIN:
		udebug_receive_begin(call);
		break;
	case UDEBUG_M_END:
		udebug_receive_end(call);
		break;
	case UDEBUG_M_SET_EVMASK:
		udebug_receive_set_evmask(call);
		break;
	case UDEBUG_M_GO:
		udebug_receive_go(call);
		break;
	case UDEBUG_M_STOP:
		udebug_receive_stop(call);
		break;
	case UDEBUG_M_THREAD_READ:
		udebug_receive_thread_read(call);
		break;
	case UDEBUG_M_ARGS_READ:
		udebug_receive_args_read(call);
		break;
	case UDEBUG_M_MEM_READ:
		udebug_receive_mem_read(call);
		break;
	}
}

/** @}
 */
