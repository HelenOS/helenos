/*
 * Copyright (c) 2007 Michal Kebrt
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

/** @addtogroup kbdarm32gxemul GXemul
 * @brief	HelenOS arm32 GXEmul uspace keyboard handler.
 * @ingroup  kbdarm32
 * @{
 */ 
/** @file
 *  @brief GXemul uspace keyboard handler.
 */

#include <ipc/ipc.h>
#include <sysinfo.h>
#include <kbd.h>
#include <keys.h>
#include <bool.h>


/* GXemul key codes in no-framebuffer mode. */
#define GXEMUL_KEY_F1  0x504f1bL
#define GXEMUL_KEY_F2  0x514f1bL
#define GXEMUL_KEY_F3  0x524f1bL
#define GXEMUL_KEY_F4  0x534f1bL
#define GXEMUL_KEY_F5  0x35315b1bL
#define GXEMUL_KEY_F6  0x37315b1bL
#define GXEMUL_KEY_F7  0x38315b1bL
#define GXEMUL_KEY_F8  0x39315b1bL
#define GXEMUL_KEY_F9  0x30325b1bL
#define GXEMUL_KEY_F10 0x31325b1bL
#define GXEMUL_KEY_F11 0x33325d1bL
#define GXEMUL_KEY_F12 0x34325b1bL 

/** Start code of F5-F12 keys. */
#define GXEMUL_KEY_F5_F12_START_CODE 0x7e

/* GXemul key codes in framebuffer mode. */
#define GXEMUL_FB_KEY_F1 0x504f5b1bL
#define GXEMUL_FB_KEY_F2 0x514f5b1bL
#define GXEMUL_FB_KEY_F3 0x524f5b1bL
#define GXEMUL_FB_KEY_F4 0x534f5b1bL
#define GXEMUL_FB_KEY_F5 0x35315b1bL
#define GXEMUL_FB_KEY_F6 0x37315b1bL
#define GXEMUL_FB_KEY_F7 0x38315b1bL
#define GXEMUL_FB_KEY_F8 0x39315b1bL
#define GXEMUL_FB_KEY_F9 0x38325b1bL
#define GXEMUL_FB_KEY_F10 0x39325b1bL
#define GXEMUL_FB_KEY_F11 0x33325b1bL
#define GXEMUL_FB_KEY_F12 0x34325b1bL


/** Function keys start code (F1=0x101) */
#define FUNCTION_KEYS 0x100

static irq_cmd_t gxemul_cmds[] = {
	{ 
		CMD_MEM_READ_1, 
		(void *) 0, 
		0, 
		2
	}
};

static irq_code_t gxemul_kbd = {
	1,
	gxemul_cmds
};


/** Framebuffer switched on. */
static bool fb;


/*
// Please preserve this code (it can be used to determine scancodes)
int to_hex(int v) 
{
        return "0123456789ABCDEF"[v];
}
*/


/** Process data sent when a key is pressed (in no-framebuffer mode).
 *  
 *  @param keybuffer Buffer of pressed key.
 *  @param scan_code Scan code.
 *
 *  @return Always 1.
 */
static int gxemul_kbd_process_no_fb(keybuffer_t *keybuffer, int scan_code)
{
	// holds at most 4 latest scan codes
	static unsigned long buf = 0;

	// number of scan codes in #buf
	static int count = 0;	

	/*
	// Preserve for detecting scan codes. 
	keybuffer_push0(keybuffer, to_hex((scan_code>>4)&0xf));
	keybuffer_push0(keybuffer, to_hex(scan_code&0xf));
	keybuffer_push0(keybuffer, 'X');
	keybuffer_push0(keybuffer, 'Y');
	return 1;
	*/

	if (scan_code == '\r') {
		scan_code = '\n';
	}
	
	if (scan_code == GXEMUL_KEY_F5_F12_START_CODE) {
		switch (buf) {
		case GXEMUL_KEY_F5:
			keybuffer_push0(keybuffer,FUNCTION_KEYS | 5);
			buf = count = 0;
			return 1;
		case GXEMUL_KEY_F6:
			keybuffer_push0(keybuffer,FUNCTION_KEYS | 6);
			buf = count = 0;
			return 1;
		case GXEMUL_KEY_F7:
			keybuffer_push0(keybuffer,FUNCTION_KEYS | 7);
			buf = count = 0;
			return 1;
		case GXEMUL_KEY_F8:
			keybuffer_push0(keybuffer,FUNCTION_KEYS | 8);
			buf = count = 0;
			return 1;
		case GXEMUL_KEY_F9:
			keybuffer_push0(keybuffer,FUNCTION_KEYS | 9);
			buf = count = 0;
			return 1;
		case GXEMUL_KEY_F10:
			keybuffer_push0(keybuffer,FUNCTION_KEYS | 10);
			buf = count = 0;
			return 1;
		case GXEMUL_KEY_F11:
			keybuffer_push0(keybuffer,FUNCTION_KEYS | 11);
			buf = count = 0;
			return 1;
		case GXEMUL_KEY_F12:
			keybuffer_push0(keybuffer,FUNCTION_KEYS | 12);
			buf = count = 0;
			return 1;
		default:
			keybuffer_push0(keybuffer, buf & 0xff);
			keybuffer_push0(keybuffer, (buf >> 8)  & 0xff);
			keybuffer_push0(keybuffer, (buf >> 16) & 0xff);
			keybuffer_push0(keybuffer, (buf >> 24) & 0xff);
			keybuffer_push0(keybuffer, scan_code);
			buf = count = 0;
			return 1;
		}
	}

	// add to buffer
	buf |= ((unsigned long) scan_code) << (8 * (count++));
	
	if ((buf & 0xff) != (GXEMUL_KEY_F1 & 0xff)) {
		keybuffer_push0(keybuffer, buf);
		buf = count = 0;
		return 1;
	}

	if (count <= 1) { 
		return 1;
	}

	if ((buf & 0xffff) != (GXEMUL_KEY_F1 & 0xffff) 
		&& (buf & 0xffff) != (GXEMUL_KEY_F5 & 0xffff) ) {

		keybuffer_push0(keybuffer, buf & 0xff);
		keybuffer_push0(keybuffer, (buf >> 8) &0xff);
		buf = count = 0;
		return 1;
	}

	if (count <= 2) {
		return 1;
	}

	switch (buf) {
	case GXEMUL_KEY_F1:
		keybuffer_push0(keybuffer,FUNCTION_KEYS | 1);
		buf = count = 0;
		return 1;
	case GXEMUL_KEY_F2:
		keybuffer_push0(keybuffer,FUNCTION_KEYS | 2);
		buf = count = 0;
		return 1;
	case GXEMUL_KEY_F3:
		keybuffer_push0(keybuffer,FUNCTION_KEYS | 3);
		buf = count = 0;
		return 1;
	case GXEMUL_KEY_F4:
		keybuffer_push0(keybuffer,FUNCTION_KEYS | 4);
		buf = count = 0;
		return 1;
	}


	if ((buf & 0xffffff) != (GXEMUL_KEY_F5 & 0xffffff)
		&& (buf & 0xffffff) != (GXEMUL_KEY_F9 & 0xffffff)) {

		keybuffer_push0(keybuffer, buf & 0xff);
		keybuffer_push0(keybuffer, (buf >> 8) & 0xff);
		keybuffer_push0(keybuffer, (buf >> 16) & 0xff);
		buf = count = 0;
		return 1;
	}

	if (count <= 3) {
		return 1;
	}
	
	switch (buf) {
	case GXEMUL_KEY_F5:
	case GXEMUL_KEY_F6:
	case GXEMUL_KEY_F7:
	case GXEMUL_KEY_F8:
	case GXEMUL_KEY_F9:
	case GXEMUL_KEY_F10:
	case GXEMUL_KEY_F11:
	case GXEMUL_KEY_F12:
		return 1;
	default:
		keybuffer_push0(keybuffer, buf & 0xff);
		keybuffer_push0(keybuffer, (buf >> 8)  & 0xff);
		keybuffer_push0(keybuffer, (buf >> 16) & 0xff);
		keybuffer_push0(keybuffer, (buf >> 24) & 0xff);
		buf = count = 0;
		return 1;
	}

	return 1;
}


/** Process data sent when a key is pressed (in framebuffer mode).
 *  
 *  @param keybuffer Buffer of pressed keys.
 *  @param scan_code Scan code.
 *
 *  @return Always 1.
 */
static int gxemul_kbd_process_fb(keybuffer_t *keybuffer, int scan_code)
{
	// holds at most 4 latest scan codes
	static unsigned long buf = 0;

	// number of scan codes in #buf
	static int count = 0;	

	/*
	// Please preserve this code (it can be used to determine scancodes)
	keybuffer_push0(keybuffer, to_hex((scan_code>>4)&0xf));
	keybuffer_push0(keybuffer, to_hex(scan_code&0xf));
	keybuffer_push0(keybuffer, ' ');
	keybuffer_push0(keybuffer, ' ');
	return 1;
	*/
	
	if (scan_code == '\r') {
		scan_code = '\n';
	}
	
	// add to buffer
	buf |= ((unsigned long) scan_code) << (8*(count++));
	
	
	if ((buf & 0xff) != (GXEMUL_FB_KEY_F1 & 0xff)) {
		keybuffer_push0(keybuffer, buf);
		buf = count = 0;
		return 1;
	}

	if (count <= 1) {
		return 1;
	}

	if ((buf & 0xffff) != (GXEMUL_FB_KEY_F1 & 0xffff)) {
		keybuffer_push0(keybuffer, buf & 0xff);
		keybuffer_push0(keybuffer, (buf >> 8) &0xff);
		buf = count = 0;
		return 1;
	}

	if (count <= 2) {
		return 1;
	}

	if ((buf & 0xffffff) != (GXEMUL_FB_KEY_F1 & 0xffffff)
		&& (buf & 0xffffff) != (GXEMUL_FB_KEY_F5 & 0xffffff)
		&& (buf & 0xffffff) != (GXEMUL_FB_KEY_F9 & 0xffffff)) {

		keybuffer_push0(keybuffer, buf & 0xff);
		keybuffer_push0(keybuffer, (buf >> 8) & 0xff);
		keybuffer_push0(keybuffer, (buf >> 16) & 0xff);
		buf = count = 0;
		return 1;
	}

	if (count <= 3) {
		return 1;
	}

	switch (buf) {
	case GXEMUL_FB_KEY_F1:
		keybuffer_push0(keybuffer,FUNCTION_KEYS | 1 );
		buf = count = 0;
		return 1;
	case GXEMUL_FB_KEY_F2:
		keybuffer_push0(keybuffer,FUNCTION_KEYS | 2 );
		buf = count = 0;
		return 1;
	case GXEMUL_FB_KEY_F3:
		keybuffer_push0(keybuffer,FUNCTION_KEYS | 3 );
		buf = count = 0;
		return 1;
	case GXEMUL_FB_KEY_F4:
		keybuffer_push0(keybuffer,FUNCTION_KEYS | 4 );
		buf = count = 0;
		return 1;
	case GXEMUL_FB_KEY_F5:
		keybuffer_push0(keybuffer,FUNCTION_KEYS | 5 );
		buf = count = 0;
		return 1;
	case GXEMUL_FB_KEY_F6:
		keybuffer_push0(keybuffer,FUNCTION_KEYS | 6 );
		buf = count = 0;
		return 1;
	case GXEMUL_FB_KEY_F7:
		keybuffer_push0(keybuffer,FUNCTION_KEYS | 7 );
		buf = count = 0;
		return 1;
	case GXEMUL_FB_KEY_F8:
		keybuffer_push0(keybuffer,FUNCTION_KEYS | 8 );
		buf = count = 0;
		return 1;
	case GXEMUL_FB_KEY_F9:
		keybuffer_push0(keybuffer,FUNCTION_KEYS | 9 );
		buf = count = 0;
		return 1;
	case GXEMUL_FB_KEY_F10:
		keybuffer_push0(keybuffer,FUNCTION_KEYS | 10 );
		buf = count = 0;
		return 1;
	case GXEMUL_FB_KEY_F11:
		keybuffer_push0(keybuffer,FUNCTION_KEYS | 11 );
		buf = count = 0;
		return 1;
	case GXEMUL_FB_KEY_F12:
		keybuffer_push0(keybuffer,FUNCTION_KEYS | 12 );
		buf = count = 0;
		return 1;
	default:
		keybuffer_push0(keybuffer, buf & 0xff );
		keybuffer_push0(keybuffer, (buf >> 8)  & 0xff);
		keybuffer_push0(keybuffer, (buf >> 16) & 0xff);
		keybuffer_push0(keybuffer, (buf >> 24) & 0xff);
		buf = count = 0;
		return 1;
	}

	return 1;
}


/** Initializes keyboard handler. */
int kbd_arch_init(void)
{
	fb = (sysinfo_value("fb.kind") == 1);
	gxemul_cmds[0].addr = (void *) sysinfo_value("kbd.address.virtual");
	ipc_register_irq(sysinfo_value("kbd.inr"), sysinfo_value("kbd.devno"), 0, &gxemul_kbd);
	return 0;
}


/** Process data sent when a key is pressed.
 *  
 *  @param keybuffer Buffer of pressed keys.
 *  @param call      IPC call.
 *
 *  @return Always 1.
 */
int kbd_arch_process(keybuffer_t *keybuffer, ipc_call_t *call) 
{
	int scan_code = IPC_GET_ARG2(*call);

	if (fb) {
		return gxemul_kbd_process_fb(keybuffer, scan_code);
	} else {
		return gxemul_kbd_process_no_fb(keybuffer, scan_code);
	}

}

/** @}
 */
