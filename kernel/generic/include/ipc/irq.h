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

/** @addtogroup genericipc
 * @{
 */
/** @file
 */

#ifndef KERN_IPC_IRQ_H_
#define KERN_IPC_IRQ_H_

/** Maximum length of IPC IRQ program */
#define IRQ_MAX_PROG_SIZE 10

typedef enum {
	CMD_MEM_READ_1 = 0,
	CMD_MEM_READ_2,
	CMD_MEM_READ_4,
	CMD_MEM_READ_8,
	CMD_MEM_WRITE_1,
	CMD_MEM_WRITE_2,
	CMD_MEM_WRITE_4,
	CMD_MEM_WRITE_8,
	CMD_PORT_READ_1,
	CMD_PORT_WRITE_1,
	CMD_IA64_GETCHAR,
	CMD_PPC32_GETCHAR,
	CMD_LAST
} irq_cmd_type;

typedef struct {
	irq_cmd_type cmd;
	void *addr;
	unsigned long long value; 
	int dstarg;
} irq_cmd_t;

typedef struct {
	unsigned int cmdcount;
	irq_cmd_t *cmds;
} irq_code_t;

#ifdef KERNEL

#include <ipc/ipc.h>
#include <typedefs.h>
#include <arch/types.h>
#include <adt/list.h>

/** IPC notification config structure.
 *
 * Primarily, this structure is encapsulated in the irq_t structure.
 * It is protected by irq_t::lock.
 */
struct ipc_notif_cfg {
	bool notify;			/**< When false, notifications are not sent. */
	answerbox_t *answerbox;		/**< Answerbox for notifications. */
	unative_t method;		/**< Method to be used for the notification. */
	irq_code_t *code;		/**< Top-half pseudocode. */
	count_t counter;		/**< Counter. */
	link_t link;			/**< Link between IRQs that are notifying the
					     same answerbox. The list is protected by
					     the answerbox irq_lock. */
};

extern int ipc_irq_register(answerbox_t *box, inr_t inr, devno_t devno, unative_t method,
	irq_code_t *ucode);
extern void ipc_irq_send_notif(irq_t *irq);
extern void ipc_irq_send_msg(irq_t *irq, unative_t a1, unative_t a2, unative_t a3);
extern void ipc_irq_unregister(answerbox_t *box, inr_t inr, devno_t devno);
extern void ipc_irq_cleanup(answerbox_t *box);

#endif

#endif

/** @}
 */
