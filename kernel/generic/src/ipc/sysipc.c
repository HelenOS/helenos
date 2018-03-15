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
#include <assert.h>
#include <errno.h>
#include <mem.h>
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
#include <security/perm.h>
#include <console/console.h>
#include <print.h>
#include <macros.h>
#include <cap/cap.h>

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
	case IPC_M_PAGE_IN:
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
	case IPC_M_CONNECT_TO_ME:
	case IPC_M_CONNECT_ME_TO:
	case IPC_M_PAGE_IN:
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
 * @return Return EOK on success or an error code.
 *
 */
errno_t answer_preprocess(call_t *answer, ipc_data_t *olddata)
{
	errno_t rc = EOK;

	spinlock_lock(&answer->forget_lock);
	if (answer->forget) {
		/*
		 * This is a forgotten call and answer->sender is not valid.
		 */
		spinlock_unlock(&answer->forget_lock);

		SYSIPC_OP(answer_cleanup, answer, olddata);
		return rc;
	} else {
		assert(answer->active);

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

	if ((errno_t) IPC_GET_RETVAL(answer->data) == EHANGUP) {
		phone_t *phone = answer->caller_phone;
		mutex_lock(&phone->lock);
		if (phone->state == IPC_PHONE_CONNECTED) {
			irq_spinlock_lock(&phone->callee->lock, true);
			list_remove(&phone->link);
			/* Drop callee->connected_phones reference */
			kobject_put(phone->kobject);
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
static errno_t request_preprocess(call_t *call, phone_t *phone)
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
	if (((errno_t) IPC_GET_RETVAL(call->data) == EHANGUP) &&
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

/** Make a call over IPC and wait for reply.
 *
 * @param handle       Phone capability handle for the call.
 * @param data[inout]  Structure with request/reply data.
 * @param priv         Value to be stored in call->priv.
 *
 * @return EOK on success.
 * @return ENOENT if there is no such phone handle.
 *
 */
errno_t ipc_req_internal(cap_handle_t handle, ipc_data_t *data, sysarg_t priv)
{
	kobject_t *kobj = kobject_get(TASK, handle, KOBJECT_TYPE_PHONE);
	if (!kobj->phone)
		return ENOENT;

	call_t *call = ipc_call_alloc(0);
	call->priv = priv;
	memcpy(call->data.args, data->args, sizeof(data->args));

	errno_t rc = request_preprocess(call, kobj->phone);
	if (!rc) {
#ifdef CONFIG_UDEBUG
		udebug_stoppable_begin();
#endif

		kobject_add_ref(call->kobject);
		rc = ipc_call_sync(kobj->phone, call);
		spinlock_lock(&call->forget_lock);
		bool forgotten = call->forget;
		spinlock_unlock(&call->forget_lock);
		kobject_put(call->kobject);

#ifdef CONFIG_UDEBUG
		udebug_stoppable_end();
#endif

		if (rc != EOK) {
			if (!forgotten) {
				/*
				 * There was an error, but it did not result
				 * in the call being forgotten. In fact, the
				 * call was not even sent. We are still
				 * its owners and are responsible for its
				 * deallocation.
				 */
				kobject_put(call->kobject);
			} else {
				/*
				 * The call was forgotten and it changed hands.
				 * We are no longer expected to free it.
				 */
				assert(rc == EINTR);
			}
			kobject_put(kobj);
			return rc;
		}

		process_answer(call);
	} else
		IPC_SET_RETVAL(call->data, rc);

	memcpy(data->args, call->data.args, sizeof(data->args));
	kobject_put(call->kobject);
	kobject_put(kobj);

	return EOK;
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
 * This function can only handle three arguments of payload, but is faster than
 * the generic function sys_ipc_call_async_slow().
 *
 * @param handle   Phone capability handle for the call.
 * @param imethod  Interface and method of the call.
 * @param arg1     Service-defined payload argument.
 * @param arg2     Service-defined payload argument.
 * @param arg3     Service-defined payload argument.
 * @param label    User-defined label.
 *
 * @return EOK on success.
 * @return An error code on error.
 *
 */
sys_errno_t sys_ipc_call_async_fast(sysarg_t handle, sysarg_t imethod,
    sysarg_t arg1, sysarg_t arg2, sysarg_t arg3, sysarg_t label)
{
	kobject_t *kobj = kobject_get(TASK, handle, KOBJECT_TYPE_PHONE);
	if (!kobj)
		return ENOENT;

	if (check_call_limit(kobj->phone)) {
		kobject_put(kobj);
		return ELIMIT;
	}

	call_t *call = ipc_call_alloc(0);
	IPC_SET_IMETHOD(call->data, imethod);
	IPC_SET_ARG1(call->data, arg1);
	IPC_SET_ARG2(call->data, arg2);
	IPC_SET_ARG3(call->data, arg3);

	/*
	 * To achieve deterministic behavior, zero out arguments that are beyond
	 * the limits of the fast version.
	 */
	IPC_SET_ARG5(call->data, 0);

	/* Set the user-defined label */
	call->data.label = label;

	errno_t res = request_preprocess(call, kobj->phone);

	if (!res)
		ipc_call(kobj->phone, call);
	else
		ipc_backsend_err(kobj->phone, call, res);

	kobject_put(kobj);
	return EOK;
}

/** Make an asynchronous IPC call allowing to transmit the entire payload.
 *
 * @param handle  Phone capability for the call.
 * @param data    Userspace address of call data with the request.
 * @param label   User-defined label.
 *
 * @return See sys_ipc_call_async_fast().
 *
 */
sys_errno_t sys_ipc_call_async_slow(sysarg_t handle, ipc_data_t *data,
    sysarg_t label)
{
	kobject_t *kobj = kobject_get(TASK, handle, KOBJECT_TYPE_PHONE);
	if (!kobj)
		return ENOENT;

	if (check_call_limit(kobj->phone)) {
		kobject_put(kobj);
		return ELIMIT;
	}

	call_t *call = ipc_call_alloc(0);
	errno_t rc = copy_from_uspace(&call->data.args, &data->args,
	    sizeof(call->data.args));
	if (rc != EOK) {
		kobject_put(call->kobject);
		kobject_put(kobj);
		return (sys_errno_t) rc;
	}

	/* Set the user-defined label */
	call->data.label = label;

	errno_t res = request_preprocess(call, kobj->phone);

	if (!res)
		ipc_call(kobj->phone, call);
	else
		ipc_backsend_err(kobj->phone, call, res);

	kobject_put(kobj);
	return EOK;
}

/** Forward a received call to another destination
 *
 * Common code for both the fast and the slow version.
 *
 * @param chandle  Call handle of the forwarded call.
 * @param phandle  Phone handle to use for forwarding.
 * @param imethod  New interface and method to use for the forwarded call.
 * @param arg1     New value of the first argument for the forwarded call.
 * @param arg2     New value of the second argument for the forwarded call.
 * @param arg3     New value of the third argument for the forwarded call.
 * @param arg4     New value of the fourth argument for the forwarded call.
 * @param arg5     New value of the fifth argument for the forwarded call.
 * @param mode     Flags that specify mode of the forward operation.
 * @param slow     If true, arg3, arg4 and arg5 are considered. Otherwise
 *                 the function considers only the fast version arguments:
 *                 i.e. arg1 and arg2.
 *
 * @return 0 on succes, otherwise an error code.
 *
 * Warning: Make sure that ARG5 is not rewritten for certain system IPC
 *
 */
static sys_errno_t sys_ipc_forward_common(sysarg_t chandle, sysarg_t phandle,
    sysarg_t imethod, sysarg_t arg1, sysarg_t arg2, sysarg_t arg3,
    sysarg_t arg4, sysarg_t arg5, unsigned int mode, bool slow)
{
	kobject_t *ckobj = cap_unpublish(TASK, chandle, KOBJECT_TYPE_CALL);
	if (!ckobj)
		return ENOENT;

	call_t *call = ckobj->call;

	ipc_data_t old;
	bool need_old = answer_need_old(call);
	if (need_old)
		old = call->data;

	bool after_forward = false;
	errno_t rc;

	kobject_t *pkobj = kobject_get(TASK, phandle, KOBJECT_TYPE_PHONE);
	if (!pkobj) {
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

	rc = ipc_forward(call, pkobj->phone, &TASK->answerbox, mode);
	if (rc != EOK) {
		after_forward = true;
		goto error;
	}

	cap_free(TASK, chandle);
	kobject_put(ckobj);
	kobject_put(pkobj);
	return EOK;

error:
	IPC_SET_RETVAL(call->data, EFORWARD);
	(void) answer_preprocess(call, need_old ? &old : NULL);
	if (after_forward)
		_ipc_answer_free_call(call, false);
	else
		ipc_answer(&TASK->answerbox, call);

	/* Republish the capability so that the call does not get lost. */
	cap_publish(TASK, chandle, ckobj);

	if (pkobj)
		kobject_put(pkobj);
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
 * @param chandle  Call handle of the call to forward.
 * @param phandle  Phone handle to use for forwarding.
 * @param imethod  New interface and method to use for the forwarded call.
 * @param arg1     New value of the first argument for the forwarded call.
 * @param arg2     New value of the second argument for the forwarded call.
 * @param mode     Flags that specify mode of the forward operation.
 *
 * @return 0 on succes, otherwise an error code.
 *
 */
sys_errno_t sys_ipc_forward_fast(sysarg_t chandle, sysarg_t phandle,
    sysarg_t imethod, sysarg_t arg1, sysarg_t arg2, unsigned int mode)
{
	return sys_ipc_forward_common(chandle, phandle, imethod, arg1, arg2, 0,
	    0, 0, mode, false);
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
 * @param chandle  Call handle of the call to forward.
 * @param phandle  Phone handle to use for forwarding.
 * @param data     Userspace address of the new IPC data.
 * @param mode     Flags that specify mode of the forward operation.
 *
 * @return 0 on succes, otherwise an error code.
 *
 */
sys_errno_t sys_ipc_forward_slow(sysarg_t chandle, sysarg_t phandle,
    ipc_data_t *data, unsigned int mode)
{
	ipc_data_t newdata;
	errno_t rc = copy_from_uspace(&newdata.args, &data->args,
	    sizeof(newdata.args));
	if (rc != EOK)
		return (sys_errno_t) rc;

	return sys_ipc_forward_common(chandle, phandle,
	    IPC_GET_IMETHOD(newdata), IPC_GET_ARG1(newdata),
	    IPC_GET_ARG2(newdata), IPC_GET_ARG3(newdata),
	    IPC_GET_ARG4(newdata), IPC_GET_ARG5(newdata), mode, true);
}

/** Answer an IPC call - fast version.
 *
 * This function can handle only two return arguments of payload, but is faster
 * than the generic sys_ipc_answer().
 *
 * @param chandle  Call handle to be answered.
 * @param retval   Return value of the answer.
 * @param arg1     Service-defined return value.
 * @param arg2     Service-defined return value.
 * @param arg3     Service-defined return value.
 * @param arg4     Service-defined return value.
 *
 * @return 0 on success, otherwise an error code.
 *
 */
sys_errno_t sys_ipc_answer_fast(sysarg_t chandle, sysarg_t retval, sysarg_t arg1,
    sysarg_t arg2, sysarg_t arg3, sysarg_t arg4)
{
	kobject_t *kobj = cap_unpublish(TASK, chandle, KOBJECT_TYPE_CALL);
	if (!kobj)
		return ENOENT;

	call_t *call = kobj->call;

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
	errno_t rc = answer_preprocess(call, saved ? &saved_data : NULL);

	ipc_answer(&TASK->answerbox, call);

	kobject_put(kobj);
	cap_free(TASK, chandle);

	return rc;
}

/** Answer an IPC call.
 *
 * @param chandle Call handle to be answered.
 * @param data    Userspace address of call data with the answer.
 *
 * @return 0 on success, otherwise an error code.
 *
 */
sys_errno_t sys_ipc_answer_slow(sysarg_t chandle, ipc_data_t *data)
{
	kobject_t *kobj = cap_unpublish(TASK, chandle, KOBJECT_TYPE_CALL);
	if (!kobj)
		return ENOENT;

	call_t *call = kobj->call;

	ipc_data_t saved_data;
	bool saved;

	if (answer_need_old(call)) {
		memcpy(&saved_data, &call->data, sizeof(call->data));
		saved = true;
	} else
		saved = false;

	errno_t rc = copy_from_uspace(&call->data.args, &data->args,
	    sizeof(call->data.args));
	if (rc != EOK) {
		/*
		 * Republish the capability so that the call does not get lost.
		 */
		cap_publish(TASK, chandle, kobj);
		return rc;
	}

	rc = answer_preprocess(call, saved ? &saved_data : NULL);

	ipc_answer(&TASK->answerbox, call);

	kobject_put(kobj);
	cap_free(TASK, chandle);

	return rc;
}

/** Hang up a phone.
 *
 * @param handle  Phone capability handle of the phone to be hung up.
 *
 * @return 0 on success or an error code.
 *
 */
sys_errno_t sys_ipc_hangup(sysarg_t handle)
{
	kobject_t *kobj = cap_unpublish(TASK, handle, KOBJECT_TYPE_PHONE);
	if (!kobj)
		return ENOENT;

	errno_t rc = ipc_phone_hangup(kobj->phone);
	kobject_put(kobj);
	cap_free(TASK, handle);
	return rc;
}

/** Wait for an incoming IPC call or an answer.
 *
 * @param calldata Pointer to buffer where the call/answer data is stored.
 * @param usec     Timeout. See waitq_sleep_timeout() for explanation.
 * @param flags    Select mode of sleep operation. See waitq_sleep_timeout()
 *                 for explanation.
 *
 * @return An error code on error.
 */
sys_errno_t sys_ipc_wait_for_call(ipc_data_t *calldata, uint32_t usec,
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

	if (!call) {
		ipc_data_t data = {};
		data.cap_handle = CAP_NIL;
		STRUCT_TO_USPACE(calldata, &data);
		return EOK;
	}

	call->data.flags = call->flags;
	if (call->flags & IPC_CALL_NOTIF) {
		/* Set in_phone_hash to the interrupt counter */
		call->data.phone = (void *) call->priv;

		call->data.cap_handle = CAP_NIL;

		STRUCT_TO_USPACE(calldata, &call->data);
		kobject_put(call->kobject);

		return EOK;
	}

	if (call->flags & IPC_CALL_ANSWERED) {
		process_answer(call);

		if (call->flags & IPC_CALL_DISCARD_ANSWER) {
			kobject_put(call->kobject);
			goto restart;
		}

		call->data.cap_handle = CAP_NIL;

		STRUCT_TO_USPACE(calldata, &call->data);
		kobject_put(call->kobject);

		return EOK;
	}

	if (process_request(&TASK->answerbox, call))
		goto restart;

	cap_handle_t handle;
	errno_t rc = cap_alloc(TASK, &handle);
	if (rc != EOK) {
		goto error;
	}

	call->data.cap_handle = handle;

	/*
	 * Include phone hash of the caller in the request, copy the whole
	 * call->data, not only call->data.args.
	 */
	rc = STRUCT_TO_USPACE(calldata, &call->data);
	if (rc != EOK)
		goto error;

	kobject_add_ref(call->kobject);
	cap_publish(TASK, handle, call->kobject);
	return EOK;

error:
	if (handle >= 0)
		cap_free(TASK, handle);

	/*
	 * The callee will not receive this call and no one else has a chance to
	 * answer it. Set the IPC_CALL_AUTO_REPLY flag and return the EPARTY
	 * error code.
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
	call->flags |= IPC_CALL_AUTO_REPLY;
	ipc_answer(&TASK->answerbox, call);

	return rc;
}

/** Interrupt one thread from sys_ipc_wait_for_call().
 *
 */
sys_errno_t sys_ipc_poke(void)
{
	waitq_unsleep(&TASK->answerbox.wq);
	return EOK;
}

/** Connect an IRQ handler to a task.
 *
 * @param inr     IRQ number.
 * @param imethod Interface and method to be associated with the notification.
 * @param ucode   Uspace pointer to the top-half pseudocode.
 *
 * @param[out] uspace_handle  Uspace pointer to IRQ kernel object capability
 *
 * @return EPERM
 * @return Error code returned by ipc_irq_subscribe().
 *
 */
sys_errno_t sys_ipc_irq_subscribe(inr_t inr, sysarg_t imethod, irq_code_t *ucode,
	cap_handle_t *uspace_handle)
{
	if (!(perm_get(TASK) & PERM_IRQ_REG))
		return EPERM;

	return ipc_irq_subscribe(&TASK->answerbox, inr, imethod, ucode, uspace_handle);
}

/** Disconnect an IRQ handler from a task.
 *
 * @param inr   IRQ number.
 * @param devno Device number.
 *
 * @return Zero on success or EPERM on error.
 *
 */
sys_errno_t sys_ipc_irq_unsubscribe(sysarg_t cap)
{
	if (!(perm_get(TASK) & PERM_IRQ_REG))
		return EPERM;

	ipc_irq_unsubscribe(&TASK->answerbox, cap);

	return 0;
}

/** Syscall connect to a task by ID
 *
 * @return Error code.
 *
 */
sys_errno_t sys_ipc_connect_kbox(task_id_t *uspace_taskid, cap_handle_t *uspace_phone)
{
#ifdef CONFIG_UDEBUG
	task_id_t taskid;
	cap_handle_t phone;

	errno_t rc = copy_from_uspace(&taskid, uspace_taskid, sizeof(task_id_t));
	if (rc == EOK) {
		rc = ipc_connect_kbox((task_id_t) taskid, &phone);
	}

	if (rc == EOK) {
		rc = copy_to_uspace(uspace_phone, &phone, sizeof(cap_handle_t));
		if (rc != EOK) {
			// Clean up the phone on failure.
			sys_ipc_hangup(phone);
		}
	}

	return (sys_errno_t) rc;
#else
	return (sys_errno_t) ENOTSUP;
#endif
}

/** @}
 */
