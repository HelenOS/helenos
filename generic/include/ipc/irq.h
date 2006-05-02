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

#ifndef __IRQ_H__
#define __IRQ_H__

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
	CMD_LAST
} irq_cmd_type;

typedef struct {
	irq_cmd_type cmd;
	void *addr;
	unsigned long long value; 
} irq_cmd_t;

typedef struct {
	unsigned int cmdcount;
	irq_cmd_t *cmds;
} irq_code_t;

#ifdef KERNEL

extern void ipc_irq_make_table(int irqcount);
extern int ipc_irq_register(answerbox_t *box, int irq, irq_code_t *ucode);
extern void ipc_irq_send_notif(int irq);
extern void ipc_irq_unregister(answerbox_t *box, int irq);
extern void irq_ipc_bind_arch(__native irq);
extern void ipc_irq_cleanup(answerbox_t *box);

#endif

#endif
