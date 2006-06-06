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

/** IRQ notification framework
 *
 * This framework allows applications to register to receive a notification
 * when interrupt is detected. The application may provide a simple 'top-half'
 * handler as part of its registration, which can perform simple operations
 * (read/write port/memory, add information to notification ipc message).
 *
 * The structure of a notification message is as follows:
 * - METHOD: IPC_M_INTERRUPT
 * - ARG1: interrupt number
 * - ARG2: payload modified by a 'top-half' handler
 * - ARG3: interrupt counter (may be needed to assure correct order
 *         in multithreaded drivers)
 */

#include <arch.h>
#include <mm/slab.h>
#include <errno.h>
#include <ipc/ipc.h>
#include <ipc/irq.h>
#include <atomic.h>
#include <syscall/copy.h>
#include <console/console.h>

typedef struct {
	SPINLOCK_DECLARE(lock);
	answerbox_t *box;
	irq_code_t *code;
	atomic_t counter;
} ipc_irq_t;


static ipc_irq_t *irq_conns = NULL;
static int irq_conns_size;

#include <print.h>
/* Execute code associated with IRQ notification */
static void code_execute(call_t *call, irq_code_t *code)
{
	int i;

	if (!code)
		return;
	
	for (i=0; i < code->cmdcount;i++) {
		switch (code->cmds[i].cmd) {
		case CMD_MEM_READ_1:
			IPC_SET_ARG2(call->data, *((__u8 *)code->cmds[i].addr));
			break;
		case CMD_MEM_READ_2:
			IPC_SET_ARG2(call->data, *((__u16 *)code->cmds[i].addr));
			break;
		case CMD_MEM_READ_4:
			IPC_SET_ARG2(call->data, *((__u32 *)code->cmds[i].addr));
			break;
		case CMD_MEM_READ_8:
			IPC_SET_ARG2(call->data, *((__u64 *)code->cmds[i].addr));
			break;
		case CMD_MEM_WRITE_1:
			*((__u8 *)code->cmds[i].addr) = code->cmds[i].value;
			break;
		case CMD_MEM_WRITE_2:
			*((__u16 *)code->cmds[i].addr) = code->cmds[i].value;
			break;
		case CMD_MEM_WRITE_4:
			*((__u32 *)code->cmds[i].addr) = code->cmds[i].value;
			break;
		case CMD_MEM_WRITE_8:
			*((__u64 *)code->cmds[i].addr) = code->cmds[i].value;
			break;
#if defined(ia32) || defined(amd64)
		case CMD_PORT_READ_1:
			IPC_SET_ARG2(call->data, inb((long)code->cmds[i].addr));
			break;
		case CMD_PORT_WRITE_1:
			outb((long)code->cmds[i].addr, code->cmds[i].value);
			break;
#endif
#if defined(ia64) 
		case CMD_IA64_GETCHAR:
			IPC_SET_ARG2(call->data, _getc(&ski_uconsole));
			break;
#endif
#if defined(ppc32) 
		case CMD_PPC32_GETCHAR:
			IPC_SET_ARG2(call->data, _getc(&kbrd));
			break;
#endif
		default:
			break;
		}
	}
}

static void code_free(irq_code_t *code)
{
	if (code) {
		free(code->cmds);
		free(code);
	}
}

static irq_code_t * code_from_uspace(irq_code_t *ucode)
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
	code->cmds = malloc(sizeof(code->cmds[0]) * (code->cmdcount), 0);
	rc = copy_from_uspace(code->cmds, ucmds, sizeof(code->cmds[0]) * (code->cmdcount));
	if (rc != 0) {
		free(code->cmds);
		free(code);
		return NULL;
	}

	return code;
}

/** Unregister task from irq */
void ipc_irq_unregister(answerbox_t *box, int irq)
{
	ipl_t ipl;
	int mq = irq + IPC_IRQ_RESERVED_VIRTUAL;

	ipl = interrupts_disable();
	spinlock_lock(&irq_conns[mq].lock);
	if (irq_conns[mq].box == box) {
		irq_conns[mq].box = NULL;
		code_free(irq_conns[mq].code);
		irq_conns[mq].code = NULL;
	}

	spinlock_unlock(&irq_conns[mq].lock);
	interrupts_restore(ipl);
}

/** Register an answerbox as a receiving end of interrupts notifications */
int ipc_irq_register(answerbox_t *box, int irq, irq_code_t *ucode)
{
	ipl_t ipl;
	irq_code_t *code;
	int mq = irq + IPC_IRQ_RESERVED_VIRTUAL;

	ASSERT(irq_conns);

	if (ucode) {
		code = code_from_uspace(ucode);
		if (!code)
			return EBADMEM;
	} else
		code = NULL;

	ipl = interrupts_disable();
	spinlock_lock(&irq_conns[mq].lock);

	if (irq_conns[mq].box) {
		spinlock_unlock(&irq_conns[mq].lock);
		interrupts_restore(ipl);
		code_free(code);
		return EEXISTS;
	}
	irq_conns[mq].box = box;
	irq_conns[mq].code = code;
	atomic_set(&irq_conns[mq].counter, 0);
	spinlock_unlock(&irq_conns[mq].lock);
	interrupts_restore(ipl);

	return 0;
}

/** Add call to proper answerbox queue
 *
 * Assume irq_conns[mq].lock is locked */
static void send_call(int mq, call_t *call)
{
	spinlock_lock(&irq_conns[mq].box->irq_lock);
	list_append(&call->link, &irq_conns[mq].box->irq_notifs);
	spinlock_unlock(&irq_conns[mq].box->irq_lock);
		
	waitq_wakeup(&irq_conns[mq].box->wq, 0);
}

/** Send notification message
 *
 */
void ipc_irq_send_msg(int irq, __native a2, __native a3)
{
	call_t *call;
	int mq = irq + IPC_IRQ_RESERVED_VIRTUAL;

	spinlock_lock(&irq_conns[mq].lock);

	if (irq_conns[mq].box) {
		call = ipc_call_alloc(FRAME_ATOMIC);
		if (!call) {
			spinlock_unlock(&irq_conns[mq].lock);
			return;
		}
		call->flags |= IPC_CALL_NOTIF;
		IPC_SET_METHOD(call->data, IPC_M_INTERRUPT);
		IPC_SET_ARG1(call->data, irq);
		IPC_SET_ARG2(call->data, a2);
		IPC_SET_ARG3(call->data, a3);
		
		send_call(mq, call);
	}
	spinlock_unlock(&irq_conns[mq].lock);
}

/** Notify process that an irq had happend
 *
 * We expect interrupts to be disabled
 */
void ipc_irq_send_notif(int irq)
{
	call_t *call;
	int mq = irq + IPC_IRQ_RESERVED_VIRTUAL;

	ASSERT(irq_conns);
	spinlock_lock(&irq_conns[mq].lock);

	if (irq_conns[mq].box) {
		call = ipc_call_alloc(FRAME_ATOMIC);
		if (!call) {
			spinlock_unlock(&irq_conns[mq].lock);
			return;
		}
		call->flags |= IPC_CALL_NOTIF;
		IPC_SET_METHOD(call->data, IPC_M_INTERRUPT);
		IPC_SET_ARG1(call->data, irq);
		IPC_SET_ARG3(call->data, atomic_preinc(&irq_conns[mq].counter));

		/* Execute code to handle irq */
		code_execute(call, irq_conns[mq].code);
		
		send_call(mq, call);
	}
		
	spinlock_unlock(&irq_conns[mq].lock);
}


/** Initialize table of interrupt handlers
 *
 * @param irqcount Count of required hardware IRQs to be supported
 */
void ipc_irq_make_table(int irqcount)
{
	int i;

	irqcount +=  IPC_IRQ_RESERVED_VIRTUAL;

	irq_conns_size = irqcount;
	irq_conns = malloc(irqcount * (sizeof(*irq_conns)), 0);
	for (i=0; i < irqcount; i++) {
		spinlock_initialize(&irq_conns[i].lock, "irq_ipc_lock");
		irq_conns[i].box = NULL;
		irq_conns[i].code = NULL;
	}
}

/** Disconnect all irq's notifications
 *
 * TODO: It may be better to do some linked list, so that
 *       we wouldn't need to go through whole array every cleanup
 */
void ipc_irq_cleanup(answerbox_t *box)
{
	int i;
	ipl_t ipl;
	
	for (i=0; i < irq_conns_size; i++) {
		ipl = interrupts_disable();
		spinlock_lock(&irq_conns[i].lock);
		if (irq_conns[i].box == box)
			irq_conns[i].box = NULL;
		spinlock_unlock(&irq_conns[i].lock);
		interrupts_restore(ipl);
	}
}
