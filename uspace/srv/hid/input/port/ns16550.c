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
 * @brief NS16550 port driver.
 */

#include <ipc/irc.h>
#include <async.h>
#include <sysinfo.h>
#include <ddi.h>
#include <errno.h>
#include "../input.h"
#include "../kbd_port.h"
#include "../kbd.h"

static int ns16550_port_init(kbd_dev_t *);
static void ns16550_port_write(uint8_t data);

kbd_port_ops_t ns16550_port = {
	.init = ns16550_port_init,
	.write = ns16550_port_write
};

static kbd_dev_t *kbd_dev;

/* NS16550 registers */
#define RBR_REG  0  /** Receiver Buffer Register. */
#define IER_REG  1  /** Interrupt Enable Register. */
#define IIR_REG  2  /** Interrupt Ident Register (read). */
#define FCR_REG  2  /** FIFO control register (write). */
#define LCR_REG  3  /** Line Control register. */
#define MCR_REG  4  /** Modem Control Register. */
#define LSR_REG  5  /** Line Status Register. */

#define LSR_DATA_READY  0x01

static irq_pio_range_t ns16550_ranges[] = {
	{
		.base = 0,
		.size = 8
	}
};

static irq_cmd_t ns16550_cmds[] = {
	{
		.cmd = CMD_PIO_READ_8,
		.addr = (void *) 0,     /* Will be patched in run-time */
		.dstarg = 1
	},
	{
		.cmd = CMD_AND,
		.value = LSR_DATA_READY,
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

irq_code_t ns16550_kbd = {
	sizeof(ns16550_ranges) / sizeof(irq_pio_range_t),
	ns16550_ranges,
	sizeof(ns16550_cmds) / sizeof(irq_cmd_t),
	ns16550_cmds
};

static uintptr_t ns16550_physical;
static kbd_dev_t *kbd_dev;
static sysarg_t inr;

static void ns16550_irq_handler(ipc_callid_t iid, ipc_call_t *call, void *arg)
{
	kbd_push_data(kbd_dev, IPC_GET_ARG2(*call));
	
	if (irc_service) {
		async_exch_t *exch = async_exchange_begin(irc_sess);
		async_msg_1(exch, IRC_CLEAR_INTERRUPT, inr);
		async_exchange_end(exch);
	}
}

static int ns16550_port_init(kbd_dev_t *kdev)
{
	kbd_dev = kdev;
	
	sysarg_t ns16550;
	if (sysinfo_get_value("kbd.type.ns16550", &ns16550) != EOK)
		return -1;
	if (!ns16550)
		return -1;
	
	if (sysinfo_get_value("kbd.address.physical", &ns16550_physical) != EOK)
		return -1;
	
	if (sysinfo_get_value("kbd.inr", &inr) != EOK)
		return -1;
	
	ns16550_kbd.ranges[0].base = ns16550_physical;
	ns16550_kbd.cmds[0].addr = (void *) (ns16550_physical + LSR_REG);
	ns16550_kbd.cmds[3].addr = (void *) (ns16550_physical + RBR_REG);
	
	async_irq_subscribe(inr, device_assign_devno(), ns16550_irq_handler, NULL,
	    &ns16550_kbd);
	
	void *vaddr;
	return pio_enable((void *) ns16550_physical, 8, &vaddr);
}

static void ns16550_port_write(uint8_t data)
{
	(void) data;
}

/**
 * @}
 */
