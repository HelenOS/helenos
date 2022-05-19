/*
 * Copyright (c) 2022 Jiri Svoboda
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

/** @addtogroup libui
 * @{
 */
/**
 * @file Push button
 */

#ifndef _UI_TYPES_PBUTTON_H
#define _UI_TYPES_PBUTTON_H

#include <errno.h>
#include <gfx/coord.h>

struct ui_pbutton;
typedef struct ui_pbutton ui_pbutton_t;

/** UI push button flags */
typedef enum {
	/** Do not depress the button in text mode */
	ui_pbf_no_text_depress = 0x1
} ui_pbutton_flags_t;

/** Push button callbacks */
typedef struct ui_pbutton_cb {
	void (*clicked)(ui_pbutton_t *, void *);
	void (*down)(ui_pbutton_t *, void *);
	void (*up)(ui_pbutton_t *, void *);
} ui_pbutton_cb_t;

/** Push button decoration ops */
typedef struct ui_pbutton_decor_ops {
	errno_t (*paint)(ui_pbutton_t *, void *, gfx_coord2_t *);
} ui_pbutton_decor_ops_t;

#endif

/** @}
 */
