/*
 * Copyright (c) 2006 Ondrej Palkovsky
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

#include <arch.h>
#include <proc/task.h>
#include <proc/thread.h>
#include <errno.h>
#include <memstr.h>
#include <debug.h>
#include <ipc/ipc.h>
#include <abi/ipc/methods.h>
#include <ipc/sysipc.h>
#include <ipc/irq.h>
#include <ipc/ipcrsc.h>
#include <ipc/event.h>
#include <ipc/kbox.h>
#include <synch/waitq.h>
#include <udebug/udebug_ipc.h>
#include <arch/interrupt.h>
#include <syscall/copy.h>
#include <security/cap.h>
#include <console/console.h>
#include <mm/as.h>
#include <print.h>
#include <macros.h>

/**
 * Maximum buffer size allowed for IPC_M_DATA_WRITE and IPC_M_DATA_READ
 * requests.
 */
#define DATA_XFER_LIMIT  (64 * 1024)

#define STRUCT_TO_USPACE(dst, src)  copy_to_uspace((dst), (src), sizeof(*(src)))

/** Get phone from the current task by ID.
 *
 * @param phoneid Phone ID.
 * @param phone   Place to store pointer to phone.
 *
 * @return EOK on success, EINVAL if ID is invalid.
 *
 */
static int phone_get(sysarg_t phoneid, phone_t **phone)
{
	if (phoneid >= IPC_MAX_PHONES)
		return EINVAL;
	
	*phone = &TASK->phones[phoneid];
	return EOK;
}

/** Decide if the interface and method is a system method.
 *
 * @param imethod Interface and method to be decided.
 *
 * @return True if the interface and method is a system
 *         interface and method.
 *
 */
static inline bool method_is_system(sysarg_t imethod)
{
	if (imethod <= IPC_M_LAST_SYSTEM)
		return true;
	
	return false;
}

/** Decide if the message with this interface and method is forwardable.
 *
 * Some system messages may be forwarded, for some of them
 * it is useless.
 *
 * @param imethod Interface and method to be decided.
 *
 * @return True if the interface and method is forwardable.
 *
 */
static inline bool method_is_forwardable(sysarg_t imethod)
{
	switch (imethod) {
	case IPC_M_CONNECTION_CLONE:
	case IPC_M_CLONE_ESTABLISH:
	case IPC_M_PHONE_HUNGUP:
		/* This message is meant only for the original recipient. */
		return false;
	default:
		return true;
	}
}

/** Decide if the message with this interface and method is immutable on forward.
 *
 * Some system messages may be forwarded but their content cannot be altered.
 *
 * @param imethod Interface and method to be decided.
 *
 * @return True if the interface and method is immutable on forward.
 *
 */
static inline bool method_is_immutable(sysarg_t imethod)
{
	switch (imethod) {
	case IPC_M_SHARE_OUT:
	case IPC_M_SHARE_IN:
	case IPC_M_DATA_WRITE:
	case IPC_M_DATA_READ:
	case IPC_M_STATE_CHANGE_AUTHORIZE:
		return true;
	default:
		return false;
	}
}


/***********************************************************************
 * Functions that preprocess answer before sending it to the recepient.
 ***********************************************************************/

/** Decide if the caller (e.g. ipc_answer()) should save the old call contents
 * for answer_preprocess().
 *
 * @param call Call structure to be decided.
 *
 * @return true if the old call contents should be saved.
 *
 */
static inline bool answer_need_old(call_t *call)
{
	switch (IPC_GET_IMETHOD(call->data)) {
	case IPC_M_CONNECTION_CLONE:
	case IPC_M_CLONE_ESTABLISH:
	case IPC_M_CONNECT_TO_ME:
	case IPC_M_CONNECT_ME_TO:
	case IPC_M_SHARE_OUT:
	case IPC_M_SHARE_IN:
	case IPC_M_DATA_WRITE:
	case IPC_M_DATA_READ:
	case IPC_M_STATE_CHANGE_AUTHORIZE:
		return true;
	default:
		return false;
	}
}

/** Interpret process answer as control information.
 *
 * This function is called directly after sys_ipc_answer().
 *
 * @param answer  Call structure with the answer.
 * @param olddata Saved data of the request.
 *
 * @return Return 0 on success or an error code.
 *
 */
static inline int answer_preprocess(call_t *answer, ipc_data_t *olddata)
{
	if ((native_t) IPC_GET_RETVAL(answer->data) == EHANGUP) {
		/* In case of forward, hangup the forwared phone,
		 * not the originator
		 */
		mutex_lock(&answer->data.phone->lock);
		irq_spinlock_lock(&TASK->answerbox.lock, true);
		if (answer->data.phone->state == IPC_PHONE_CONNECTED) {
			list_remove(&answer->data.phone->link);
			answer->data.phone->state = IPC_PHONE_SLAMMED;
		}
		irq_spinlock_unlock(&TASK->answerbox.lock, true);
		mutex_unlock(&answer->data.phone->lock);
	}
	
	if (!olddata)
		return 0;
	
	if (IPC_GET_IMETHOD(*olddata) == IPC_M_CONNECTION_CLONE) {
		int phoneid = IPC_GET_ARG1(*olddata);
		phone_t *phone = &TASK->phones[phoneid];
		
		if (IPC_GET_RETVAL(answer->data) != EOK) {
			/*
			 * The recipient of the cloned phone rejected the offer.
			 * In this case, the connection was established at the
			 * request time and therefore we need to slam the phone.
			 * We don't merely hangup as that would result in
			 * sending IPC_M_HUNGUP to the third party on the
			 * other side of the cloned phone.
			 */
			mutex_lock(&phone->lock);
			if (phone->state == IPC_PHONE_CONNECTED) {
				irq_spinlock_lock(&phone->callee->lock, true);
				list_remove(&phone->link);
				phone->state = IPC_PHONE_SLAMMED;
				irq_spinlock_unlock(&phone->callee->lock, true);
			}
			mutex_unlock(&phone->lock);
		}
	} else if (IPC_GET_IMETHOD(*olddata) == IPC_M_CLONE_ESTABLISH) {
		phone_t *phone = (phone_t *) IPC_GET_ARG5(*olddata);
		
		if (IPC_GET_RETVAL(answer->data) != EOK) {
			/*
			 * The other party on the cloned phoned rejected our
			 * request for connection on the protocol level.
			 * We need to break the connection without sending
			 * IPC_M_HUNGUP back.
			 */
			mutex_lock(&phone->lock);
			if (phone->state == IPC_PHONE_CONNECTED) {
				irq_spinlock_lock(&phone->callee->lock, true);
				list_remove(&phone->link);
				phone->state = IPC_PHONE_SLAMMED;
				irq_spinlock_unlock(&phone->callee->lock, true);
			}
			mutex_unlock(&phone->lock);
		}
	} else if (IPC_GET_IMETHOD(*olddata) == IPC_M_CONNECT_TO_ME) {
		int phoneid = IPC_GET_ARG5(*olddata);
		
		if (IPC_GET_RETVAL(answer->data) != EOK) {
			/* The connection was not accepted */
			phone_dealloc(phoneid);
		} else {
			/* The connection was accepted */
			phone_connect(phoneid, &answer->sender->answerbox);
			/* Set 'phone hash' as arg5 of response */
			IPC_SET_ARG5(answer->data,
			    (sysarg_t) &TASK->phones[phoneid]);
		}
	} else if (IPC_GET_IMETHOD(*olddata) == IPC_M_CONNECT_ME_TO) {
		/* If the users accepted call, connect */
		if (IPC_GET_RETVAL(answer->data) == EOK) {
			ipc_phone_connect((phone_t *) IPC_GET_ARG5(*olddata),
			    &TASK->answerbox);
		}
	} else if (IPC_GET_IMETHOD(*olddata) == IPC_M_SHARE_OUT) {
		if (!IPC_GET_RETVAL(answer->data)) {
			/* Accepted, handle as_area receipt */
			
			irq_spinlock_lock(&answer->sender->lock, true);
			as_t *as = answer->sender->as;
			irq_spinlock_unlock(&answer->sender->lock, true);
			
			uintptr_t dst_base = (uintptr_t) -1;
			int rc = as_area_share(as, IPC_GET_ARG1(*olddata),
			    IPC_GET_ARG2(*olddata), AS, IPC_GET_ARG3(*olddata),
			    &dst_base, IPC_GET_ARG1(answer->data));
			
			if (rc == EOK)
				rc = copy_to_uspace((void *) IPC_GET_ARG2(answer->data),
				    &dst_base, sizeof(dst_base));
			
			IPC_SET_RETVAL(answer->data, rc);
			return rc;
		}
	} else if (IPC_GET_IMETHOD(*olddata) == IPC_M_SHARE_IN) {
		if (!IPC_GET_RETVAL(answer->data)) {
			irq_spinlock_lock(&answer->sender->lock, true);
			as_t *as = answer->sender->as;
			irq_spinlock_unlock(&answer->sender->lock, true);
			
			uintptr_t dst_base = (uintptr_t) -1;
			int rc = as_area_share(AS, IPC_GET_ARG1(answer->data),
			    IPC_GET_ARG1(*olddata), as, IPC_GET_ARG2(answer->data),
			    &dst_base, IPC_GET_ARG3(answer->data));
			IPC_SET_ARG4(answer->data, dst_base);
			IPC_SET_RETVAL(answer->data, rc);
		}
	} else if (IPC_GET_IMETHOD(*olddata) == IPC_M_DATA_READ) {
		ASSERT(!answer->buffer);
		if (!IPC_GET_RETVAL(answer->data)) {
			/* The recipient agreed to send data. */
			uintptr_t src = IPC_GET_ARG1(answer->data);
			uintptr_t dst = IPC_GET_ARG1(*olddata);
			size_t max_size = IPC_GET_ARG2(*olddata);
			size_t size = IPC_GET_ARG2(answer->data);
			if (size && size <= max_size) {
				/*
				 * Copy the destination VA so that this piece of
				 * information is not lost.
				 */
				IPC_SET_ARG1(answer->data, dst);
				
				answer->buffer = malloc(size, 0);
				int rc = copy_from_uspace(answer->buffer,
				    (void *) src, size);
				if (rc) {
					IPC_SET_RETVAL(answer->data, rc);
					free(answer->buffer);
					answer->buffer = NULL;
				}
			} else if (!size) {
				IPC_SET_RETVAL(answer->data, EOK);
			} else {
				IPC_SET_RETVAL(answer->data, ELIMIT);
			}
		}
	} else if (IPC_GET_IMETHOD(*olddata) == IPC_M_DATA_WRITE) {
		ASSERT(answer->buffer);
		if (!IPC_GET_RETVAL(answer->data)) {
			/* The recipient agreed to receive data. */
			uintptr_t dst = (uintptr_t)IPC_GET_ARG1(answer->data);
			size_t size = (size_t)IPC_GET_ARG2(answer->data);
			size_t max_size = (size_t)IPC_GET_ARG2(*olddata);
			
			if (size <= max_size) {
				int rc = copy_to_uspace((void *) dst,
				    answer->buffer, size);
				if (rc)
					IPC_SET_RETVAL(answer->data, rc);
			} else {
				IPC_SET_RETVAL(answer->data, ELIMIT);
			}
		}
		free(answer->buffer);
		answer->buffer = NULL;
	} else if (IPC_GET_IMETHOD(*olddata) == IPC_M_STATE_CHANGE_AUTHORIZE) {
		if (!IPC_GET_RETVAL(answer->data)) {
			/* The recipient authorized the change of state. */
			phone_t *recipient_phone;
			task_t *other_task_s;
			task_t *other_task_r;
			int rc;

			rc = phone_get(IPC_GET_ARG1(answer->data),
			    &recipient_phone);
			if (rc != EOK) {
				IPC_SET_RETVAL(answer->data, ENOENT);
				return ENOENT;
			}

			mutex_lock(&recipient_phone->lock);
			if (recipient_phone->state != IPC_PHONE_CONNECTED) {
				mutex_unlock(&recipient_phone->lock);
				IPC_SET_RETVAL(answer->data, EINVAL);
				return EINVAL;
			}

			other_task_r = recipient_phone->callee->task;
			other_task_s = (task_t *) IPC_GET_ARG5(*olddata);

			/*
			 * See if both the sender and the recipient meant the
			 * same third party task.
			 */
			if (other_task_r != other_task_s) {
				IPC_SET_RETVAL(answer->data, EINVAL);
				rc = EINVAL;
			} else {
				rc = event_task_notify_5(other_task_r,
				    EVENT_TASK_STATE_CHANGE, false,
				    IPC_GET_ARG1(*olddata),
				    IPC_GET_ARG2(*olddata),
				    IPC_GET_ARG3(*olddata),
				    LOWER32(olddata->task_id),
				    UPPER32(olddata->task_id));
				IPC_SET_RETVAL(answer->data, rc);
			}

			mutex_unlock(&recipient_phone->lock);
			return rc;
		}
	}
	
	return 0;
}

static void phones_lock(phone_t *p1, phone_t *p2)
{
	if (p1 < p2) {
		mutex_lock(&p1->lock);
		mutex_lock(&p2->lock);
	} else if (p1 > p2) {
		mutex_lock(&p2->lock);
		mutex_lock(&p1->lock);
	} else
		mutex_lock(&p1->lock);
}

static void phones_unlock(phone_t *p1, phone_t *p2)
{
	mutex_unlock(&p1->lock);
	if (p1 != p2)
		mutex_unlock(&p2->lock);
}

/** Called before the request is sent.
 *
 * @param call  Call structure with the request.
 * @param phone Phone that the call will be sent through.
 *
 * @return Return 0 on success, ELIMIT or EPERM on error.
 *
 */
static int request_preprocess(call_t *call, phone_t *phone)
{
	switch (IPC_GET_IMETHOD(call->data)) {
	case IPC_M_CONNECTION_CLONE: {
		phone_t *cloned_phone;
		if (phone_get(IPC_GET_ARG1(call->data), &cloned_phone) != EOK)
			return ENOENT;
		
		phones_lock(cloned_phone, phone);
		
		if ((cloned_phone->state != IPC_PHONE_CONNECTED) ||
		    phone->state != IPC_PHONE_CONNECTED) {
			phones_unlock(cloned_phone, phone);
			return EINVAL;
		}
		
		/*
		 * We can be pretty sure now that both tasks exist and we are
		 * connected to them. As we continue to hold the phone locks,
		 * we are effectively preventing them from finishing their
		 * potential cleanup.
		 *
		 */
		int newphid = phone_alloc(phone->callee->task);
		if (newphid < 0) {
			phones_unlock(cloned_phone, phone);
			return ELIMIT;
		}
		
		ipc_phone_connect(&phone->callee->task->phones[newphid],
		    cloned_phone->callee);
		phones_unlock(cloned_phone, phone);
		
		/* Set the new phone for the callee. */
		IPC_SET_ARG1(call->data, newphid);
		break;
	}
	case IPC_M_CLONE_ESTABLISH:
		IPC_SET_ARG5(call->data, (sysarg_t) phone);
		break;
	case IPC_M_CONNECT_ME_TO: {
		int newphid = phone_alloc(TASK);
		if (newphid < 0)
			return ELIMIT;
		
		/* Set arg5 for server */
		IPC_SET_ARG5(call->data, (sysarg_t) &TASK->phones[newphid]);
		call->flags |= IPC_CALL_CONN_ME_TO;
		call->priv = newphid;
		break;
	}
	case IPC_M_SHARE_OUT: {
		size_t size = as_area_get_size(IPC_GET_ARG1(call->data));
		if (!size)
			return EPERM;
		
		IPC_SET_ARG2(call->data, size);
		break;
	}
	case IPC_M_DATA_READ: {
		size_t size = IPC_GET_ARG2(call->data);
		if (size > DATA_XFER_LIMIT) {
			int flags = IPC_GET_ARG3(call->data);
			if (flags & IPC_XF_RESTRICT)
				IPC_SET_ARG2(call->data, DATA_XFER_LIMIT);
			else
				return ELIMIT;
		}
		break;
	}
	case IPC_M_DATA_WRITE: {
		uintptr_t src = IPC_GET_ARG1(call->data);
		size_t size = IPC_GET_ARG2(call->data);
		
		if (size > DATA_XFER_LIMIT) {
			int flags = IPC_GET_ARG3(call->data);
			if (flags & IPC_XF_RESTRICT) {
				size = DATA_XFER_LIMIT;
				IPC_SET_ARG2(call->data, size);
			} else
				return ELIMIT;
		}
		
		call->buffer = (uint8_t *) malloc(size, 0);
		int rc = copy_from_uspace(call->buffer, (void *) src, size);
		if (rc != 0) {
			free(call->buffer);
			return rc;
		}
		
		break;
	}
	case IPC_M_STATE_CHANGE_AUTHORIZE: {
		phone_t *sender_phone;
		task_t *other_task_s;

		if (phone_get(IPC_GET_ARG5(call->data), &sender_phone) != EOK)
			return ENOENT;

		mutex_lock(&sender_phone->lock);
		if (sender_phone->state != IPC_PHONE_CONNECTED) {
			mutex_unlock(&sender_phone->lock);
			return EINVAL;
		}

		other_task_s = sender_phone->callee->task;

		mutex_unlock(&sender_phone->lock);

		/* Remember the third party task hash. */
		IPC_SET_ARG5(call->data, (sysarg_t) other_task_s);
		break;
	}
#ifdef CONFIG_UDEBUG
	case IPC_M_DEBUG:
		return udebug_request_preprocess(call, phone);
#endif
	default:
		break;
	}
	
	return 0;
}

/*******************************************************************************
 * Functions called to process received call/answer before passing it to uspace.
 *******************************************************************************/

/** Do basic kernel processing of received call answer.
 *
 * @param call Call structure with the answer.
 *
 */
static void process_answer(call_t *call)
{
	if (((native_t) IPC_GET_RETVAL(call->data) == EHANGUP) &&
	    (call->flags & IPC_CALL_FORWARDED))
		IPC_SET_RETVAL(call->data, EFORWARD);
	
	if (call->flags & IPC_CALL_CONN_ME_TO) {
		if (IPC_GET_RETVAL(call->data))
			phone_dealloc(call->priv);
		else
			IPC_SET_ARG5(call->data, call->priv);
	}
	
	if (call->buffer) {
		/*
		 * This must be an affirmative answer to IPC_M_DATA_READ
		 * or IPC_M_DEBUG/UDEBUG_M_MEM_READ...
		 *
		 */
		uintptr_t dst = IPC_GET_ARG1(call->data);
		size_t size = IPC_GET_ARG2(call->data);
		int rc = copy_to_uspace((void *) dst, call->buffer, size);
		if (rc)
			IPC_SET_RETVAL(call->data, rc);
		free(call->buffer);
		call->buffer = NULL;
	}
}

/** Do basic kernel processing of received call request.
 *
 * @param box  Destination answerbox structure.
 * @param call Call structure with the request.
 *
 * @return 0 if the call should be passed to userspace.
 * @return -1 if the call should be ignored.
 *
 */
static int process_request(answerbox_t *box, call_t *call)
{
	if (IPC_GET_IMETHOD(call->data) == IPC_M_CONNECT_TO_ME) {
		int phoneid = phone_alloc(TASK);
		if (phoneid < 0) {  /* Failed to allocate phone */
			IPC_SET_RETVAL(call->data, ELIMIT);
			ipc_answer(box, call);
			return -1;
		}
		
		IPC_SET_ARG5(call->data, phoneid);
	}
	
	switch (IPC_GET_IMETHOD(call->data)) {
	case IPC_M_DEBUG:
		return -1;
	default:
		break;
	}
	
	return 0;
}

/** Make a fast call over IPC, wait for reply and return to user.
 *
 * This function can handle only three arguments of payload, but is faster than
 * the generic function (i.e. sys_ipc_call_sync_slow()).
 *
 * @param phoneid Phone handle for the call.
 * @param imethod Interface and method of the call.
 * @param arg1    Service-defined payload argument.
 * @param arg2    Service-defined payload argument.
 * @param arg3    Service-defined payload argument.
 * @param data    Address of user-space structure where the reply call will
 *                be stored.
 *
 * @return 0 on success.
 * @return ENOENT if there is no such phone handle.
 *
 */
sysarg_t sys_ipc_call_sync_fast(sysarg_t phoneid, sysarg_t imethod,
    sysarg_t arg1, sysarg_t arg2, sysarg_t arg3, ipc_data_t *data)
{
	phone_t *phone;
	if (phone_get(phoneid, &phone) != EOK)
		return ENOENT;
	
	call_t *call = ipc_call_alloc(0);
	IPC_SET_IMETHOD(call->data, imethod);
	IPC_SET_ARG1(call->data, arg1);
	IPC_SET_ARG2(call->data, arg2);
	IPC_SET_ARG3(call->data, arg3);
	
	/*
	 * To achieve deterministic behavior, zero out arguments that are beyond
	 * the limits of the fast version.
	 */
	IPC_SET_ARG4(call->data, 0);
	IPC_SET_ARG5(call->data, 0);
	
	int res = request_preprocess(call, phone);
	int rc;
	
	if (!res) {
#ifdef CONFIG_UDEBUG
		udebug_stoppable_begin();
#endif
		rc = ipc_call_sync(phone, call);
#ifdef CONFIG_UDEBUG
		udebug_stoppable_end();
#endif
		
		if (rc != EOK) {
			/* The call will be freed by ipc_cleanup(). */
			return rc;
		}
		
		process_answer(call);
	} else
		IPC_SET_RETVAL(call->data, res);
	
	rc = STRUCT_TO_USPACE(&data->args, &call->data.args);
	ipc_call_free(call);
	if (rc != 0)
		return rc;
	
	return 0;
}

/** Make a synchronous IPC call allowing to transmit the entire payload.
 *
 * @param phoneid Phone handle for the call.
 * @param request User-space address of call data with the request.
 * @param reply   User-space address of call data where to store the
 *                answer.
 *
 * @return Zero on success or an error code.
 *
 */
sysarg_t sys_ipc_call_sync_slow(sysarg_t phoneid, ipc_data_t *request,
    ipc_data_t *reply)
{
	phone_t *phone;
	if (phone_get(phoneid, &phone) != EOK)
		return ENOENT;
	
	call_t *call = ipc_call_alloc(0);
	int rc = copy_from_uspace(&call->data.args, &request->args,
	    sizeof(call->data.args));
	if (rc != 0) {
		ipc_call_free(call);
		return (sysarg_t) rc;
	}
	
	int res = request_preprocess(call, phone);
	
	if (!res) {
#ifdef CONFIG_UDEBUG
		udebug_stoppable_begin();
#endif
		rc = ipc_call_sync(phone, call);
#ifdef CONFIG_UDEBUG
		udebug_stoppable_end();
#endif
		
		if (rc != EOK) {
			/* The call will be freed by ipc_cleanup(). */
			return rc;
		}
		
		process_answer(call);
	} else
		IPC_SET_RETVAL(call->data, res);
	
	rc = STRUCT_TO_USPACE(&reply->args, &call->data.args);
	ipc_call_free(call);
	if (rc != 0)
		return rc;
	
	return 0;
}

/** Check that the task did not exceed the allowed limit of asynchronous calls
 * made over a phone.
 *
 * @param phone Phone to check the limit against.
 *
 * @return 0 if limit not reached or -1 if limit exceeded.
 *
 */
static int check_call_limit(phone_t *phone)
{
	if (atomic_get(&phone->active_calls) >= IPC_MAX_ASYNC_CALLS)
		return -1;
	
	return 0;
}

/** Make a fast asynchronous call over IPC.
 *
 * This function can only handle four arguments of payload, but is faster than
 * the generic function sys_ipc_call_async_slow().
 *
 * @param phoneid Phone handle for the call.
 * @param imethod Interface and method of the call.
 * @param arg1    Service-defined payload argument.
 * @param arg2    Service-defined payload argument.
 * @param arg3    Service-defined payload argument.
 * @param arg4    Service-defined payload argument.
 *
 * @return Call hash on success.
 * @return IPC_CALLRET_FATAL in case of a fatal error.
 * @return IPC_CALLRET_TEMPORARY if there are too many pending
 *         asynchronous requests; answers should be handled first.
 *
 */
sysarg_t sys_ipc_call_async_fast(sysarg_t phoneid, sysarg_t imethod,
    sysarg_t arg1, sysarg_t arg2, sysarg_t arg3, sysarg_t arg4)
{
	phone_t *phone;
	if (phone_get(phoneid, &phone) != EOK)
		return IPC_CALLRET_FATAL;
	
	if (check_call_limit(phone))
		return IPC_CALLRET_TEMPORARY;
	
	call_t *call = ipc_call_alloc(0);
	IPC_SET_IMETHOD(call->data, imethod);
	IPC_SET_ARG1(call->data, arg1);
	IPC_SET_ARG2(call->data, arg2);
	IPC_SET_ARG3(call->data, arg3);
	IPC_SET_ARG4(call->data, arg4);
	
	/*
	 * To achieve deterministic behavior, zero out arguments that are beyond
	 * the limits of the fast version.
	 */
	IPC_SET_ARG5(call->data, 0);
	
	int res = request_preprocess(call, phone);
	
	if (!res)
		ipc_call(phone, call);
	else
		ipc_backsend_err(phone, call, res);
	
	return (sysarg_t) call;
}

/** Make an asynchronous IPC call allowing to transmit the entire payload.
 *
 * @param phoneid Phone handle for the call.
 * @param data    Userspace address of call data with the request.
 *
 * @return See sys_ipc_call_async_fast().
 *
 */
sysarg_t sys_ipc_call_async_slow(sysarg_t phoneid, ipc_data_t *data)
{
	phone_t *phone;
	if (phone_get(phoneid, &phone) != EOK)
		return IPC_CALLRET_FATAL;

	if (check_call_limit(phone))
		return IPC_CALLRET_TEMPORARY;

	call_t *call = ipc_call_alloc(0);
	int rc = copy_from_uspace(&call->data.args, &data->args,
	    sizeof(call->data.args));
	if (rc != 0) {
		ipc_call_free(call);
		return (sysarg_t) rc;
	}
	
	int res = request_preprocess(call, phone);
	
	if (!res)
		ipc_call(phone, call);
	else
		ipc_backsend_err(phone, call, res);
	
	return (sysarg_t) call;
}

/** Forward a received call to another destination
 *
 * Common code for both the fast and the slow version.
 *
 * @param callid  Hash of the call to forward.
 * @param phoneid Phone handle to use for forwarding.
 * @param imethod New interface and method to use for the forwarded call.
 * @param arg1    New value of the first argument for the forwarded call.
 * @param arg2    New value of the second argument for the forwarded call.
 * @param arg3    New value of the third argument for the forwarded call.
 * @param arg4    New value of the fourth argument for the forwarded call.
 * @param arg5    New value of the fifth argument for the forwarded call.
 * @param mode    Flags that specify mode of the forward operation.
 * @param slow    If true, arg3, arg4 and arg5 are considered. Otherwise
 *                the function considers only the fast version arguments:
 *                i.e. arg1 and arg2.
 *
 * @return 0 on succes, otherwise an error code.
 *
 * Warning: Make sure that ARG5 is not rewritten for certain system IPC
 *
 */
static sysarg_t sys_ipc_forward_common(sysarg_t callid, sysarg_t phoneid,
    sysarg_t imethod, sysarg_t arg1, sysarg_t arg2, sysarg_t arg3,
    sysarg_t arg4, sysarg_t arg5, unsigned int mode, bool slow)
{
	call_t *call = get_call(callid);
	if (!call)
		return ENOENT;
	
	call->flags |= IPC_CALL_FORWARDED;
	
	phone_t *phone;
	if (phone_get(phoneid, &phone) != EOK) {
		IPC_SET_RETVAL(call->data, EFORWARD);
		ipc_answer(&TASK->answerbox, call);
		return ENOENT;
	}
	
	if (!method_is_forwardable(IPC_GET_IMETHOD(call->data))) {
		IPC_SET_RETVAL(call->data, EFORWARD);
		ipc_answer(&TASK->answerbox, call);
		return EPERM;
	}
	
	/*
	 * User space is not allowed to change interface and method of system
	 * methods on forward, allow changing ARG1, ARG2, ARG3 and ARG4 by
	 * means of imethod, arg1, arg2 and arg3.
	 * If the interface and method is immutable, don't change anything.
	 */
	if (!method_is_immutable(IPC_GET_IMETHOD(call->data))) {
		if (method_is_system(IPC_GET_IMETHOD(call->data))) {
			if (IPC_GET_IMETHOD(call->data) == IPC_M_CONNECT_TO_ME)
				phone_dealloc(IPC_GET_ARG5(call->data));
			
			IPC_SET_ARG1(call->data, imethod);
			IPC_SET_ARG2(call->data, arg1);
			IPC_SET_ARG3(call->data, arg2);
			
			if (slow)
				IPC_SET_ARG4(call->data, arg3);
			
			/*
			 * For system methods we deliberately don't
			 * overwrite ARG5.
			 */
		} else {
			IPC_SET_IMETHOD(call->data, imethod);
			IPC_SET_ARG1(call->data, arg1);
			IPC_SET_ARG2(call->data, arg2);
			if (slow) {
				IPC_SET_ARG3(call->data, arg3);
				IPC_SET_ARG4(call->data, arg4);
				IPC_SET_ARG5(call->data, arg5);
			}
		}
	}
	
	return ipc_forward(call, phone, &TASK->answerbox, mode);
}

/** Forward a received call to another destination - fast version.
 *
 * In case the original interface and method is a system method, ARG1, ARG2
 * and ARG3 are overwritten in the forwarded message with the new method and
 * the new arg1 and arg2, respectively. Otherwise the IMETHOD, ARG1 and ARG2
 * are rewritten with the new interface and method, arg1 and arg2, respectively.
 * Also note there is a set of immutable methods, for which the new method and
 * arguments are not set and these values are ignored.
 *
 * @param callid  Hash of the call to forward.
 * @param phoneid Phone handle to use for forwarding.
 * @param imethod New interface and method to use for the forwarded call.
 * @param arg1    New value of the first argument for the forwarded call.
 * @param arg2    New value of the second argument for the forwarded call.
 * @param mode    Flags that specify mode of the forward operation.
 *
 * @return 0 on succes, otherwise an error code.
 *
 */
sysarg_t sys_ipc_forward_fast(sysarg_t callid, sysarg_t phoneid,
    sysarg_t imethod, sysarg_t arg1, sysarg_t arg2, unsigned int mode)
{
	return sys_ipc_forward_common(callid, phoneid, imethod, arg1, arg2, 0, 0,
	    0, mode, false); 
}

/** Forward a received call to another destination - slow version.
 *
 * This function is the slow verision of the sys_ipc_forward_fast interface.
 * It can copy all five new arguments and the new interface and method from
 * the userspace. It naturally extends the functionality of the fast version.
 * For system methods, it additionally stores the new value of arg3 to ARG4.
 * For non-system methods, it additionally stores the new value of arg3, arg4
 * and arg5, respectively, to ARG3, ARG4 and ARG5, respectively.
 *
 * @param callid  Hash of the call to forward.
 * @param phoneid Phone handle to use for forwarding.
 * @param data    Userspace address of the new IPC data.
 * @param mode    Flags that specify mode of the forward operation.
 *
 * @return 0 on succes, otherwise an error code.
 *
 */
sysarg_t sys_ipc_forward_slow(sysarg_t callid, sysarg_t phoneid,
    ipc_data_t *data, unsigned int mode)
{
	ipc_data_t newdata;
	int rc = copy_from_uspace(&newdata.args, &data->args,
	    sizeof(newdata.args));
	if (rc != 0)
		return (sysarg_t) rc;
	
	return sys_ipc_forward_common(callid, phoneid,
	    IPC_GET_IMETHOD(newdata), IPC_GET_ARG1(newdata),
	    IPC_GET_ARG2(newdata), IPC_GET_ARG3(newdata),
	    IPC_GET_ARG4(newdata), IPC_GET_ARG5(newdata), mode, true); 
}

/** Answer an IPC call - fast version.
 *
 * This function can handle only two return arguments of payload, but is faster
 * than the generic sys_ipc_answer().
 *
 * @param callid Hash of the call to be answered.
 * @param retval Return value of the answer.
 * @param arg1   Service-defined return value.
 * @param arg2   Service-defined return value.
 * @param arg3   Service-defined return value.
 * @param arg4   Service-defined return value.
 *
 * @return 0 on success, otherwise an error code.
 *
 */
sysarg_t sys_ipc_answer_fast(sysarg_t callid, sysarg_t retval,
    sysarg_t arg1, sysarg_t arg2, sysarg_t arg3, sysarg_t arg4)
{
	/* Do not answer notification callids */
	if (callid & IPC_CALLID_NOTIFICATION)
		return 0;
	
	call_t *call = get_call(callid);
	if (!call)
		return ENOENT;
	
	ipc_data_t saved_data;
	bool saved;
	
	if (answer_need_old(call)) {
		memcpy(&saved_data, &call->data, sizeof(call->data));
		saved = true;
	} else
		saved = false;
	
	IPC_SET_RETVAL(call->data, retval);
	IPC_SET_ARG1(call->data, arg1);
	IPC_SET_ARG2(call->data, arg2);
	IPC_SET_ARG3(call->data, arg3);
	IPC_SET_ARG4(call->data, arg4);
	
	/*
	 * To achieve deterministic behavior, zero out arguments that are beyond
	 * the limits of the fast version.
	 */
	IPC_SET_ARG5(call->data, 0);
	int rc = answer_preprocess(call, saved ? &saved_data : NULL);
	
	ipc_answer(&TASK->answerbox, call);
	return rc;
}

/** Answer an IPC call.
 *
 * @param callid Hash of the call to be answered.
 * @param data   Userspace address of call data with the answer.
 *
 * @return 0 on success, otherwise an error code.
 *
 */
sysarg_t sys_ipc_answer_slow(sysarg_t callid, ipc_data_t *data)
{
	/* Do not answer notification callids */
	if (callid & IPC_CALLID_NOTIFICATION)
		return 0;
	
	call_t *call = get_call(callid);
	if (!call)
		return ENOENT;
	
	ipc_data_t saved_data;
	bool saved;
	
	if (answer_need_old(call)) {
		memcpy(&saved_data, &call->data, sizeof(call->data));
		saved = true;
	} else
		saved = false;
	
	int rc = copy_from_uspace(&call->data.args, &data->args, 
	    sizeof(call->data.args));
	if (rc != 0)
		return rc;
	
	rc = answer_preprocess(call, saved ? &saved_data : NULL);
	
	ipc_answer(&TASK->answerbox, call);
	return rc;
}

/** Hang up a phone.
 *
 * @param Phone handle of the phone to be hung up.
 *
 * @return 0 on success or an error code.
 *
 */
sysarg_t sys_ipc_hangup(sysarg_t phoneid)
{
	phone_t *phone;
	
	if (phone_get(phoneid, &phone) != EOK)
		return ENOENT;
	
	if (ipc_phone_hangup(phone))
		return -1;
	
	return 0;
}

/** Wait for an incoming IPC call or an answer.
 *
 * @param calldata Pointer to buffer where the call/answer data is stored.
 * @param usec     Timeout. See waitq_sleep_timeout() for explanation.
 * @param flags    Select mode of sleep operation. See waitq_sleep_timeout()
 *                 for explanation.
 *
 * @return Hash of the call.
 *         If IPC_CALLID_NOTIFICATION bit is set in the hash, the
 *         call is a notification. IPC_CALLID_ANSWERED denotes an
 *         answer.
 *
 */
sysarg_t sys_ipc_wait_for_call(ipc_data_t *calldata, uint32_t usec,
    unsigned int flags)
{
	call_t *call;
	
restart:
	
#ifdef CONFIG_UDEBUG
	udebug_stoppable_begin();
#endif
	
	call = ipc_wait_for_call(&TASK->answerbox, usec,
	    flags | SYNCH_FLAGS_INTERRUPTIBLE);
	
#ifdef CONFIG_UDEBUG
	udebug_stoppable_end();
#endif
	
	if (!call)
		return 0;
	
	if (call->flags & IPC_CALL_NOTIF) {
		/* Set in_phone_hash to the interrupt counter */
		call->data.phone = (void *) call->priv;
		
		STRUCT_TO_USPACE(calldata, &call->data);
		
		ipc_call_free(call);
		
		return ((sysarg_t) call) | IPC_CALLID_NOTIFICATION;
	}
	
	if (call->flags & IPC_CALL_ANSWERED) {
		process_answer(call);
		
		if (call->flags & IPC_CALL_DISCARD_ANSWER) {
			ipc_call_free(call);
			goto restart;
		}
		
		STRUCT_TO_USPACE(&calldata->args, &call->data.args);
		ipc_call_free(call);
		
		return ((sysarg_t) call) | IPC_CALLID_ANSWERED;
	}
	
	if (process_request(&TASK->answerbox, call))
		goto restart;
	
	/* Include phone address('id') of the caller in the request,
	 * copy whole call->data, not only call->data.args */
	if (STRUCT_TO_USPACE(calldata, &call->data)) {
		/*
		 * The callee will not receive this call and no one else has
		 * a chance to answer it. Reply with the EPARTY error code.
		 */
		ipc_data_t saved_data;
		bool saved;
		
		if (answer_need_old(call)) {
			memcpy(&saved_data, &call->data, sizeof(call->data));
			saved = true;
		} else
			saved = false;
		
		IPC_SET_RETVAL(call->data, EPARTY);
		(void) answer_preprocess(call, saved ? &saved_data : NULL);
		ipc_answer(&TASK->answerbox, call);
		return 0;
	}
	
	return (sysarg_t) call;
}

/** Interrupt one thread from sys_ipc_wait_for_call().
 *
 */
sysarg_t sys_ipc_poke(void)
{
	waitq_unsleep(&TASK->answerbox.wq);
	return EOK;
}

/** Connect an IRQ handler to a task.
 *
 * @param inr     IRQ number.
 * @param devno   Device number.
 * @param imethod Interface and method to be associated with the notification.
 * @param ucode   Uspace pointer to the top-half pseudocode.
 *
 * @return EPERM or a return code returned by ipc_irq_register().
 *
 */
sysarg_t sys_irq_register(inr_t inr, devno_t devno, sysarg_t imethod,
    irq_code_t *ucode)
{
	if (!(cap_get(TASK) & CAP_IRQ_REG))
		return EPERM;
	
	return ipc_irq_register(&TASK->answerbox, inr, devno, imethod, ucode);
}

/** Disconnect an IRQ handler from a task.
 *
 * @param inr   IRQ number.
 * @param devno Device number.
 *
 * @return Zero on success or EPERM on error.
 *
 */
sysarg_t sys_irq_unregister(inr_t inr, devno_t devno)
{
	if (!(cap_get(TASK) & CAP_IRQ_REG))
		return EPERM;
	
	ipc_irq_unregister(&TASK->answerbox, inr, devno);
	
	return 0;
}

#ifdef __32_BITS__

/** Syscall connect to a task by ID (32 bits)
 *
 * @return Phone id on success, or negative error code.
 *
 */
sysarg_t sys_ipc_connect_kbox(sysarg64_t *uspace_taskid)
{
#ifdef CONFIG_UDEBUG
	sysarg64_t taskid;
	int rc = copy_from_uspace(&taskid, uspace_taskid, sizeof(sysarg64_t));
	if (rc != 0)
		return (sysarg_t) rc;
	
	return ipc_connect_kbox((task_id_t) taskid);
#else
	return (sysarg_t) ENOTSUP;
#endif
}

#endif  /* __32_BITS__ */

#ifdef __64_BITS__

/** Syscall connect to a task by ID (64 bits)
 *
 * @return Phone id on success, or negative error code.
 *
 */
sysarg_t sys_ipc_connect_kbox(sysarg_t taskid)
{
#ifdef CONFIG_UDEBUG
	return ipc_connect_kbox((task_id_t) taskid);
#else
	return (sysarg_t) ENOTSUP;
#endif
}

#endif  /* __64_BITS__ */

/** @}
 */
