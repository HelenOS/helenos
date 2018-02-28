/*
 * Copyright (c) 2012 Jakub Jermar
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

/** @addtogroup genericipc
 * @{
 */
/** @file
 */

#ifndef KERN_SYSIPC_OPS_H_
#define KERN_SYSIPC_OPS_H_

#include <ipc/ipc.h>

#define SYSIPC_OP(op, call, ...) \
	({ \
		sysipc_ops_t *ops = sysipc_ops_get((call)->request_method); \
		assert(ops->op); \
		ops->op((call), ##__VA_ARGS__); \
	})

/**
 * This header declares the per-method IPC callbacks. Using these callbacks,
 * each IPC method (but system methods in particular), can define actions that
 * will be called at specific moments in the call life-cycle.
 *
 * Normally, the kernel will attempt to invoke the following callbacks in the
 * following order on each call:
 *
 * request_preprocess()
 * request_process()
 * answer_preprocess()
 * answer_process()
 *
 * This callback invocation sequence is a natural order of processing. Note,
 * however, that due to various special circumstances, callbacks may be called
 * also in a different than natural order of processing. This means that some
 * callbacks may be skipped and some others may be called instead.
 *
 * The additional callbacks that may be called are as follows:
 *
 * request_forget()
 * answer_cleanup()
 *
 * There are several notable scenarios in which some callbacks of the natural
 * order of processing will be skipped.
 *
 * The request_process(), answer_preprocess() and answer_process() callbacks
 * will be skipped if the call cannot be delivered to the callee. This may
 * happen when e.g. the request_preprocess() callback fails or the connection
 * to the callee is not functional. The next callback that will be invoked on
 * the call is request_forget().
 *
 * The comments for each callback type describe the specifics of each callback
 * such as the context in which it is invoked and various constraints.
 */

typedef struct {
	/**
	 * This callback is called from request_preprocess().
	 *
	 * Context:		caller
	 * Caller alive:	guaranteed
	 * Races with:		N/A
	 * Invoked on:		all calls
	 */
	errno_t (*request_preprocess)(call_t *, phone_t *);

	/**
	 * This callback is called when the IPC cleanup code wins the race to
	 * forget the call.
	 *
	 * Context:		caller
	 * Caller alive:	guaranteed
	 * Races with:		request_process(), answer_cleanup(),
	 *			_ipc_answer_free_call()
	 * Invoked on:		all forgotten calls
	 */
	errno_t (*request_forget)(call_t *);

	/**
	 * This callback is called from process_request().
	 *
	 * Context:		callee
	 * Caller alive:	no guarantee
	 * Races with:		request_forget()
	 * Invoked on:		all calls delivered to the callee
	 */
	int (*request_process)(call_t *, answerbox_t *);

	/**
	 * This callback is called when answer_preprocess() loses the race to
	 * answer the call.
	 *
	 * Context:		callee
	 * Caller alive:	no guarantee
	 * Races with:		request_forget()
	 * Invoked on:		all forgotten calls
	 */
	errno_t (*answer_cleanup)(call_t *, ipc_data_t *);

	/**
	 * This callback is called when answer_preprocess() wins the race to
	 * answer the call.
	 *
	 * Context:		callee
	 * Caller alive:	guaranteed
	 * Races with:		N/A
	 * Invoked on:		all answered calls
	 */
	errno_t (*answer_preprocess)(call_t *, ipc_data_t *);

	/**
	 * This callback is called from process_answer().
	 *
	 * Context:		caller
	 * Caller alive:	guaranteed
	 * Races with:		N/A
	 * Invoked on:		all answered calls
	 */
	errno_t (*answer_process)(call_t *);
} sysipc_ops_t;

extern sysipc_ops_t *sysipc_ops_get(sysarg_t);

extern errno_t null_request_preprocess(call_t *, phone_t *);
extern errno_t null_request_forget(call_t *);
extern int null_request_process(call_t *, answerbox_t *);
extern errno_t null_answer_cleanup(call_t *, ipc_data_t *);
extern errno_t null_answer_preprocess(call_t *, ipc_data_t *);
extern errno_t null_answer_process(call_t *);

#endif

/** @}
 */
