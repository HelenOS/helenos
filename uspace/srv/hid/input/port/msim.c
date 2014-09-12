/*
 * Copyright (c) 2006 Josef Cejka
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
 * @brief Msim keyboard port driver.
 */

#include <async.h>
#include <sysinfo.h>
#include <ddi.h>
#include <errno.h>
#include "../kbd_port.h"
#include "../kbd.h"

static int msim_port_init(kbd_dev_t *);
static void msim_port_write(uint8_t data);

kbd_port_ops_t msim_port = {
	.init = msim_port_init,
	.write = msim_port_write
};

static kbd_dev_t *kbd_dev;

static irq_pio_range_t msim_ranges[] = {
	{
		.base = 0,
		.size = 1
	}
};

static irq_cmd_t msim_cmds[] = {
	{
		.cmd = CMD_PIO_READ_8,
		.addr = (void *) 0,	/* will be patched in run-time */
		.dstarg = 2
	},
	{
		.cmd = CMD_ACCEPT
	}
};

static irq_code_t msim_kbd = {
	sizeof(msim_ranges) / sizeof(irq_pio_range_t),
	msim_ranges,
	sizeof(msim_cmds) / sizeof(irq_cmd_t),
	msim_cmds
};

static void msim_irq_handler(ipc_callid_t iid, ipc_call_t *call, void *arg)
{
	kbd_push_data(kbd_dev, IPC_GET_ARG2(*call));
}

static int msim_port_init(kbd_dev_t *kdev)
{
	kbd_dev = kdev;

	sysarg_t paddr;
	if (sysinfo_get_value("kbd.address.physical", &paddr) != EOK)
		return -1;
	
	sysarg_t inr;
	if (sysinfo_get_value("kbd.inr", &inr) != EOK)
		return -1;
	
	msim_ranges[0].base = paddr;
	msim_cmds[0].addr = (void *) paddr;
	async_irq_subscribe(inr, device_assign_devno(), msim_irq_handler, NULL,
	    &msim_kbd);
	
	return 0;
}

static void msim_port_write(uint8_t data)
{
	(void) data;
}

/** @}
 */
