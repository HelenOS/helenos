/*
 * Copyright (c) 2006 Martin Decky
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
 * @brief	Z8350 keyboard port driver.
 */

#include <ipc/ipc.h>
#include <async.h>
#include <sysinfo.h>
#include <kbd.h>
#include <kbd_port.h>
#include <sys/types.h>

/** Top-half pseudocode for z8530. */
irq_cmd_t z8530_cmds[] = {
	{
		CMD_MEM_READ_1,
		0,		/**< Address. Will be patched in run-time. */
		0,		/**< Value. Not used. */
		1		/**< Arg 1 will contain the result. */
	}
};

	
irq_code_t z8530_kbd = {
	1,
	z8530_cmds
};

static void z8530_irq_handler(ipc_callid_t iid, ipc_call_t *call);

int kbd_port_init(void)
{
	async_set_interrupt_received(z8350_irq_handler);
	z8530_cmds[0].addr = (void *) sysinfo_value("kbd.address.virtual") + 6;
	ipc_register_irq(sysinfo_value("kbd.inr"), sysinfo_value("kbd.devno"),
	    0, &z8530_kbd);
	return 0;
}

static void z8530_irq_handler(ipc_callid_t iid, ipc_call_t *call)
{
	int scan_code = IPC_GET_ARG1(*call);
	kbd_push_scancode(scan_code);
}

/** @}
 */
