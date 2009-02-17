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

/** @addtogroup kbd
 * @{
 */ 
/** @file
 */

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

static irq_cmd_t i8042_cmds[2] = {
	{ CMD_PORT_READ_1, (void *) 0x64, 0, 1 },
	{ CMD_PORT_READ_1, (void *) 0x60, 0, 2 }
};

static irq_code_t i8042_kbd = {
	2,
	i8042_cmds
};

static void wait_ready(void) {
	while (i8042_status_read() & i8042_INPUT_FULL)
	    ;
}

static void i8042_irq_handler(ipc_callid_t iid, ipc_call_t *call);

int kbd_port_init(void)
{
//	int i;
	int mouseenabled = 0;

	async_set_interrupt_received(i8042_irq_handler);
	iospace_enable(task_get_id(), (void *) i8042_DATA, 5);

	/* Disable kbd, enable mouse */
	i8042_command_write(i8042_CMD_KBD);
	wait_ready();
	i8042_command_write(i8042_CMD_KBD);
	wait_ready();
	i8042_data_write(i8042_KBD_DISABLE);
	wait_ready();

	/* Flush all current IO */
	while (i8042_status_read() & i8042_OUTPUT_FULL)
		i8042_data_read();
	
	/* Initialize mouse */
/*	i8042_command_write(i8042_CMD_MOUSE);
	wait_ready();
	i8042_data_write(MOUSE_OUT_INIT);
	wait_ready();
	
	int mouseanswer = 0;
	for (i=0;i < 1000; i++) {
		int status = i8042_status_read();
		if (status & i8042_OUTPUT_FULL) {
			int data = i8042_data_read();
			if (status & i8042_MOUSE_DATA) {
				mouseanswer = data;
				break;
			}
		}
		usleep(1000);
	}*/
//	if (mouseanswer == MOUSE_ACK) {
//		/* enable mouse */
//		mouseenabled = 1;
//		
//		ipc_register_irq(sysinfo_value("mouse.inr"), sysinfo_value("mouse.devno"), 0, &i8042_kbd);
//	}

	/* Enable kbd */
	ipc_register_irq(sysinfo_value("kbd.inr"), sysinfo_value("kbd.devno"), 0, &i8042_kbd);

	int newcontrol = i8042_KBD_IE | i8042_KBD_TRANSLATE;
	if (mouseenabled)
		newcontrol |= i8042_MOUSE_IE;
	
	i8042_command_write(i8042_CMD_KBD);
	wait_ready();
	i8042_data_write(newcontrol);
	wait_ready();
	
	return 0;
}

static void i8042_irq_handler(ipc_callid_t iid, ipc_call_t *call)
{
	int status = IPC_GET_ARG1(*call);

	if ((status & i8042_MOUSE_DATA))
		return 0;

	int scan_code = IPC_GET_ARG2(*call);

	kbd_push_scancode(scan_code);
	return 1;
}

/**
 * @}
 */ 
