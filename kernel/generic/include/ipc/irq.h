/*
 * SPDX-FileCopyrightText: 2006 Ondrej Palkovsky
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup kernel_generic_ipc
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
#include <cap/cap.h>

extern kobject_ops_t irq_kobject_ops;

extern irq_ownership_t ipc_irq_top_half_claim(irq_t *);
extern void ipc_irq_top_half_handler(irq_t *);

extern errno_t ipc_irq_subscribe(answerbox_t *, inr_t, sysarg_t, uspace_ptr_irq_code_t,
    uspace_ptr_cap_irq_handle_t);
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
