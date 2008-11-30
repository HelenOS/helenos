/*
 * Copyright (c) 2006 Ondrej Palkovsky
 * Copyright (c) 2006 Jakub Jermar
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
/**
 * @file
 * @brief IRQ notification framework.
 *
 * This framework allows applications to register to receive a notification
 * when interrupt is detected. The application may provide a simple 'top-half'
 * handler as part of its registration, which can perform simple operations
 * (read/write port/memory, add information to notification ipc message).
 *
 * The structure of a notification message is as follows:
 * - METHOD: method as registered by the SYS_IPC_REGISTER_IRQ syscall
 * - ARG1: payload modified by a 'top-half' handler
 * - ARG2: payload modified by a 'top-half' handler
 * - ARG3: payload modified by a 'top-half' handler
 * - in_phone_hash: interrupt counter (may be needed to assure correct order
 *         in multithreaded drivers)
 */

#include <arch.h>
#include <mm/slab.h>
#include <errno.h>
#include <ddi/irq.h>
#include <ipc/ipc.h>
#include <ipc/irq.h>
#include <syscall/copy.h>
#include <console/console.h>
#include <print.h>

/** Execute code associated with IRQ notification.
 *
 * @param call		Notification call.
 * @param code		Top-half pseudocode.
 */
static void code_execute(call_t *call, irq_code_t *code)
{
	unsigned int i;
	unative_t dstval = 0;
	
	if (!code)
		return;
	
	for (i = 0; i < code->cmdcount; i++) {
		switch (code->cmds[i].cmd) {
		case CMD_MEM_READ_1:
			dstval = *((uint8_t *) code->cmds[i].addr);
			break;
		case CMD_MEM_READ_2:
			dstval = *((uint16_t *) code->cmds[i].addr);
			break;
		case CMD_MEM_READ_4:
			dstval = *((uint32_t *) code->cmds[i].addr);
			break;
		case CMD_MEM_READ_8:
			dstval = *((uint64_t *) code->cmds[i].addr);
			break;
		case CMD_MEM_WRITE_1:
			*((uint8_t *) code->cmds[i].addr) = code->cmds[i].value;
			break;
		case CMD_MEM_WRITE_2:
			*((uint16_t *) code->cmds[i].addr) =
			    code->cmds[i].value;
			break;
		case CMD_MEM_WRITE_4:
			*((uint32_t *) code->cmds[i].addr) =
			    code->cmds[i].value;
			break;
		case CMD_MEM_WRITE_8:
			*((uint64_t *) code->cmds[i].addr) =
			    code->cmds[i].value;
			break;
#if defined(ia32) || defined(amd64) || defined(ia64)
		case CMD_PORT_READ_1:
			dstval = inb((long) code->cmds[i].addr);
			break;
		case CMD_PORT_WRITE_1:
			outb((long) code->cmds[i].addr, code->cmds[i].value);
			break;
#endif
#if defined(ia64) && defined(SKI)
		case CMD_IA64_GETCHAR:
			dstval = _getc(&ski_uconsole);
			break;
#endif
#if defined(ppc32)
		case CMD_PPC32_GETCHAR:
			dstval = cuda_get_scancode();
			break;
#endif
		default:
			break;
		}
		if (code->cmds[i].dstarg && code->cmds[i].dstarg <
		    IPC_CALL_LEN) {
			call->data.args[code->cmds[i].dstarg] = dstval;
		}
	}
}

/** Free top-half pseudocode.
 *
 * @param code		Pointer to the top-half pseudocode.
 */
static void code_free(irq_code_t *code)
{
	if (code) {
		free(code->cmds);
		free(code);
	}
}

/** Copy top-half pseudocode from userspace into the kernel.
 *
 * @param ucode		Userspace address of the top-half pseudocode.
 *
 * @return		Kernel address of the copied pseudocode.
 */
static irq_code_t *code_from_uspace(irq_code_t *ucode)
{
	irq_code_t *code;
	irq_cmd_t *ucmds;
	int rc;

	code = malloc(sizeof(*code), 0);
	rc = copy_from_uspace(code, ucode, sizeof(*code));
	if (rc != 0) {
		free(code);
		return NULL;
	}
	
	if (code->cmdcount > IRQ_MAX_PROG_SIZE) {
		free(code);
		return NULL;
	}
	ucmds = code->cmds;
	code->cmds = malloc(sizeof(code->cmds[0]) * code->cmdcount, 0);
	rc = copy_from_uspace(code->cmds, ucmds,
	    sizeof(code->cmds[0]) * code->cmdcount);
	if (rc != 0) {
		free(code->cmds);
		free(code);
		return NULL;
	}

	return code;
}

/** Unregister task from IRQ notification.
 *
 * @param box		Answerbox associated with the notification.
 * @param inr		IRQ number.
 * @param devno		Device number.
 */
void ipc_irq_unregister(answerbox_t *box, inr_t inr, devno_t devno)
{
	ipl_t ipl;
	irq_t *irq;

	ipl = interrupts_disable();
	irq = irq_find_and_lock(inr, devno);
	if (irq) {
		if (irq->notif_cfg.answerbox == box) {
			code_free(irq->notif_cfg.code);
			irq->notif_cfg.notify = false;
			irq->notif_cfg.answerbox = NULL;
			irq->notif_cfg.code = NULL;
			irq->notif_cfg.method = 0;
			irq->notif_cfg.counter = 0;

			spinlock_lock(&box->irq_lock);
			list_remove(&irq->notif_cfg.link);
			spinlock_unlock(&box->irq_lock);
			
			spinlock_unlock(&irq->lock);
		}
	}
	interrupts_restore(ipl);
}

/** Register an answerbox as a receiving end for IRQ notifications.
 *
 * @param box		Receiving answerbox.
 * @param inr		IRQ number.
 * @param devno		Device number.
 * @param method	Method to be associated with the notification.
 * @param ucode		Uspace pointer to top-half pseudocode.
 *
 * @return 		EBADMEM, ENOENT or EEXISTS on failure or 0 on success.
 */
int ipc_irq_register(answerbox_t *box, inr_t inr, devno_t devno,
    unative_t method, irq_code_t *ucode)
{
	ipl_t ipl;
	irq_code_t *code;
	irq_t *irq;

	if (ucode) {
		code = code_from_uspace(ucode);
		if (!code)
			return EBADMEM;
	} else {
		code = NULL;
	}

	ipl = interrupts_disable();
	irq = irq_find_and_lock(inr, devno);
	if (!irq) {
		interrupts_restore(ipl);
		code_free(code);
		return ENOENT;
	}
	
	if (irq->notif_cfg.answerbox) {
		spinlock_unlock(&irq->lock);
		interrupts_restore(ipl);
		code_free(code);
		return EEXISTS;
	}
	
	irq->notif_cfg.notify = true;
	irq->notif_cfg.answerbox = box;
	irq->notif_cfg.method = method;
	irq->notif_cfg.code = code;
	irq->notif_cfg.counter = 0;

	spinlock_lock(&box->irq_lock);
	list_append(&irq->notif_cfg.link, &box->irq_head);
	spinlock_unlock(&box->irq_lock);

	spinlock_unlock(&irq->lock);
	interrupts_restore(ipl);

	return 0;
}

/** Add a call to the proper answerbox queue.
 *
 * Assume irq->lock is locked.
 *
 * @param irq		IRQ structure referencing the target answerbox.
 * @param call		IRQ notification call.
 */
static void send_call(irq_t *irq, call_t *call)
{
	spinlock_lock(&irq->notif_cfg.answerbox->irq_lock);
	list_append(&call->link, &irq->notif_cfg.answerbox->irq_notifs);
	spinlock_unlock(&irq->notif_cfg.answerbox->irq_lock);
		
	waitq_wakeup(&irq->notif_cfg.answerbox->wq, WAKEUP_FIRST);
}

/** Send notification message.
 *
 * @param irq		IRQ structure.
 * @param a1		Driver-specific payload argument.
 * @param a2		Driver-specific payload argument.
 * @param a3		Driver-specific payload argument.
 * @param a4		Driver-specific payload argument.
 * @param a5		Driver-specific payload argument.
 */
void ipc_irq_send_msg(irq_t *irq, unative_t a1, unative_t a2, unative_t a3,
    unative_t a4, unative_t a5)
{
	call_t *call;

	spinlock_lock(&irq->lock);

	if (irq->notif_cfg.answerbox) {
		call = ipc_call_alloc(FRAME_ATOMIC);
		if (!call) {
			spinlock_unlock(&irq->lock);
			return;
		}
		call->flags |= IPC_CALL_NOTIF;
		IPC_SET_METHOD(call->data, irq->notif_cfg.method);
		IPC_SET_ARG1(call->data, a1);
		IPC_SET_ARG2(call->data, a2);
		IPC_SET_ARG3(call->data, a3);
		IPC_SET_ARG4(call->data, a4);
		IPC_SET_ARG5(call->data, a5);
		/* Put a counter to the message */
		call->priv = ++irq->notif_cfg.counter;
		
		send_call(irq, call);
	}
	spinlock_unlock(&irq->lock);
}

/** Notify a task that an IRQ had occurred.
 *
 * We expect interrupts to be disabled and the irq->lock already held.
 *
 * @param irq		IRQ structure.
 */
void ipc_irq_send_notif(irq_t *irq)
{
	call_t *call;

	ASSERT(irq);

	if (irq->notif_cfg.answerbox) {
		call = ipc_call_alloc(FRAME_ATOMIC);
		if (!call) {
			return;
		}
		call->flags |= IPC_CALL_NOTIF;
		/* Put a counter to the message */
		call->priv = ++irq->notif_cfg.counter;
		/* Set up args */
		IPC_SET_METHOD(call->data, irq->notif_cfg.method);

		/* Execute code to handle irq */
		code_execute(call, irq->notif_cfg.code);
		
		send_call(irq, call);
	}
}

/** Disconnect all IRQ notifications from an answerbox.
 *
 * This function is effective because the answerbox contains
 * list of all irq_t structures that are registered to
 * send notifications to it.
 *
 * @param box		Answerbox for which we want to carry out the cleanup.
 */
void ipc_irq_cleanup(answerbox_t *box)
{
	ipl_t ipl;
	
loop:
	ipl = interrupts_disable();
	spinlock_lock(&box->irq_lock);
	
	while (box->irq_head.next != &box->irq_head) {
		link_t *cur = box->irq_head.next;
		irq_t *irq;
		DEADLOCK_PROBE_INIT(p_irqlock);
		
		irq = list_get_instance(cur, irq_t, notif_cfg.link);
		if (!spinlock_trylock(&irq->lock)) {
			/*
			 * Avoid deadlock by trying again.
			 */
			spinlock_unlock(&box->irq_lock);
			interrupts_restore(ipl);
			DEADLOCK_PROBE(p_irqlock, DEADLOCK_THRESHOLD);
			goto loop;
		}
		
		ASSERT(irq->notif_cfg.answerbox == box);
		
		list_remove(&irq->notif_cfg.link);
		
		/*
		 * Don't forget to free any top-half pseudocode.
		 */
		code_free(irq->notif_cfg.code);
		
		irq->notif_cfg.notify = false;
		irq->notif_cfg.answerbox = NULL;
		irq->notif_cfg.code = NULL;
		irq->notif_cfg.method = 0;
		irq->notif_cfg.counter = 0;

		spinlock_unlock(&irq->lock);
	}
	
	spinlock_unlock(&box->irq_lock);
	interrupts_restore(ipl);
}

/** @}
 */
