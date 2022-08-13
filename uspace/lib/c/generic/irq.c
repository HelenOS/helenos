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

#include <ipc/irq.h>
#include <libc.h>
#include <stdlib.h>
#include <stddef.h>
#include <macros.h>

static irq_cmd_t default_cmds[] = {
	{
		.cmd = CMD_ACCEPT
	}
};

static const irq_code_t default_ucode = {
	0,
	NULL,
	ARRAY_SIZE(default_cmds),
	default_cmds
};

/** Subscribe to IRQ notification.
 *
 * @param inr    IRQ number.
 * @param method Use this method for notifying me.
 * @param ucode  Top-half pseudocode handler.
 *
 * @param[out] out_handle  IRQ capability handle returned by the kernel.
 *
 * @return Error code returned by the kernel.
 *
 */
errno_t ipc_irq_subscribe(int inr, sysarg_t method, const irq_code_t *ucode,
    cap_irq_handle_t *out_handle)
{
	if (ucode == NULL)
		ucode = &default_ucode;

	return (errno_t) __SYSCALL4(SYS_IPC_IRQ_SUBSCRIBE, inr, method,
	    (sysarg_t) ucode, (sysarg_t) out_handle);
}

/** Unsubscribe from IRQ notification.
 *
 * @param cap   IRQ capability handle.
 *
 * @return Value returned by the kernel.
 *
 */
errno_t ipc_irq_unsubscribe(cap_irq_handle_t cap)
{
	return (errno_t) __SYSCALL1(SYS_IPC_IRQ_UNSUBSCRIBE,
	    cap_handle_raw(cap));
}

/** @}
 */
