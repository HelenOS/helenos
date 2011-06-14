/*
 * Copyright (c) 2006 Martin Decky
 * Copyright (c) 2011 Jiri Svoboda
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
 * @brief Z8530 keyboard port driver.
 */

#include <ipc/irc.h>
#include <async.h>
#include <async_obsolete.h>
#include <sysinfo.h>
#include <input.h>
#include <kbd.h>
#include <kbd_port.h>
#include <sys/types.h>
#include <ddi.h>
#include <errno.h>

static int z8530_port_init(kbd_dev_t *);
static void z8530_port_yield(void);
static void z8530_port_reclaim(void);
static void z8530_port_write(uint8_t data);

kbd_port_ops_t z8530_port = {
	.init = z8530_port_init,
	.yield = z8530_port_yield,
	.reclaim = z8530_port_reclaim,
	.write = z8530_port_write
};

static kbd_dev_t *kbd_dev;

#define CHAN_A_STATUS  4
#define CHAN_A_DATA    6

#define RR0_RCA  1

static irq_cmd_t z8530_cmds[] = {
	{
		.cmd = CMD_PIO_READ_8,
		.addr = (void *) 0,     /* Will be patched in run-time */
		.dstarg = 1
	},
	{
		.cmd = CMD_BTEST,
		.value = RR0_RCA,
		.srcarg = 1,
		.dstarg = 3
	},
	{
		.cmd = CMD_PREDICATE,
		.value = 2,
		.srcarg = 3
	},
	{
		.cmd = CMD_PIO_READ_8,
		.addr = (void *) 0,     /* Will be patched in run-time */
		.dstarg = 2
	},
	{
		.cmd = CMD_ACCEPT
	}
};

static irq_code_t z8530_kbd = {
	sizeof(z8530_cmds) / sizeof(irq_cmd_t),
	z8530_cmds
};

static void z8530_irq_handler(ipc_callid_t iid, ipc_call_t *call);

static int z8530_port_init(kbd_dev_t *kdev)
{
	kbd_dev = kdev;
	
	sysarg_t z8530;
	if (sysinfo_get_value("kbd.type.z8530", &z8530) != EOK)
		return -1;
	if (!z8530)
		return -1;
	
	sysarg_t kaddr;
	if (sysinfo_get_value("kbd.address.kernel", &kaddr) != EOK)
		return -1;
	
	sysarg_t inr;
	if (sysinfo_get_value("kbd.inr", &inr) != EOK)
		return -1;
	
	z8530_cmds[0].addr = (void *) kaddr + CHAN_A_STATUS;
	z8530_cmds[3].addr = (void *) kaddr + CHAN_A_DATA;
	
	async_set_interrupt_received(z8530_irq_handler);
	register_irq(inr, device_assign_devno(), inr, &z8530_kbd);
	
	return 0;
}

static void z8530_port_yield(void)
{
}

static void z8530_port_reclaim(void)
{
}

static void z8530_port_write(uint8_t data)
{
	(void) data;
}

static void z8530_irq_handler(ipc_callid_t iid, ipc_call_t *call)
{
	int scan_code = IPC_GET_ARG2(*call);
	kbd_push_scancode(kbd_dev, scan_code);
	
	if (irc_service)
		async_obsolete_msg_1(irc_phone, IRC_CLEAR_INTERRUPT,
		    IPC_GET_IMETHOD(*call));
}

/** @}
 */
