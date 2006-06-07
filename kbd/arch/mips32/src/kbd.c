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

/** @addtogroup kbdmips32 mips32
 * @brief	HelenOS mips32 arch dependent parts of uspace keyboard handler.
 * @ingroup  kbd
 * @{
 */ 
/** @file
 */
#include <arch/kbd.h>
#include <ipc/ipc.h>
#include <sysinfo.h>


#define MSIM_KEY_F1 0x504f1bL
#define MSIM_KEY_F2 0x514f1bL
#define MSIM_KEY_F3 0x524f1bL
#define MSIM_KEY_F4 0x534f1bL
#define MSIM_KEY_F5 0x35315b1bL
#define MSIM_KEY_F6 0x37315b1bL
#define MSIM_KEY_F7 0x38315b1bL
#define MSIM_KEY_F8 0x39315b1bL
#define MSIM_KEY_F9 0x30325b1bL
#define MSIM_KEY_F10 0x31325b1bL
#define MSIM_KEY_F11 0x33325b1bL
#define MSIM_KEY_F12 0x34325b1bL


#define GXEMUL_KEY_F1 0x504f5b1bL
#define GXEMUL_KEY_F2 0x514f5b1bL
#define GXEMUL_KEY_F3 0x524f5b1bL
#define GXEMUL_KEY_F4 0x534f5b1bL
#define GXEMUL_KEY_F5 0x35315b1bL
#define GXEMUL_KEY_F6 0x37315b1bL
#define GXEMUL_KEY_F7 0x38315b1bL
#define GXEMUL_KEY_F8 0x39315b1bL
#define GXEMUL_KEY_F9 0x38325b1bL
#define GXEMUL_KEY_F10 0x39325b1bL
#define GXEMUL_KEY_F11 0x33325b1bL
#define GXEMUL_KEY_F12 0x34325b1bL


#define FUNCTION_KEYS 0x100


irq_cmd_t msim_cmds[1] = {
	{ CMD_MEM_READ_1, (void *)0xB0000000, 0 }
};

irq_code_t msim_kbd = {
	1,
	msim_cmds
};

static int msim,gxemul;

int kbd_arch_init(void)
{
	ipc_register_irq(2, &msim_kbd);
	msim=sysinfo_value("machine.msim");
	gxemul=sysinfo_value("machine.lgxemul");
	return 1;
}


//*
//*
//* Please preserve this code (it can be used to determine scancodes)
//*
int to_hex(int v) 
{
        return "0123456789ABCDEF"[v];
}
//*/

static int kbd_arch_process_msim(keybuffer_t *keybuffer, int scan_code)
{

	static unsigned long buf=0;
	static int count=0;	


	//* Please preserve this code (it can be used to determine scancodes)
	//*
	//keybuffer_push(keybuffer, to_hex((scan_code>>4)&0xf));
	//keybuffer_push(keybuffer, to_hex(scan_code&0xf));
	//keybuffer_push(keybuffer, ' ');
	//keybuffer_push(keybuffer, ' ');
	//*/
	//return 1;
	
	
	if(scan_code==0x7e)
	{
		switch (buf){
			case MSIM_KEY_F5:
				keybuffer_push(keybuffer,FUNCTION_KEYS | 5 );
				buf=count=0;
				return 1;
			case MSIM_KEY_F6:
				keybuffer_push(keybuffer,FUNCTION_KEYS | 6 );
				buf=count=0;
				return 1;
			case MSIM_KEY_F7:
				keybuffer_push(keybuffer,FUNCTION_KEYS | 7 );
				buf=count=0;
				return 1;
			case MSIM_KEY_F8:
				keybuffer_push(keybuffer,FUNCTION_KEYS | 8 );
				buf=count=0;
				return 1;

			case MSIM_KEY_F9:
				keybuffer_push(keybuffer,FUNCTION_KEYS | 9 );
				buf=count=0;
				return 1;
			case MSIM_KEY_F10:
				keybuffer_push(keybuffer,FUNCTION_KEYS | 10 );
				buf=count=0;
				return 1;

			case MSIM_KEY_F11:
				keybuffer_push(keybuffer,FUNCTION_KEYS | 11 );
				buf=count=0;
				return 1;
			case MSIM_KEY_F12:
				keybuffer_push(keybuffer,FUNCTION_KEYS | 12 );
				buf=count=0;
				return 1;
			default:
				keybuffer_push(keybuffer, buf & 0xff );
				keybuffer_push(keybuffer, (buf >> 8) &0xff );
				keybuffer_push(keybuffer, (buf >> 16) &0xff );
				keybuffer_push(keybuffer, (buf >> 24) &0xff );
				keybuffer_push(keybuffer, scan_code );
				buf=count=0;
				return 1;
	
		}
	}

	buf|=((unsigned long) scan_code)<<(8*(count++));
	
	
	if((buf & 0xff)!= (MSIM_KEY_F1 & 0xff)) {

		keybuffer_push(keybuffer,buf );
		buf=count=0;
		return 1;
	}

	if ( count <= 1 ) 
		return 1;

	if(    (buf & 0xffff) != (MSIM_KEY_F1 & 0xffff) 
	    && (buf & 0xffff) != (MSIM_KEY_F5 & 0xffff) ) {

		keybuffer_push(keybuffer, buf & 0xff );
		keybuffer_push(keybuffer, (buf >> 8) &0xff );
		buf=count=0;
		return 1;
	}

	if ( count <= 2) 
		return 1;

	switch (buf){
		case MSIM_KEY_F1:
			keybuffer_push(keybuffer,FUNCTION_KEYS | 1 );
			buf=count=0;
			return 1;
		case MSIM_KEY_F2:
			keybuffer_push(keybuffer,FUNCTION_KEYS | 2 );
			buf=count=0;
			return 1;
		case MSIM_KEY_F3:
			keybuffer_push(keybuffer,FUNCTION_KEYS | 3 );
			buf=count=0;
			return 1;
		case MSIM_KEY_F4:
			keybuffer_push(keybuffer,FUNCTION_KEYS | 4 );
			buf=count=0;
			return 1;
	}


	if(    (buf & 0xffffff) != (MSIM_KEY_F5 & 0xffffff)
	    && (buf & 0xffffff) != (MSIM_KEY_F9 & 0xffffff) ) {

		keybuffer_push(keybuffer, buf & 0xff );
		keybuffer_push(keybuffer, (buf >> 8) &0xff );
		keybuffer_push(keybuffer, (buf >> 16) &0xff );
		buf=count=0;
		return 1;
	}

	if ( count <= 3 ) 
		return 1;
	

	
	
	switch (buf){
		case MSIM_KEY_F5:
		case MSIM_KEY_F6:
		case MSIM_KEY_F7:
		case MSIM_KEY_F8:
		case MSIM_KEY_F9:
		case MSIM_KEY_F10:
		case MSIM_KEY_F11:
		case MSIM_KEY_F12:
			return 1;
		default:
			keybuffer_push(keybuffer, buf & 0xff );
			keybuffer_push(keybuffer, (buf >> 8) &0xff );
			keybuffer_push(keybuffer, (buf >> 16) &0xff );
			keybuffer_push(keybuffer, (buf >> 24) &0xff );
			buf=count=0;
			return 1;
		
		}
	return 1;
}



static int kbd_arch_process_gxemul(keybuffer_t *keybuffer, int scan_code)
{

	static unsigned long buf=0;
	static int count=0;	


	//* Please preserve this code (it can be used to determine scancodes)
	//*
	//keybuffer_push(keybuffer, to_hex((scan_code>>4)&0xf));
	//keybuffer_push(keybuffer, to_hex(scan_code&0xf));
	//keybuffer_push(keybuffer, ' ');
	//keybuffer_push(keybuffer, ' ');
	//*/
	//return 1;
	
	
	if ( scan_code == '\r' )
		scan_code = '\n' ;
	
	buf|=((unsigned long) scan_code)<<(8*(count++));
	
	
	if((buf & 0xff)!= (GXEMUL_KEY_F1 & 0xff)) {

		keybuffer_push(keybuffer,buf );
		buf=count=0;
		return 1;
	}

	if ( count <= 1 ) 
		return 1;

	if(    (buf & 0xffff) != (GXEMUL_KEY_F1 & 0xffff)  ) {

		keybuffer_push(keybuffer, buf & 0xff );
		keybuffer_push(keybuffer, (buf >> 8) &0xff );
		buf=count=0;
		return 1;
	}

	if ( count <= 2) 
		return 1;


	if(    (buf & 0xffffff) != (GXEMUL_KEY_F1 & 0xffffff)
	    && (buf & 0xffffff) != (GXEMUL_KEY_F5 & 0xffffff)
	    && (buf & 0xffffff) != (GXEMUL_KEY_F9 & 0xffffff) ) {

		keybuffer_push(keybuffer, buf & 0xff );
		keybuffer_push(keybuffer, (buf >> 8) &0xff );
		keybuffer_push(keybuffer, (buf >> 16) &0xff );
		buf=count=0;
		return 1;
	}

	if ( count <= 3 ) 
		return 1;
	

	switch (buf){

		case GXEMUL_KEY_F1:
			keybuffer_push(keybuffer,FUNCTION_KEYS | 1 );
			buf=count=0;
			return 1;
		case GXEMUL_KEY_F2:
			keybuffer_push(keybuffer,FUNCTION_KEYS | 2 );
			buf=count=0;
			return 1;
		case GXEMUL_KEY_F3:
			keybuffer_push(keybuffer,FUNCTION_KEYS | 3 );
			buf=count=0;
			return 1;
		case GXEMUL_KEY_F4:
			keybuffer_push(keybuffer,FUNCTION_KEYS | 4 );
			buf=count=0;
			return 1;
		case GXEMUL_KEY_F5:
			keybuffer_push(keybuffer,FUNCTION_KEYS | 5 );
			buf=count=0;
			return 1;
		case GXEMUL_KEY_F6:
			keybuffer_push(keybuffer,FUNCTION_KEYS | 6 );
			buf=count=0;
			return 1;
		case GXEMUL_KEY_F7:
			keybuffer_push(keybuffer,FUNCTION_KEYS | 7 );
			buf=count=0;
			return 1;
		case GXEMUL_KEY_F8:
			keybuffer_push(keybuffer,FUNCTION_KEYS | 8 );
			buf=count=0;
			return 1;
		case GXEMUL_KEY_F9:
			keybuffer_push(keybuffer,FUNCTION_KEYS | 9 );
			buf=count=0;
			return 1;
		case GXEMUL_KEY_F10:
			keybuffer_push(keybuffer,FUNCTION_KEYS | 10 );
			buf=count=0;
			return 1;
		case GXEMUL_KEY_F11:
			keybuffer_push(keybuffer,FUNCTION_KEYS | 11 );
			buf=count=0;
			return 1;
		case GXEMUL_KEY_F12:
			keybuffer_push(keybuffer,FUNCTION_KEYS | 12 );
			buf=count=0;
			return 1;

		default:
			keybuffer_push(keybuffer, buf & 0xff );
			keybuffer_push(keybuffer, (buf >> 8) &0xff );
			keybuffer_push(keybuffer, (buf >> 16) &0xff );
			keybuffer_push(keybuffer, (buf >> 24) &0xff );
			buf=count=0;
			return 1;
		
		}
	return 1;
}

int kbd_arch_process(keybuffer_t *keybuffer, int scan_code)
{

	static int esc_count=0;

	
	if ( scan_code == 0x1b ) {
		esc_count++;
		if ( esc_count == 3 ) {
			__SYSCALL0(SYS_DEBUG_ENABLE_CONSOLE);
		}	
	} else {
		esc_count=0;
	}

	if(msim) return kbd_arch_process_msim(keybuffer, scan_code);
	if(gxemul) return kbd_arch_process_gxemul(keybuffer, scan_code);

	return 0;
}

/**
 * @}
 */ 
