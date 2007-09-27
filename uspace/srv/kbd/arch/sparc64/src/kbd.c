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

/** @addtogroup kbdsparc64 sparc64
 * @brief	HelenOS sparc64 arch dependent parts of uspace keyboard handler.
 * @ingroup  kbd
 * @{
 */ 
/** @file
 */

#include <arch/kbd.h>
#include <ipc/ipc.h>
#include <sysinfo.h>
#include <kbd.h>
#include <keys.h>
#include <stdio.h>
#include <sys/types.h>
#include <genarch/kbd.h>

#define KBD_KEY_RELEASE		0x80
#define KBD_ALL_KEYS_UP		0x7f

/** Top-half pseudocode for z8530. */
irq_cmd_t z8530_cmds[] = {
	{
		CMD_MEM_READ_1,
		0,			/**< Address. Will be patched in run-time. */
		0,			/**< Value. Not used. */
		1			/**< Arg 1 will contain the result. */
	}
};
	
irq_code_t z8530_kbd = {
	1,
	z8530_cmds
};

/** Top-half pseudocode for ns16550. */
irq_cmd_t ns16550_cmds[] = {
	{
		CMD_MEM_READ_1,
		0,			/**< Address. Will be patched in run-time. */
		0,			/**< Value. Not used. */
		1			/**< Arg 1 will contain the result. */
	}
};
	
irq_code_t ns16550_kbd = {
	1,
	ns16550_cmds
};

#define KBD_Z8530	1
#define KBD_NS16550	2

int kbd_arch_init(void)
{
	int type = sysinfo_value("kbd.type");
	switch (type) {
	case KBD_Z8530:
		z8530_cmds[0].addr = (void *) sysinfo_value("kbd.address.virtual") + 6;
		ipc_register_irq(sysinfo_value("kbd.inr"), sysinfo_value("kbd.devno"), 0, &z8530_kbd);
		break;
	case KBD_NS16550:
		ns16550_cmds[0].addr = (void *) sysinfo_value("kbd.address.virtual");
		ipc_register_irq(sysinfo_value("kbd.inr"), sysinfo_value("kbd.devno"), 0, &ns16550_kbd);
		break;
	default:
		break;
	}
	return 0;
}

/** Process keyboard events */
int kbd_arch_process(keybuffer_t *keybuffer, ipc_call_t *call)
{
	int scan_code = IPC_GET_ARG1(*call);

	if (scan_code == KBD_ALL_KEYS_UP)
		return 1;
		
	if (scan_code & KBD_KEY_RELEASE)
		key_released(keybuffer, scan_code ^ KBD_KEY_RELEASE);
	else
		key_pressed(keybuffer, scan_code);

	return 1;
}

/** @}
 */
