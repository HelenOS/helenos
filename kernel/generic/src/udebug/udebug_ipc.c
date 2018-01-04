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
 *
 * This module handles udebug IPC messages and calls the appropriate
 * functions from the udebug_ops module which implement them.
 */

#include <assert.h>
#include <proc/task.h>
#include <proc/thread.h>
#include <mm/as.h>
#include <arch.h>
#include <errno.h>
#include <ipc/ipc.h>
#include <syscall/copy.h>
#include <udebug/udebug.h>
#include <udebug/udebug_ops.h>
#include <udebug/udebug_ipc.h>

errno_t udebug_request_preprocess(call_t *call, phone_t *phone)
{
	switch (IPC_GET_ARG1(call->data)) {
	/* future UDEBUG_M_REGS_WRITE, UDEBUG_M_MEM_WRITE: */
	default:
		break;
	}

	return 0;
}

/** Process a BEGIN call.
 *
 * Initiates a debugging session for the current task. The reply
 * to this call may or may not be sent before this function returns.
 *
 * @param call	The call structure.
 */
static void udebug_receive_begin(call_t *call)
{
	errno_t rc;
	bool active;

	rc = udebug_begin(call, &active);
	if (rc != EOK) {
		IPC_SET_RETVAL(call->data, rc);
		ipc_answer(&TASK->kb.box, call);
		return;
	}

	/*
	 * If the initialization of the debugging session has finished,
	 * send a reply.
	 */
	if (active) {
		IPC_SET_RETVAL(call->data, EOK);
		ipc_answer(&TASK->kb.box, call);
	}
}

/** Process an END call.
 *
 * Terminates the debugging session for the current task.
 * @param call	The call structure.
 */
static void udebug_receive_end(call_t *call)
{
	errno_t rc;

	rc = udebug_end();

	IPC_SET_RETVAL(call->data, rc);
	ipc_answer(&TASK->kb.box, call);
}

/** Process a SET_EVMASK call.
 *
 * Sets an event mask for the current debugging session.
 * @param call	The call structure.
 */
static void udebug_receive_set_evmask(call_t *call)
{
	errno_t rc;
	udebug_evmask_t mask;

	mask = IPC_GET_ARG2(call->data);
	rc = udebug_set_evmask(mask);

	IPC_SET_RETVAL(call->data, rc);
	ipc_answer(&TASK->kb.box, call);
}


/** Process a GO call.
 *
 * Resumes execution of the specified thread.
 * @param call	The call structure.
 */
static void udebug_receive_go(call_t *call)
{
	thread_t *t;
	errno_t rc;

	t = (thread_t *)IPC_GET_ARG2(call->data);

	rc = udebug_go(t, call);
	if (rc != EOK) {
		IPC_SET_RETVAL(call->data, rc);
		ipc_answer(&TASK->kb.box, call);
		return;
	}
}

/** Process a STOP call.
 *
 * Suspends execution of the specified thread.
 * @param call	The call structure.
 */
static void udebug_receive_stop(call_t *call)
{
	thread_t *t;
	errno_t rc;

	t = (thread_t *)IPC_GET_ARG2(call->data);

	rc = udebug_stop(t, call);
	IPC_SET_RETVAL(call->data, rc);
	ipc_answer(&TASK->kb.box, call);
}

/** Process a THREAD_READ call.
 *
 * Reads the list of hashes of the (userspace) threads in the current task.
 * @param call	The call structure.
 */
static void udebug_receive_thread_read(call_t *call)
{
	uintptr_t uspace_addr;
	size_t buf_size;
	void *buffer;
	size_t copied, needed;
	errno_t rc;

	uspace_addr = IPC_GET_ARG2(call->data);	/* Destination address */
	buf_size = IPC_GET_ARG3(call->data);	/* Dest. buffer size */

	/*
	 * Read thread list. Variable n will be filled with actual number
	 * of threads times thread-id size.
	 */
	rc = udebug_thread_read(&buffer, buf_size, &copied, &needed);
	if (rc != EOK) {
		IPC_SET_RETVAL(call->data, rc);
		ipc_answer(&TASK->kb.box, call);
		return;
	}

	/*
	 * Make use of call->buffer to transfer data to caller's userspace
	 */

	IPC_SET_RETVAL(call->data, 0);
	/* ARG1=dest, ARG2=size as in IPC_M_DATA_READ so that
	   same code in process_answer() can be used 
	   (no way to distinguish method in answer) */
	IPC_SET_ARG1(call->data, uspace_addr);
	IPC_SET_ARG2(call->data, copied);
	IPC_SET_ARG3(call->data, needed);
	call->buffer = buffer;

	ipc_answer(&TASK->kb.box, call);
}

/** Process a NAME_READ call.
 *
 * Returns a string containing the name of the task.
 *
 * @param call	The call structure.
 */
static void udebug_receive_name_read(call_t *call)
{
	sysarg_t uspace_addr;
	sysarg_t to_copy;
	size_t data_size;
	size_t buf_size;
	void *data;

	uspace_addr = IPC_GET_ARG2(call->data);	/* Destination address */
	buf_size = IPC_GET_ARG3(call->data);	/* Dest. buffer size */

	/*
	 * Read task name.
	 */
	udebug_name_read((char **) &data, &data_size);

	/* Copy MAX(buf_size, data_size) bytes */

	if (buf_size > data_size)
		to_copy = data_size;
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

	IPC_SET_ARG3(call->data, data_size);
	call->buffer = data;

	ipc_answer(&TASK->kb.box, call);
}

/** Process an AREAS_READ call.
 *
 * Returns a list of address space areas in the current task, as an array
 * of as_area_info_t structures.
 *
 * @param call	The call structure.
 */
static void udebug_receive_areas_read(call_t *call)
{
	sysarg_t uspace_addr;
	sysarg_t to_copy;
	size_t data_size;
	size_t buf_size;
	void *data;

	uspace_addr = IPC_GET_ARG2(call->data);	/* Destination address */
	buf_size = IPC_GET_ARG3(call->data);	/* Dest. buffer size */

	/*
	 * Read area list.
	 */
	as_get_area_info(AS, (as_area_info_t **) &data, &data_size);

	/* Copy MAX(buf_size, data_size) bytes */

	if (buf_size > data_size)
		to_copy = data_size;
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

	IPC_SET_ARG3(call->data, data_size);
	call->buffer = data;

	ipc_answer(&TASK->kb.box, call);
}


/** Process an ARGS_READ call.
 *
 * Reads the argument of a current syscall event (SYSCALL_B or SYSCALL_E).
 * @param call	The call structure.
 */
static void udebug_receive_args_read(call_t *call)
{
	thread_t *t;
	sysarg_t uspace_addr;
	errno_t rc;
	void *buffer;

	t = (thread_t *)IPC_GET_ARG2(call->data);

	rc = udebug_args_read(t, &buffer);
	if (rc != EOK) {
		IPC_SET_RETVAL(call->data, rc);
		ipc_answer(&TASK->kb.box, call);
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
	IPC_SET_ARG2(call->data, 6 * sizeof(sysarg_t));
	call->buffer = buffer;

	ipc_answer(&TASK->kb.box, call);
}

/** Receive a REGS_READ call.
 *
 * Reads the thread's register state (istate structure).
 */
static void udebug_receive_regs_read(call_t *call)
{
	thread_t *t;
	sysarg_t uspace_addr;
	sysarg_t to_copy;
	void *buffer = NULL;
	errno_t rc;

	t = (thread_t *) IPC_GET_ARG2(call->data);

	rc = udebug_regs_read(t, &buffer);
	if (rc != EOK) {
		IPC_SET_RETVAL(call->data, rc);
		ipc_answer(&TASK->kb.box, call);
		return;
	}

	assert(buffer != NULL);

	/*
	 * Make use of call->buffer to transfer data to caller's userspace
	 */

	uspace_addr = IPC_GET_ARG3(call->data);
	to_copy = sizeof(istate_t);

	IPC_SET_RETVAL(call->data, 0);
	/* ARG1=dest, ARG2=size as in IPC_M_DATA_READ so that
	   same code in process_answer() can be used 
	   (no way to distinguish method in answer) */
	IPC_SET_ARG1(call->data, uspace_addr);
	IPC_SET_ARG2(call->data, to_copy);

	call->buffer = buffer;

	ipc_answer(&TASK->kb.box, call);
}


/** Process an MEM_READ call.
 *
 * Reads memory of the current (debugged) task.
 * @param call	The call structure.
 */
static void udebug_receive_mem_read(call_t *call)
{
	sysarg_t uspace_dst;
	sysarg_t uspace_src;
	unsigned size;
	void *buffer = NULL;
	errno_t rc;

	uspace_dst = IPC_GET_ARG2(call->data);
	uspace_src = IPC_GET_ARG3(call->data);
	size = IPC_GET_ARG4(call->data);

	rc = udebug_mem_read(uspace_src, size, &buffer);
	if (rc != EOK) {
		IPC_SET_RETVAL(call->data, rc);
		ipc_answer(&TASK->kb.box, call);
		return;
	}

	assert(buffer != NULL);

	IPC_SET_RETVAL(call->data, 0);
	/* ARG1=dest, ARG2=size as in IPC_M_DATA_READ so that
	   same code in process_answer() can be used 
	   (no way to distinguish method in answer) */
	IPC_SET_ARG1(call->data, uspace_dst);
	IPC_SET_ARG2(call->data, size);
	call->buffer = buffer;

	ipc_answer(&TASK->kb.box, call);
}

/** Handle a debug call received on the kernel answerbox.
 *
 * This is called by the kbox servicing thread. Verifies that the sender
 * is indeed the debugger and calls the appropriate processing function.
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
			ipc_answer(&TASK->kb.box, call);
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
	case UDEBUG_M_NAME_READ:
		udebug_receive_name_read(call);
		break;
	case UDEBUG_M_AREAS_READ:
		udebug_receive_areas_read(call);
		break;
	case UDEBUG_M_ARGS_READ:
		udebug_receive_args_read(call);
		break;
	case UDEBUG_M_REGS_READ:
		udebug_receive_regs_read(call);
		break;
	case UDEBUG_M_MEM_READ:
		udebug_receive_mem_read(call);
		break;
	}
}

/** @}
 */
