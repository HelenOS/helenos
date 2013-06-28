/*
 * Copyright (c) 2012 Petr Koupy
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

/** @addtogroup draw
 * @{
 */
/**
 * @file
 */

#ifndef DRAW_DRAWCTX_H_
#define DRAW_DRAWCTX_H_

#include <sys/types.h>
#include <stdbool.h>

#include <compose.h>

#include "surface.h"
#include "source.h"
#include "path.h"
#include "font.h"

struct drawctx {
	link_t link;
	list_t list;

	surface_t *surface;
	compose_t compose;
	surface_t *mask;
	source_t *source;
	font_t *font;

	bool shall_clip;
	sysarg_t clip_x;
	sysarg_t clip_y;
	sysarg_t clip_width;
	sysarg_t clip_height;
};

extern void drawctx_init(drawctx_t *, surface_t *);

extern void drawctx_save(drawctx_t *);
extern void drawctx_restore(drawctx_t *);

extern void drawctx_set_compose(drawctx_t *, compose_t);
extern void drawctx_set_clip(drawctx_t *, sysarg_t, sysarg_t, sysarg_t, sysarg_t);
extern void drawctx_set_mask(drawctx_t *, surface_t *);
extern void drawctx_set_source(drawctx_t *, source_t *);
extern void drawctx_set_font(drawctx_t *, font_t *);

extern void drawctx_transfer(drawctx_t *, sysarg_t, sysarg_t, sysarg_t, sysarg_t);
extern void drawctx_stroke(drawctx_t *, path_t *);
extern void drawctx_fill(drawctx_t *, path_t *);
extern void drawctx_print(drawctx_t *, const char *, sysarg_t, sysarg_t);

#endif

/** @}
 */
