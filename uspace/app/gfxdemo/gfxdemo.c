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

/** @addtogroup gfxdemo
 * @{
 */
/** @file Graphic demo
 */

#include <fibril.h>
#include <gfx/backend/console.h>
#include <gfx/color.h>
#include <gfx/render.h>
#include <io/console.h>
#include <stdlib.h>

int main(int argc, char *argv[])
{
	console_ctrl_t *con = NULL;
	gfx_color_t *color = NULL;
	console_gc_t *cgc = NULL;
	gfx_context_t *gc;
	gfx_rect_t rect;
	int i;
	errno_t rc;

	printf("Init console..\n");
	con = console_init(stdin, stdout);
	if (con == NULL)
		return 1;

	printf("Create console GC\n");
	rc = console_gc_create(con, stdout, &cgc);
	if (rc != EOK)
		return 1;

	gc = console_gc_get_ctx(cgc);

	while (true) {
		rc = gfx_color_new_rgb_i16(rand() % 0x10000, rand() % 0x10000,
		    rand() % 0x10000, &color);
		if (rc != EOK)
			return 1;

		rc = gfx_set_color(gc, color);
		if (rc != EOK)
			return 1;

		for (i = 0; i < 10; i++) {
			rect.p0.x = rand() % 79;
			rect.p0.y = rand() % 24;
			rect.p1.x = rect.p0.x + rand() % (79 - rect.p0.x);
			rect.p1.y = rect.p0.y + rand() % (24 - rect.p0.y);

			rc = gfx_fill_rect(gc, &rect);
			if (rc != EOK)
				return 1;
		}

		gfx_color_delete(color);

		fibril_usleep(500 * 1000);
	}

	rc = console_gc_delete(cgc);
	if (rc != EOK)
		return 1;

	return 0;
}

/** @}
 */
