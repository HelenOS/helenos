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
 * - There may be objection that a race may occur when the syscall finds
 *   an appropriate call and before executing ipc_send, the phone call might
 *   be disconnected and connected elsewhere. As there is no easy solution,
 *   the application will be notified by an  'PHONE_DISCONNECTED' message
 *   and the phone will not be allocated before the application notifies
 *   the kernel subsystem that it does not have any pending calls regarding
 *   this phone call.
 *
 * Locking order
 *
 * There are 2 possibilities
 * - first phone, then answerbox
 *   + Easy locking on calls
 *   - Very hard traversing list of phones when disconnecting because
 *     the phones may disconnect during traversal of list of connected phones.
 *     The only possibility is try_lock with restart of list traversal.
 *
 * - first answerbox, then phone(s)
 *   + Easy phone disconnect
 *   - Multiple checks needed when sending message
 *
 * Because the answerbox destroyal is much less frequent operation, 
 * the first method is chosen.
 *
 * Cleanup strategy
 * 
 * 1) Disconnect all phones.
 *    * Send message 'PHONE_DISCONNECTED' to the target application 
 * - Once all phones are disconnected, no further calls can arrive
 *
 * 2) Answer all messages in 'calls' and 'dispatched_calls' queues with
 *    appropriate error code.
 *
 * 3) Wait for all async answers to arrive
 * Alternatively - we might try to invalidate all messages by setting some
 * flag, that would dispose of the message once it is answered. This
 * would need to link all calls in one big list, which we don't currently
 * do.
 * 
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
		if (!TASK->phones[i].busy) {
			TASK->phones[i].busy = 1;
			break;
		}
	}
	spinlock_unlock(&TASK->lock);

	if (i >= IPC_MAX_PHONES)
		return -1;
	return i;
}

/** Disconnect phone */
void phone_dealloc(int phoneid)
{
	spinlock_lock(&TASK->lock);

	ASSERT(TASK->phones[phoneid].busy);

	if (TASK->phones[phoneid].callee)
		ipc_phone_destroy(&TASK->phones[phoneid]);

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
