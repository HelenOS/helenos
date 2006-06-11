/*
 * Copyright (C) 2006 Josef Cejka
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


#define FUNCTION_KEYS 0x100

irq_cmd_t ski_cmds[1] = {
	{ CMD_IA64_GETCHAR, 0, 0, 2 }
};

irq_code_t ski_kbd = {
	1,
	ski_cmds
};

int kbd_arch_init(void)
{
	if(sysinfo_value("kbd")) {
		ipc_register_irq(sysinfo_value("kbd.irq"), &ski_kbd);
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

int kbd_arch_process(keybuffer_t *keybuffer, ipc_call_t *call) 
{
	static unsigned long long buf=0;
	static int count=0;	
	static int esc_count=0;
	int scan_code = IPC_GET_ARG2(*call);


	/*
	* Please preserve this code (it can be used to determine scancodes)
	*/
	//keybuffer_push(keybuffer, to_hex((scan_code>>4)&0xf));
	//keybuffer_push(keybuffer, to_hex(scan_code&0xf));
	//keybuffer_push(keybuffer, ' ');
	//keybuffer_push(keybuffer, ' ');
	//*/
	
	
	if (scan_code){
		buf|=(unsigned long long) scan_code<<(8*(count++));
	} else {
		

		if ( buf == 0x1b ) {
			esc_count++;
			if ( esc_count == 3 ) {
			__SYSCALL0(SYS_DEBUG_ENABLE_CONSOLE);
			}	
		} else {
			esc_count=0;
		}
	
		if ( ! ( buf & 0xff00 ))
			keybuffer_push(keybuffer, buf);
		else {
			switch (buf){
				case KEY_F1:
					keybuffer_push(keybuffer,FUNCTION_KEYS | 1 );
					break;
				case KEY_F2:
					keybuffer_push(keybuffer,FUNCTION_KEYS | 2 );
					break;
				case KEY_F3:
					keybuffer_push(keybuffer,FUNCTION_KEYS | 3 );
					break;
				case KEY_F4:
					keybuffer_push(keybuffer,FUNCTION_KEYS | 4 );
					break;
				case KEY_F5:
					keybuffer_push(keybuffer,FUNCTION_KEYS | 5 );
					break;
				case KEY_F6:
					keybuffer_push(keybuffer,FUNCTION_KEYS | 6 );
					break;
				case KEY_F7:
					keybuffer_push(keybuffer,FUNCTION_KEYS | 7 );
					break;
				case KEY_F8:
					keybuffer_push(keybuffer,FUNCTION_KEYS | 8 );
					break;

				case KEY_F9:
					keybuffer_push(keybuffer,FUNCTION_KEYS | 9 );
					break;
				case KEY_F10:
					keybuffer_push(keybuffer,FUNCTION_KEYS | 10 );
					break;

				case KEY_F11:
					keybuffer_push(keybuffer,FUNCTION_KEYS | 11 );
					break;
				case KEY_F12:
					keybuffer_push(keybuffer,FUNCTION_KEYS | 12 );
					break;


			}
		}
		buf=count=0;
	}

	return 	1;
}

/**
 * @}
 */ 
