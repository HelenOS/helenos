/*
 * Copyright (c) 2019 Jiri Svoboda
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

/** @addtogroup display
 * @{
 */
/**
 * @file Display server output
 */

#include <errno.h>
#include <gfx/context.h>
#include <guigfx/canvas.h>
#include <stdio.h>
#include <stdlib.h>
#include <window.h>
#include "output.h"

errno_t output_init(gfx_context_t **rgc)
{
	canvas_gc_t *cgc = NULL;
	window_t *window = NULL;
	pixel_t *pixbuf = NULL;
	surface_t *surface = NULL;
	canvas_t *canvas = NULL;
	gfx_coord_t vw, vh;
	errno_t rc;

	printf("Init canvas..\n");

	window = window_open("comp:0/winreg", NULL,
	    WINDOW_MAIN | WINDOW_DECORATED, "Display Server");
	if (window == NULL) {
		printf("Error creating window.\n");
		return -1;
	}

	vw = 800;
	vh = 600;

	pixbuf = calloc(vw * vh, sizeof(pixel_t));
	if (pixbuf == NULL) {
		printf("Error allocating memory for pixel buffer.\n");
		return ENOMEM;
	}

	surface = surface_create(vw, vh, pixbuf, 0);
	if (surface == NULL) {
		printf("Error creating surface.\n");
		return EIO;
	}

	canvas = create_canvas(window_root(window), NULL, vw, vh,
	    surface);
	if (canvas == NULL) {
		printf("Error creating canvas.\n");
		return EIO;
	}

	window_resize(window, 0, 0, vw + 10, vh + 30, WINDOW_PLACEMENT_ANY);
	window_exec(window);

	printf("Create canvas GC\n");
	rc = canvas_gc_create(canvas, surface, &cgc);
	if (rc != EOK)
		return rc;

	*rgc = canvas_gc_get_ctx(cgc);
	return EOK;
}

/** @}
 */
