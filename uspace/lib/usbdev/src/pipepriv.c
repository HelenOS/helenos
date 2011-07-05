/*
 * Copyright (c) 2011 Vojtech Horky
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

/** @addtogroup libusbdev
 * @{
 */
/** @file
 * Library internal functions on USB pipes (implementation).
 */
#include "pipepriv.h"
#include <devman.h>
#include <errno.h>
#include <assert.h>

/** Ensure exclusive access to the IPC phone of given pipe.
 *
 * @param pipe Pipe to be exclusively accessed.
 */
void pipe_start_transaction(usb_pipe_t *pipe)
{
	fibril_mutex_lock(&pipe->hc_sess_mutex);
}

/** Terminate exclusive access to the IPC phone of given pipe.
 *
 * @param pipe Pipe to be released from exclusive usage.
 */
void pipe_end_transaction(usb_pipe_t *pipe)
{
	fibril_mutex_unlock(&pipe->hc_sess_mutex);
}

/** Ensure exclusive access to the pipe as a whole.
 *
 * @param pipe Pipe to be exclusively accessed.
 */
void pipe_acquire(usb_pipe_t *pipe)
{
	fibril_mutex_lock(&pipe->guard);
}

/** Terminate exclusive access to the pipe as a whole.
 *
 * @param pipe Pipe to be released from exclusive usage.
 */
void pipe_release(usb_pipe_t *pipe)
{
	fibril_mutex_unlock(&pipe->guard);
}

/** Add reference of active transfers over the pipe.
 *
 * @param pipe The USB pipe.
 * @param hide_failure Whether to hide failure when adding reference
 *	(use soft refcount).
 * @return Error code.
 * @retval EOK Currently always.
 */
int pipe_add_ref(usb_pipe_t *pipe, bool hide_failure)
{
	pipe_acquire(pipe);
	
	if (pipe->refcount == 0) {
		/* Need to open the phone by ourselves. */
		async_sess_t *sess =
		    devman_device_connect(EXCHANGE_SERIALIZE, pipe->wire->hc_handle, 0);
		if (!sess) {
			if (hide_failure) {
				pipe->refcount_soft++;
				pipe_release(pipe);
				return EOK;
			}
			
			pipe_release(pipe);
			return ENOMEM;
		}
		
		/*
		 * No locking is needed, refcount is zero and whole pipe
		 * mutex is locked.
		 */
		
		pipe->hc_sess = sess;
	}
	
	pipe->refcount++;
	pipe_release(pipe);
	
	return EOK;
}

/** Drop active transfer reference on the pipe.
 *
 * @param pipe The USB pipe.
 */
void pipe_drop_ref(usb_pipe_t *pipe)
{
	pipe_acquire(pipe);
	
	if (pipe->refcount_soft > 0) {
		pipe->refcount_soft--;
		pipe_release(pipe);
		return;
	}
	
	assert(pipe->refcount > 0);
	
	pipe->refcount--;
	
	if (pipe->refcount == 0) {
		/* We were the last users, let's hang-up. */
		async_hangup(pipe->hc_sess);
		pipe->hc_sess = NULL;
	}
	
	pipe_release(pipe);
}


/**
 * @}
 */
