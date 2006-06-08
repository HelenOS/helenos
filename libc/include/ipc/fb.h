/*
 * Copyright (C) 2006 Ondrej Palkovsky
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

#include <arch/types.h>
#include <types.h>

#ifndef __libc__FB_H__
#define __libc__FB_H__

#define FB_PUTCHAR           1025
#define FB_CLEAR             1026
#define FB_GET_CSIZE         1027
#define FB_CURSOR_VISIBILITY 1028
#define FB_CURSOR_GOTO       1029
#define FB_SCROLL            1030
#define FB_VIEWPORT_SWITCH   1031
#define FB_VIEWPORT_CREATE   1032
#define FB_VIEWPORT_DELETE   1033
#define FB_SET_STYLE         1034
#define FB_GET_RESOLUTION    1035
#define FB_DRAW_TEXT_DATA    1036
#define FB_FLUSH             1037
#define FB_VIEWPORT_DB       1038

#define FB_DRAW_PPM          1100
#define FB_PREPARE_SHM       1101
#define FB_DROP_SHM          1102
#define FB_SHM2PIXMAP        1103

#define FB_VP_DRAW_PIXMAP    1104
#define FB_VP2PIXMAP         1105
#define FB_DROP_PIXMAP       1106
#define FB_TRANS_PUTCHAR     1107

#define FB_ANIM_CREATE       1200
#define FB_ANIM_DROP         1201
#define FB_ANIM_ADDPIXMAP    1202
#define FB_ANIM_CHGVP        1203
#define FB_ANIM_START        1204
#define FB_ANIM_STOP         1205

#endif


 /** @}
 */
 
 
