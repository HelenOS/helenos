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

#ifndef KERN_IPC_IRQ_H_
#define KERN_IPC_IRQ_H_

/** Maximum number of IPC IRQ programmed I/O ranges. */
#define IRQ_MAX_RANGE_COUNT  8

/** Maximum length of IPC IRQ program. */
#define IRQ_MAX_PROG_SIZE  256

#include <ipc/ipc.h>
#include <ddi/irq.h>
#include <typedefs.h>
#include <adt/list.h>


extern irq_ownership_t ipc_irq_top_half_claim(irq_t *);
extern void ipc_irq_top_half_handler(irq_t *);

extern errno_t ipc_irq_subscribe(answerbox_t *, inr_t, sysarg_t, irq_code_t *,
    cap_irq_handle_t *);
extern errno_t ipc_irq_unsubscribe(answerbox_t *, cap_irq_handle_t);

/*
 * User friendly wrappers for ipc_irq_send_msg(). They are in the form
 * ipc_irq_send_msg_m(), where m is the number of payload arguments.
 */
#define ipc_irq_send_msg_0(irq) \
	ipc_irq_send_msg((irq), 0, 0, 0, 0, 0)

#define ipc_irq_send_msg_1(irq, a1) \
	ipc_irq_send_msg((irq), (a1), 0, 0, 0, 0)

#define ipc_irq_send_msg_2(irq, a1, a2) \
	ipc_irq_send_msg((irq), (a1), (a2), 0, 0, 0)

#define ipc_irq_send_msg_3(irq, a1, a2, a3) \
	ipc_irq_send_msg((irq), (a1), (a2), (a3), 0, 0)

#define ipc_irq_send_msg_4(irq, a1, a2, a3, a4) \
	ipc_irq_send_msg((irq), (a1), (a2), (a3), (a4), 0)

#define ipc_irq_send_msg_5(irq, a1, a2, a3, a4, a5) \
	ipc_irq_send_msg((irq), (a1), (a2), (a3), (a4), (a5))

extern void ipc_irq_send_msg(irq_t *, sysarg_t, sysarg_t, sysarg_t, sysarg_t,
    sysarg_t);

#endif

/** @}
 */
