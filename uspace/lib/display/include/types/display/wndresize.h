/*
 * Copyright (c) 2020 Jiri Svoboda
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

/** @addtogroup libdisplay
 * @{
 */
/** @file
 */

#ifndef _LIBDISPLAY_TYPES_DISPLAY_WNDRESIZE_H_
#define _LIBDISPLAY_TYPES_DISPLAY_WNDRESIZE_H_

/** Window resize type */
typedef enum {
	display_wr_top = 0x1,
	display_wr_left = 0x2,
	display_wr_bottom = 0x4,
	display_wr_right = 0x8,

	display_wr_top_left = display_wr_top | display_wr_left,
	display_wr_bottom_left = display_wr_bottom | display_wr_left,
	display_wr_bottom_right = display_wr_bottom | display_wr_right,
	display_wr_top_right = display_wr_top | display_wr_right
} display_wnd_rsztype_t;

#endif

/** @}
 */
