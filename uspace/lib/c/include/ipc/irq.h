/*
 * SPDX-FileCopyrightText: 2014 Martin Decky
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup libc
 * @{
 */
/** @file
 */

#ifndef _LIBC_IPC_IRQ_H_
#define _LIBC_IPC_IRQ_H_

#include <types/common.h>
#include <abi/ddi/irq.h>
#include <abi/cap.h>

extern errno_t ipc_irq_subscribe(int, sysarg_t, const irq_code_t *,
    cap_irq_handle_t *);
extern errno_t ipc_irq_unsubscribe(cap_irq_handle_t);

#endif

/** @}
 */
