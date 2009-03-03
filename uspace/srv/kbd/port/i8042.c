/*
 * Copyright (c) 2001-2004 Jakub Jermar
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
 * @ingroup kbd
 * @{
 */ 
/** @file
 * @brief i8042 port driver.
 */

#include <ddi.h>
#include <libarch/ddi.h>
#include <ipc/ipc.h>
#include <async.h>
#include <unistd.h>
#include <sysinfo.h>
#include <kbd_port.h>
#include <kbd.h>
#include "i8042.h"

/* Interesting bits for status register */
#define i8042_OUTPUT_FULL  0x1
#define i8042_INPUT_FULL   0x2
#define i8042_MOUSE_DATA   0x20

/* Command constants */
#define i8042_CMD_KBD 0x60
#define i8042_CMD_MOUSE  0xd4

/* Keyboard cmd byte */
#define i8042_KBD_IE        0x1
#define i8042_MOUSE_IE      0x2
#define i8042_KBD_DISABLE   0x10
#define i8042_MOUSE_DISABLE 0x20
#define i8042_KBD_TRANSLATE 0x40

/* Mouse constants */
#define MOUSE_OUT_INIT  0xf4
#define MOUSE_ACK       0xfa

static irq_cmd_t i8042_cmds[] = {
	{
		.cmd = CMD_PIO_READ_8,
		.addr = NULL,	/* will be patched in run-time */
		.dstarg = 1
	},
	{
		.cmd = CMD_BTEST,
		.value = i8042_OUTPUT_FULL,
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
		.addr = NULL,	/* will be patched in run-time */
		.dstarg = 2
	},
	{
		.cmd = CMD_ACCEPT
	}
};

static irq_code_t i8042_kbd = {
	sizeof(i8042_cmds) / sizeof(irq_cmd_t),
	i8042_cmds
};

static uintptr_t i8042_physical;
static uintptr_t i8042_kernel;
static i8042_t * i8042;

static void wait_ready(void) {
	while (pio_read_8(&i8042->status) & i8042_INPUT_FULL)
		;
}

static void i8042_irq_handler(ipc_callid_t iid, ipc_call_t *call);

int kbd_port_init(void)
{
	int mouseenabled = 0;
	void *vaddr;

	i8042_physical = sysinfo_value("kbd.address.physical");
	i8042_kernel = sysinfo_value("kbd.address.kernel");
	if (pio_enable((void *) i8042_physical, sizeof(i8042_t), &vaddr) != 0)
		return -1;
	i8042 = vaddr;

	async_set_interrupt_received(i8042_irq_handler);

	/* Disable kbd, enable mouse */
	pio_write_8(&i8042->status, i8042_CMD_KBD);
	wait_ready();
	pio_write_8(&i8042->status, i8042_CMD_KBD);
	wait_ready();
	pio_write_8(&i8042->data, i8042_KBD_DISABLE);
	wait_ready();

	/* Flush all current IO */
	while (pio_read_8(&i8042->status) & i8042_OUTPUT_FULL)
		(void) pio_read_8(&i8042->data);
	
	/* Enable kbd */
	i8042_kbd.cmds[0].addr = &((i8042_t *) i8042_kernel)->status;
	i8042_kbd.cmds[3].addr = &((i8042_t *) i8042_kernel)->data;
	ipc_register_irq(sysinfo_value("kbd.inr"), sysinfo_value("kbd.devno"), 0, &i8042_kbd);

	int newcontrol = i8042_KBD_IE | i8042_KBD_TRANSLATE;
	if (mouseenabled)
		newcontrol |= i8042_MOUSE_IE;
	
	pio_write_8(&i8042->status, i8042_CMD_KBD);
	wait_ready();
	pio_write_8(&i8042->data, newcontrol);
	wait_ready();
	
	return 0;
}

static void i8042_irq_handler(ipc_callid_t iid, ipc_call_t *call)
{
	int status = IPC_GET_ARG1(*call);

	if ((status & i8042_MOUSE_DATA))
		return;

	int scan_code = IPC_GET_ARG2(*call);

	kbd_push_scancode(scan_code);
	return;
}

/**
 * @}
 */ 
