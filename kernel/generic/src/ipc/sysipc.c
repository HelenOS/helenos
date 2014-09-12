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
#include <errno.h>
#include <memstr.h>
#include <ipc/ipc.h>
#include <abi/ipc/methods.h>
#include <ipc/sysipc.h>
#include <ipc/sysipc_ops.h>
#include <ipc/sysipc_priv.h>
#include <ipc/irq.h>
#include <ipc/ipcrsc.h>
#include <ipc/event.h>
#include <ipc/kbox.h>
#include <synch/waitq.h>
#include <arch/interrupt.h>
#include <syscall/copy.h>
#include <security/cap.h>
#include <console/console.h>
#include <print.h>
#include <macros.h>

#define STRUCT_TO_USPACE(dst, src)  copy_to_uspace((dst), (src), sizeof(*(src)))

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
 * @return Return EOK on success or a negative error code.
 *
 */
int answer_preprocess(call_t *answer, ipc_data_t *olddata)
{
	int rc = EOK;

	spinlock_lock(&answer->forget_lock);
	if (answer->forget) {
		/*
		 * This is a forgotten call and answer->sender is not valid.
		 */
		spinlock_unlock(&answer->forget_lock);

		SYSIPC_OP(answer_cleanup, answer, olddata);
		return rc;
	} else {
		ASSERT(answer->active);

		/*
		 * Mark the call as inactive to prevent _ipc_answer_free_call()
		 * from attempting to remove the call from the active list
		 * itself.
		 */
		answer->active = false;

		/*
		 * Remove the call from the sender's active call list.
		 * We enforce this locking order so that any potential
		 * concurrently executing forget operation is forced to
		 * release its active_calls_lock and lose the race to
		 * forget this soon to be answered call. 
		 */
		spinlock_lock(&answer->sender->active_calls_lock);
		list_remove(&answer->ta_link);
		spinlock_unlock(&answer->sender->active_calls_lock);
	}
	spinlock_unlock(&answer->forget_lock);

	if ((native_t) IPC_GET_RETVAL(answer->data) == EHANGUP) {
		phone_t *phone = answer->caller_phone;
		mutex_lock(&phone->lock);
		if (phone->state == IPC_PHONE_CONNECTED) {
			irq_spinlock_lock(&phone->callee->lock, true);
			list_remove(&phone->link);
			phone->state = IPC_PHONE_SLAMMED;
			irq_spinlock_unlock(&phone->callee->lock, true);
		}
		mutex_unlock(&phone->lock);
	}
	
	if (!olddata)
		return rc;

	return SYSIPC_OP(answer_preprocess, answer, olddata);
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
	call->request_method = IPC_GET_IMETHOD(call->data);
	return SYSIPC_OP(request_preprocess, call, phone);
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
	
	SYSIPC_OP(answer_process, call);
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
	return SYSIPC_OP(request_process, call, box);
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
	phone_t *phone;
	bool need_old = answer_need_old(call);
	bool after_forward = false;
	ipc_data_t old;
	int rc;

	if (!call)
		return ENOENT;

	if (need_old)
		old = call->data;
	
	if (phone_get(phoneid, &phone) != EOK) {
		rc = ENOENT;
		goto error;
	}
	
	if (!method_is_forwardable(IPC_GET_IMETHOD(call->data))) {
		rc = EPERM;
		goto error;
	}

	call->flags |= IPC_CALL_FORWARDED;
	
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
	
	rc = ipc_forward(call, phone, &TASK->answerbox, mode);
	if (rc != EOK) {
		after_forward = true;
		goto error;
	}

	return EOK;

error:
	IPC_SET_RETVAL(call->data, EFORWARD);
	(void) answer_preprocess(call, need_old ? &old : NULL);
	if (after_forward)
		_ipc_answer_free_call(call, false);
	else
		ipc_answer(&TASK->answerbox, call);

	return rc;
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
 * @return EPERM or a return code returned by ipc_irq_subscribe().
 *
 */
sysarg_t sys_ipc_irq_subscribe(inr_t inr, devno_t devno, sysarg_t imethod,
    irq_code_t *ucode)
{
	if (!(cap_get(TASK) & CAP_IRQ_REG))
		return EPERM;
	
	return ipc_irq_subscribe(&TASK->answerbox, inr, devno, imethod, ucode);
}

/** Disconnect an IRQ handler from a task.
 *
 * @param inr   IRQ number.
 * @param devno Device number.
 *
 * @return Zero on success or EPERM on error.
 *
 */
sysarg_t sys_ipc_irq_unsubscribe(inr_t inr, devno_t devno)
{
	if (!(cap_get(TASK) & CAP_IRQ_REG))
		return EPERM;
	
	ipc_irq_unsubscribe(&TASK->answerbox, inr, devno);
	
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
