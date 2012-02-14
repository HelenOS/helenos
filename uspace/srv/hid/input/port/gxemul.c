/*
 * Copyright (c) 2007 Michal Kebrt
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
 * @{
 * @ingroup  kbd
 */ 
/** @file
 * @brief	GXEmul keyboard port driver.
 */

#include <async.h>
#include <sysinfo.h>
#include <kbd_port.h>
#include <kbd.h>
#include <ddi.h>
#include <errno.h>

static int gxemul_port_init(kbd_dev_t *);
static void gxemul_port_yield(void);
static void gxemul_port_reclaim(void);
static void gxemul_port_write(uint8_t data);

kbd_port_ops_t gxemul_port = {
	.init = gxemul_port_init,
	.yield = gxemul_port_yield,
	.reclaim = gxemul_port_reclaim,
	.write = gxemul_port_write
};

static kbd_dev_t *kbd_dev;

static irq_pio_range_t gxemul_ranges[] = {
	{
		.base = 0,
		.size = 1
	}
};

static irq_cmd_t gxemul_cmds[] = {
	{ 
		.cmd = CMD_PIO_READ_8, 
		.addr = (void *) 0, 	/* will be patched in run-time */
		.dstarg = 2,
	},
	{
		.cmd = CMD_ACCEPT
	}
};

static irq_code_t gxemul_kbd = {
	sizeof(gxemul_ranges) / sizeof(irq_pio_range_t),
	gxemul_ranges,
	sizeof(gxemul_cmds) / sizeof(irq_cmd_t),
	gxemul_cmds
};

static void gxemul_irq_handler(ipc_callid_t iid, ipc_call_t *call);

/** Initializes keyboard handler. */
static int gxemul_port_init(kbd_dev_t *kdev)
{
	kbd_dev = kdev;
	
	sysarg_t addr;
	if (sysinfo_get_value("kbd.address.physical", &addr) != EOK)
		return -1;
	
	sysarg_t inr;
	if (sysinfo_get_value("kbd.inr", &inr) != EOK)
		return -1;
	
	async_set_interrupt_received(gxemul_irq_handler);
	gxemul_ranges[0].base = addr;
	gxemul_cmds[0].addr = (void *) addr;
	irq_register(inr, device_assign_devno(), 0, &gxemul_kbd);
	return 0;
}

static void gxemul_port_yield(void)
{
}

static void gxemul_port_reclaim(void)
{
}

static void gxemul_port_write(uint8_t data)
{
	(void) data;
}

/** Process data sent when a key is pressed.
 *
 * @param keybuffer Buffer of pressed keys.
 * @param call      IPC call.
 *
 */
static void gxemul_irq_handler(ipc_callid_t iid, ipc_call_t *call)
{
	kbd_push_data(kbd_dev, IPC_GET_ARG2(*call));
}

/** @}
 */
