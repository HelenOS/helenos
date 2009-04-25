/*
 * Copyright (c) 2006 Josef Cejka
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

/** @addtogroup kbd_port
 * @ingroup  kbd
 * @{
 */ 
/** @file
 * @brief	Msim keyboard port driver.
 */

#include <ipc/ipc.h>
#include <async.h>
#include <sysinfo.h>
#include <kbd_port.h>
#include <kbd.h>
#include <ddi.h>

irq_cmd_t msim_cmds[] = {
	{
		.cmd = CMD_PIO_READ_8,
		.addr = (void *) 0,	/* will be patched in run-time */
		.dstarg = 2
	},
	{
		.cmd = CMD_ACCEPT
	}
	
};

irq_code_t msim_kbd = {
	sizeof(msim_cmds) / sizeof(irq_cmd_t),
	msim_cmds
};

static void msim_irq_handler(ipc_callid_t iid, ipc_call_t *call);

int kbd_port_init(void)
{
	async_set_interrupt_received(msim_irq_handler);
	msim_cmds[0].addr = sysinfo_value("kbd.address.virtual");
	ipc_register_irq(sysinfo_value("kbd.inr"), device_assign_devno(),
	    0, &msim_kbd);
	return 0;
}

void kbd_port_yield(void)
{
}

void kbd_port_reclaim(void)
{
}

static void msim_irq_handler(ipc_callid_t iid, ipc_call_t *call)
{
	int scan_code = IPC_GET_ARG2(*call);
	kbd_push_scancode(scan_code);
}

/** @}
*/
