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

/** @addtogroup kbdia64 ia64
 * @brief	HelenOS ia64 arch dependent parts of uspace keyboard handler.
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
#include <stdlib.h>

#include <unistd.h>
#include <align.h>
#include <async.h>
#include <ipc/ipc.h>
#include <errno.h>
#include <stdio.h>
#include <ddi.h>
#include <sysinfo.h>
#include <as.h>
#include <ipc/fb.h>
#include <ipc/ipc.h>
#include <ipc/ns.h>
#include <ipc/services.h>
#include <libarch/ddi.h>


extern int lkbd_arch_process(keybuffer_t *keybuffer, ipc_call_t *call);
extern int lkbd_arch_init(void);



#define KEY_F1 0x504f1b
#define KEY_F2 0x514f1b
#define KEY_F3 0x524f1b
#define KEY_F4 0x534f1b
#define KEY_F5 0x7e35315b1b
#define KEY_F6 0x7e37315b1b
#define KEY_F7 0x7e38315b1b
#define KEY_F8 0x7e39315b1b
#define KEY_F9 0x7e30325b1b
#define KEY_F10 0x7e31325b1b
#define KEY_F11 0x7e33325b1b
#define KEY_F12 0x7e34325b1b




#define NSKEY_F1 0x415b5b1b
#define NSKEY_F2 0x425b5b1b
#define NSKEY_F3 0x435b5b1b
#define NSKEY_F4 0x445b5b1b
#define NSKEY_F5 0x455b5b1b
#define NSKEY_F6 0x37315b1b
#define NSKEY_F7 0x38315b1b
#define NSKEY_F8 0x39315b1b
#define NSKEY_F9 0x30325b1b
#define NSKEY_F10 0x31325b1b
#define NSKEY_F11 0x33325b1b
#define NSKEY_F12 0x34325b1b


#define FUNCTION_KEYS 0x100


#define KBD_SKI 1
#define	KBD_LEGACY 2
#define	KBD_NS16550 3




/* NS16550 registers */
#define RBR_REG		0	/** Receiver Buffer Register. */
#define IER_REG		1	/** Interrupt Enable Register. */
#define IIR_REG		2	/** Interrupt Ident Register (read). */
#define FCR_REG		2	/** FIFO control register (write). */
#define LCR_REG		3	/** Line Control register. */
#define MCR_REG		4	/** Modem Control Register. */
#define LSR_REG		5	/** Line Status Register. */




irq_cmd_t ski_cmds[1] = {
	{ CMD_IA64_GETCHAR, 0, 0, 2 }
};

irq_code_t ski_kbd = {
	1,
	ski_cmds
};



irq_cmd_t ns16550_cmds[1] = {
	{ CMD_PORT_READ_1, 0, 0, 2 },
};

irq_code_t ns16550_kbd = {
	1,
	ns16550_cmds
};


uint16_t ns16550_port;

int kbd_type;

int kbd_arch_init(void)
{
	if (sysinfo_value("kbd")) {
		kbd_type=sysinfo_value("kbd.type");
		if(kbd_type==KBD_SKI) ipc_register_irq(sysinfo_value("kbd.inr"), sysinfo_value("kbd.devno"), 0, &ski_kbd);
		if(kbd_type==KBD_LEGACY) return lkbd_arch_init();
		if(kbd_type==KBD_NS16550) {
			ns16550_kbd.cmds[0].addr= (void *)  (sysinfo_value("kbd.port")+RBR_REG);
			ipc_register_irq(sysinfo_value("kbd.inr"), sysinfo_value("kbd.devno"), 0, &ns16550_kbd);
			iospace_enable(task_get_id(),ns16550_port=sysinfo_value("kbd.port"),8);
		}	
		return 0;
	}	
	return 1;
}

/*
* Please preserve this code (it can be used to determine scancodes)
*
int to_hex(int v) 
{
    return "0123456789ABCDEF"[v];
}
*/
#define LSR_DATA_READY	0x01

int kbd_ns16550_process(keybuffer_t *keybuffer, ipc_call_t *call) 
{
	static unsigned long buf = 0;
	static int count = 0, esc_count=0;	

	int scan_code = IPC_GET_ARG2(*call);

	if (scan_code == 0x1b) {
		esc_count++;
		if (esc_count == 3) {
			__SYSCALL0(SYS_DEBUG_ENABLE_CONSOLE);
		}	
	} else {
		esc_count = 0;
	}

	if(scan_code==0x0d) return 1;	//Delete CR

	if(scan_code == 0x7e) {
		switch (buf) {
		case NSKEY_F6:
			keybuffer_push(keybuffer,FUNCTION_KEYS | 6);
			buf = count = 0;
			return 1;
		case NSKEY_F7:
			keybuffer_push(keybuffer,FUNCTION_KEYS | 7);
			buf = count = 0;
			return 1;
		case NSKEY_F8:
			keybuffer_push(keybuffer,FUNCTION_KEYS | 8);
			buf = count = 0;
			return 1;
		case NSKEY_F9:
			keybuffer_push(keybuffer,FUNCTION_KEYS | 9);
			buf = count = 0;
			return 1;
		case NSKEY_F10:
			keybuffer_push(keybuffer,FUNCTION_KEYS | 10);
			buf = count = 0;
			return 1;
		case NSKEY_F11:
			keybuffer_push(keybuffer,FUNCTION_KEYS | 11);
			buf = count = 0;
			return 1;
		case NSKEY_F12:
			keybuffer_push(keybuffer,FUNCTION_KEYS | 12);
			buf = count = 0;
			return 1;
		default:
			keybuffer_push(keybuffer, buf & 0xff);
			keybuffer_push(keybuffer, (buf >> 8) &0xff);
			keybuffer_push(keybuffer, (buf >> 16) &0xff);
			keybuffer_push(keybuffer, (buf >> 24) &0xff);
			keybuffer_push(keybuffer, scan_code);
			buf = count = 0;
			return 1;
		}
	}

	buf |= ((unsigned long) scan_code)<<(8*(count++));
	
	if((buf & 0xff) != (NSKEY_F1 & 0xff)) {
		keybuffer_push(keybuffer, buf);
		buf = count = 0;
		return 1;
	}

	if (count <= 1) 
		return 1;

	if ((buf & 0xffff) != (NSKEY_F1 & 0xffff))  {

		keybuffer_push(keybuffer, buf & 0xff);
		keybuffer_push(keybuffer, (buf >> 8) &0xff);
		buf = count = 0;
		return 1;
	}

	if (count <= 2) 
		return 1;


	if ((buf & 0xffffff) != (NSKEY_F1 & 0xffffff) 
		&& (buf & 0xffffff) != (NSKEY_F6 & 0xffffff) 
		&& (buf & 0xffffff) != (NSKEY_F9 & 0xffffff) ) {

		keybuffer_push(keybuffer, buf & 0xff);
		keybuffer_push(keybuffer, (buf >> 8) &0xff);
		keybuffer_push(keybuffer, (buf >> 16) &0xff);
		buf = count = 0;
		return 1;
	}

	if (count <= 3) 
		return 1;

	switch (buf) {
	case NSKEY_F1:
		keybuffer_push(keybuffer,FUNCTION_KEYS | 1);
		buf = count = 0;
		return 1;
	case NSKEY_F2:
		keybuffer_push(keybuffer,FUNCTION_KEYS | 2);
		buf = count = 0;
		return 1;
	case NSKEY_F3:
		keybuffer_push(keybuffer,FUNCTION_KEYS | 3);
		buf = count = 0;
		return 1;
	case NSKEY_F4:
		keybuffer_push(keybuffer,FUNCTION_KEYS | 4);
		buf = count = 0;
		return 1;
	case NSKEY_F5:
		keybuffer_push(keybuffer,FUNCTION_KEYS | 5);
		buf = count = 0;
		return 1;
	}


	
	switch (buf) {
	case NSKEY_F6:
	case NSKEY_F7:
	case NSKEY_F8:
	case NSKEY_F9:
	case NSKEY_F10:
	case NSKEY_F11:
	case NSKEY_F12:
		return 1;
	default:
		keybuffer_push(keybuffer, buf & 0xff);
		keybuffer_push(keybuffer, (buf >> 8) &0xff);
		keybuffer_push(keybuffer, (buf >> 16) &0xff);
		keybuffer_push(keybuffer, (buf >> 24) &0xff);
		buf = count = 0;
		return 1;
	}
	return 1;
}







int kbd_ski_process(keybuffer_t *keybuffer, ipc_call_t *call) 
{
	static unsigned long long buf = 0;
	static int count = 0;	
	static int esc_count = 0;
	int scan_code = IPC_GET_ARG2(*call);
	
	/*
	 * Please preserve this code (it can be used to determine scancodes)
	 */
	//keybuffer_push(keybuffer, to_hex((scan_code>>4)&0xf));
	//keybuffer_push(keybuffer, to_hex(scan_code&0xf));
	//keybuffer_push(keybuffer, ' ');
	//keybuffer_push(keybuffer, ' ');
	//*/
	
	if (scan_code) {
		buf |= (unsigned long long) scan_code<<(8*(count++));
	} else {
		
		if (buf == 0x1b) {
			esc_count++;
			if (esc_count == 3) {
				__SYSCALL0(SYS_DEBUG_ENABLE_CONSOLE);
			}	
		} else {
			esc_count = 0;
		}
	
		if (!(buf & 0xff00)) {
			keybuffer_push(keybuffer, buf);
		} else {
			switch (buf) {
			case KEY_F1:
				keybuffer_push(keybuffer, FUNCTION_KEYS | 1);
				break;
			case KEY_F2:
				keybuffer_push(keybuffer, FUNCTION_KEYS | 2);
				break;
			case KEY_F3:
				keybuffer_push(keybuffer, FUNCTION_KEYS | 3);
				break;
			case KEY_F4:
				keybuffer_push(keybuffer, FUNCTION_KEYS | 4);
				break;
			case KEY_F5:
				keybuffer_push(keybuffer, FUNCTION_KEYS | 5);
				break;
			case KEY_F6:
				keybuffer_push(keybuffer, FUNCTION_KEYS | 6);
				break;
			case KEY_F7:
				keybuffer_push(keybuffer, FUNCTION_KEYS | 7);
				break;
			case KEY_F8:
				keybuffer_push(keybuffer, FUNCTION_KEYS | 8);
				break;
			case KEY_F9:
				keybuffer_push(keybuffer, FUNCTION_KEYS | 9);
				break;
			case KEY_F10:
				keybuffer_push(keybuffer, FUNCTION_KEYS | 10);
				break;
			case KEY_F11:
				keybuffer_push(keybuffer, FUNCTION_KEYS | 11);
				break;
			case KEY_F12:
				keybuffer_push(keybuffer, FUNCTION_KEYS | 12);
				break;
			}
		}
		buf = count = 0;
	}
	return 	1;
}

int kbd_arch_process(keybuffer_t *keybuffer, ipc_call_t *call) 
{
	printf("KBD Key pressed: %x(%c)\n",IPC_GET_ARG2(*call),IPC_GET_ARG2(*call));
	if(kbd_type==KBD_SKI) return kbd_ski_process(keybuffer,call);
	if(kbd_type==KBD_NS16550) return kbd_ns16550_process(keybuffer,call);
	if(kbd_type==KBD_LEGACY) return lkbd_arch_process(keybuffer,call);

	
}



/**
 * @}
 */ 
