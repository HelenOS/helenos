/*
 * Copyright (c) 2009 Vineeth Pillai
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
 * @ingroup kbd
 * @{
 */ 
/** @file
 * @brief pl050 port driver.
 */

#include <ddi.h>
#include <libarch/ddi.h>
#include <async.h>
#include <unistd.h>
#include <sysinfo.h>
#include <kbd_port.h>
#include <kbd.h>
#include <ddi.h>
#include <stdio.h>
#include <errno.h>

static int pl050_port_init(kbd_dev_t *);
static void pl050_port_yield(void);
static void pl050_port_reclaim(void);
static void pl050_port_write(uint8_t data);

kbd_port_ops_t pl050_port = {
	.init = pl050_port_init,
	.yield = pl050_port_yield,
	.reclaim = pl050_port_reclaim,
	.write = pl050_port_write
};

static kbd_dev_t *kbd_dev;

#define PL050_STAT	4
#define PL050_DATA	8

#define PL050_STAT_RXFULL  (1 << 4)

static irq_pio_range_t pl050_ranges[] = {
	{
		.base = 0,
		.size = 9, 
	}
};

static irq_cmd_t pl050_cmds[] = {
	{
		.cmd = CMD_PIO_READ_8,
		.addr = NULL,
		.dstarg = 1
	},
	{
		.cmd = CMD_AND,
		.value = PL050_STAT_RXFULL,
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
		.addr = NULL,  /* Will be patched in run-time */
		.dstarg = 2
	},
	{
		.cmd = CMD_ACCEPT
	}
};

static irq_code_t pl050_kbd = {
	sizeof(pl050_ranges) / sizeof(irq_pio_range_t),
	pl050_ranges,
	sizeof(pl050_cmds) / sizeof(irq_cmd_t),
	pl050_cmds
};

static void pl050_irq_handler(ipc_callid_t iid, ipc_call_t *call);

static int pl050_port_init(kbd_dev_t *kdev)
{
	kbd_dev = kdev;
	
	sysarg_t addr;
	if (sysinfo_get_value("kbd.address.physical", &addr) != EOK)
		return -1;
	
	pl050_kbd.ranges[0].base = addr;
	pl050_kbd.cmds[0].addr = (void *) addr + PL050_STAT;
	pl050_kbd.cmds[3].addr = (void *) addr + PL050_DATA;
	
	sysarg_t inr;
	if (sysinfo_get_value("kbd.inr", &inr) != EOK)
		return -1;
	
	async_set_interrupt_received(pl050_irq_handler);
	irq_register(inr, device_assign_devno(), 0, &pl050_kbd);
	
	return 0;
}

static void pl050_port_yield(void)
{
}

static void pl050_port_reclaim(void)
{
}

static void pl050_port_write(uint8_t data)
{
	(void) data;
}

static void pl050_irq_handler(ipc_callid_t iid, ipc_call_t *call)
{
	kbd_push_data(kbd_dev, IPC_GET_ARG2(*call));
}

/**
 * @}
 */
