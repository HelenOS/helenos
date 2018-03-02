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

#include <assert.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>

#include "../gfx/cursor-11x18.h"
#include "embedded.h"
#include "../surface.h"

static void cde_init(char *path, uint8_t *state_count, void **data)
{
	assert(state_count);
	assert(data);

	(*state_count) = 1;
	(*data) = NULL;
}

static surface_t *cde_render(uint8_t state)
{
	if (state != 0)
		return NULL;

	surface_t *surface = surface_create(CURSOR_WIDTH, CURSOR_HEIGHT, NULL, 0);
	if (!surface)
		return NULL;

	for (unsigned int y = 0; y < CURSOR_HEIGHT; ++y) {
		for (unsigned int x = 0; x < CURSOR_WIDTH; ++x) {
			size_t offset = y * ((CURSOR_WIDTH - 1) / 8 + 1) + x / 8;
			bool visible = cursor_mask[offset] & (1 << (x % 8));
			pixel_t pixel = (cursor_texture[offset] & (1 << (x % 8))) ?
			    PIXEL(255, 0, 0, 0) : PIXEL(255, 255, 255, 255);

			if (visible)
				surface_put_pixel(surface, x, y, pixel);
		}
	}

	return surface;
}

static void cde_release(void *data)
{
	/* no-op */
}

cursor_decoder_t cd_embedded = {
	.init = cde_init,
	.render = cde_render,
	.release = cde_release
};

/** @}
 */
