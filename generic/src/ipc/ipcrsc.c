/*
 * Copyright (C) 2006 Ondrej Palkovsky
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

/* IPC resources management
 *
 * The goal of this source code is to properly manage IPC resources
 * and allow straight and clean clean-up procedure upon task termination.
 *
 * The pattern of usage of the resources is:
 * - allocate empty phone slot, connect | deallocate slot
 * - disconnect connected phone (some messages might be on the fly)
 * - find phone in slot and send a message using phone
 * - answer message to phone
 * - hangup phone (the caller has hung up)
 * - hangup phone (the answerbox is exiting)
 * 
 * Locking strategy
 *
 * - To use a phone, disconnect a phone etc., the phone must be
 *   first locked and then checked that it is connected
 * - To connect an allocated phone it need not be locked (assigning
 *   pointer is atomic on all platforms)
 *
 * - To find an empty phone slot, the TASK must be locked
 * - To answer a message, the answerbox must be locked
 * - The locking of phone and answerbox is done at the ipc_ level.
 *   It is perfectly correct to pass unconnected phone to these functions
 *   and proper reply will be generated.
 *
 * Locking order
 *
 * - first phone, then answerbox
 *   + Easy locking on calls
 *   - Very hard traversing list of phones when disconnecting because
 *     the phones may disconnect during traversal of list of connected phones.
 *     The only possibility is try_lock with restart of list traversal.
 *
 * Destroying is less frequent, this approach is taken.
 *
 * Phone hangup
 * 
 * *** The caller hangs up (sys_ipc_hangup) ***
 * - The phone is disconnected (no more messages can be sent over this phone),
 *   all in-progress messages are correctly handled. The anwerbox receives
 *   IPC_M_PHONE_HUNGUP call from the phone that hung up. When all async
 *   calls are answered, the phone is deallocated.
 *
 * *** The answerbox hangs up (ipc_answer(ESLAM))
 * - The phone is disconnected. IPC_M_ANSWERBOX_HUNGUP notification
 *   is sent to source task, the calling process is expected to
 *   send an sys_ipc_hangup after cleaning up it's internal structures.
 *
 * Cleanup strategy
 * 
 * 1) Disconnect all our phones ('sys_ipc_hangup')
 *
 * 2) Disconnect all phones connected to answerbox.
 *    * Send message 'PHONE_DISCONNECTED' to the target application 
 * - Once all phones are disconnected, no further calls can arrive
 *
 * 3) Answer all messages in 'calls' and 'dispatched_calls' queues with
 *    appropriate error code.
 *
 * 4) Wait for all async answers to arrive
 * 
 */

#include <synch/spinlock.h>
#include <ipc/ipc.h>
#include <arch.h>
#include <proc/task.h>
#include <ipc/ipcrsc.h>
#include <debug.h>

/** Find call_t * in call table according to callid
 *
 * @return NULL on not found, otherwise pointer to call structure
 */
call_t * get_call(__native callid)
{
	/* TODO: Traverse list of dispatched calls and find one */
	/* TODO: locking of call, ripping it from dispatched calls etc.  */
	return (call_t *) callid;
}

/** Allocate new phone slot in current TASK structure */
int phone_alloc(void)
{
	int i;

	spinlock_lock(&TASK->lock);
	
	for (i=0; i < IPC_MAX_PHONES; i++) {
		if (!TASK->phones[i].busy && !atomic_get(&TASK->phones[i].active_calls)) {
			TASK->phones[i].busy = 1;
			break;
		}
	}
	spinlock_unlock(&TASK->lock);

	if (i >= IPC_MAX_PHONES)
		return -1;
	return i;
}

/** Disconnect phone a free the slot
 *
 * All already sent messages will be correctly processed
 */
void phone_dealloc(int phoneid)
{
	spinlock_lock(&TASK->lock);

	ASSERT(TASK->phones[phoneid].busy);
	ASSERT(! TASK->phones[phoneid].callee);

	TASK->phones[phoneid].busy = 0;
	spinlock_unlock(&TASK->lock);
}

/** Connect phone to a given answerbox
 *
 * @param phoneid The slot that will be connected
 *
 * The procedure _enforces_ that the user first marks the phone
 * busy (e.g. via phone_alloc) and then connects the phone, otherwise
 * race condition may appear.
 */
void phone_connect(int phoneid, answerbox_t *box)
{
	phone_t *phone = &TASK->phones[phoneid];
	
	ASSERT(phone->busy);
	ipc_phone_connect(phone, box);
}
