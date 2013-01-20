/*
 * Copyright (c) 2013 Jan Vesely
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

/** @addtogroup amdm37x
 * @{
 */
/**
 * @file
 */

#include <assert.h>
#include <errno.h>

#include "amdm37x_dispc.h"

static int handle_damage(visualizer_t *vs,
    sysarg_t x0, sysarg_t y0, sysarg_t width, sysarg_t height,
    sysarg_t x_offset, sysarg_t y_offset)
{
	return EOK;
}

static const visualizer_ops_t amdm37x_dispc_vis_ops = {
	.handle_damage = handle_damage,
	// TODO DO we need dummy implementations of stuff like claim, yield, ...
};

static void mode_init(vslmode_list_element_t *mode,
    unsigned width, unsigned height, visual_t visual)
{
	mode->mode.index = 0;
	mode->mode.version = 0;
	mode->mode.refresh_rate = 0;
	mode->mode.screen_aspect.width = width;
	mode->mode.screen_aspect.height = height;
	mode->mode.screen_width = width;
	mode->mode.screen_height = height;
	mode->mode.cell_aspect.width = 1;
	mode->mode.cell_aspect.height = 1;
	mode->mode.cell_visual.pixel_visual = visual;

	link_initialize(&mode->link);

}

int amdm37x_dispc_init(amdm37x_dispc_t *instance, visualizer_t *vis)
{
	assert(instance);
	assert(vis);

	unsigned width = CONFIG_BFB_WIDTH;
	unsigned height = CONFIG_BFB_HEIGHT;
	unsigned bpp = CONFIG_BFB_BPP;

	mode_init(&instance->modes[0], width, height, bpp); //TODO convert bpp to visual

	/* Handle vis stuff */
	vis->dev_ctx = instance;
	vis->def_mode_idx = 0;
	list_append(&instance->modes[0].link, &vis->modes);

	return EOK;
};

int amdm37x_dispc_fini(amdm37x_dispc_t *instance)
{
	return EOK;
};

