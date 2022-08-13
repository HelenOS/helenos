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

#ifndef KERN_SYSIPC_H_
#define KERN_SYSIPC_H_

#include <ipc/ipc.h>
#include <ipc/irq.h>
#include <typedefs.h>

extern errno_t ipc_req_internal(cap_phone_handle_t, ipc_data_t *, sysarg_t);

extern sys_errno_t sys_ipc_call_async_fast(cap_phone_handle_t, sysarg_t,
    sysarg_t, sysarg_t, sysarg_t, sysarg_t);
extern sys_errno_t sys_ipc_call_async_slow(cap_phone_handle_t, uspace_ptr_ipc_data_t,
    sysarg_t);
extern sys_errno_t sys_ipc_answer_fast(cap_call_handle_t, sysarg_t, sysarg_t,
    sysarg_t, sysarg_t, sysarg_t);
extern sys_errno_t sys_ipc_answer_slow(cap_call_handle_t, uspace_ptr_ipc_data_t);
extern sys_errno_t sys_ipc_wait_for_call(uspace_ptr_ipc_data_t, uint32_t, unsigned int);
extern sys_errno_t sys_ipc_poke(void);
extern sys_errno_t sys_ipc_forward_fast(cap_call_handle_t, cap_phone_handle_t,
    sysarg_t, sysarg_t, sysarg_t, unsigned int);
extern sys_errno_t sys_ipc_forward_slow(cap_call_handle_t, cap_phone_handle_t,
    uspace_ptr_ipc_data_t, unsigned int);
extern sys_errno_t sys_ipc_hangup(cap_phone_handle_t);

extern sys_errno_t sys_ipc_irq_subscribe(inr_t, sysarg_t, uspace_ptr_irq_code_t,
    uspace_ptr_cap_irq_handle_t);
extern sys_errno_t sys_ipc_irq_unsubscribe(cap_irq_handle_t);

extern sys_errno_t sys_ipc_connect_kbox(uspace_ptr_task_id_t, uspace_ptr_cap_phone_handle_t);

#endif

/** @}
 */
