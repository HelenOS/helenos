/*
 * Copyright (c) 2014 Martin Decky
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
