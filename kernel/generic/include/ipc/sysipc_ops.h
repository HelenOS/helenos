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

typedef struct {
	/**
	 * This callback is called from request_preprocess().
	 * 
	 * Context:		caller
	 * Caller alive:	guaranteed
	 * Races with:		N/A
	 * Invoked on:		all calls
	 */
	int (* request_preprocess)(call_t *, phone_t *);

	/**
	 * This callback is called when the IPC cleanup code wins the race to
	 * forget the call.
	 *
	 * Context:		caller
	 * Caller alive:	guaranteed
	 * Races with:		request_process(), answer_cleanup()
	 * Invoked on:		all forgotten calls
	 */	
	void (* request_forget)(call_t *);

	/**
	 * This callback is called from process_request().
	 *
	 * Context:		callee
	 * Caller alive:	no guarantee
	 * Races with:		request_forget()
	 * Invoked on:		all calls
	 */	
	int (* request_process)(call_t *, answerbox_t *);

	/**
	 * This callback is called when answer_preprocess() loses the race to
	 * answer the call.
	 *
	 * Context:		callee
	 * Caller alive:	no guarantee
	 * Races with:		request_forget()
	 * Invoked on:		all forgotten calls
	 */
	void (* answer_cleanup)(call_t *, ipc_data_t *);

	/**
	 * This callback is called when answer_preprocess() wins the race to
	 * answer the call.
	 *
	 * Context:		callee
	 * Caller alive:	guaranteed
	 * Races with:		N/A
	 * Invoked on:		all answered calls
	 */
	int (* answer_preprocess)(call_t *, ipc_data_t *);

	/**
	 * This callback is called from process_answer().
	 *
	 * Context:		caller
	 * Caller alive:	guaranteed
	 * Races with:		N/A
	 * Invoked on:		all answered calls
	 */
	int (* answer_process)(call_t *);
} sysipc_ops_t;

extern sysipc_ops_t *sysipc_ops_get(sysarg_t);

extern int null_request_preprocess(call_t *, phone_t *);
extern void null_request_forget(call_t *);
extern int null_request_process(call_t *, answerbox_t *);
extern void null_answer_cleanup(call_t *, ipc_data_t *);
extern int null_answer_preprocess(call_t *, ipc_data_t *);
extern int null_answer_process(call_t *);

#endif

/** @}
 */
