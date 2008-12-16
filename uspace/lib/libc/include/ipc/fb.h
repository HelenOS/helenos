/*
 * Copyright (c) 2006 Ondrej Palkovsky
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

/** @addtogroup libcipc
 * @{
 */
/** @file
 */ 

#ifndef LIBC_FB_H_
#define LIBC_FB_H_

#include <ipc/ipc.h>

typedef enum {
	FB_PUTCHAR = IPC_FIRST_USER_METHOD,
	FB_CLEAR,
	FB_GET_CSIZE,
	FB_CURSOR_VISIBILITY,
	FB_CURSOR_GOTO,
	FB_SCROLL,
	FB_VIEWPORT_SWITCH,
	FB_VIEWPORT_CREATE,
	FB_VIEWPORT_DELETE,
	FB_SET_STYLE,
	FB_GET_RESOLUTION,
	FB_DRAW_TEXT_DATA,
	FB_FLUSH,
	FB_DRAW_PPM,
	FB_PREPARE_SHM,
	FB_DROP_SHM,
	FB_SHM2PIXMAP,
	FB_VP_DRAW_PIXMAP,
	FB_VP2PIXMAP,
	FB_DROP_PIXMAP,
	FB_ANIM_CREATE,
	FB_ANIM_DROP,
	FB_ANIM_ADDPIXMAP,
	FB_ANIM_CHGVP,
	FB_ANIM_START,
	FB_ANIM_STOP,
	FB_POINTER_MOVE
} fb_request_t;


#endif

/** @}
 */
