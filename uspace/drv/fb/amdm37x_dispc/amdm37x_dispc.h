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

/** @addtogroup kgraph
 * @{
 */
/**
 * @file
 */

#ifndef AMDM37X_DISPC_H_
#define AMDM37X_DISPC_H_

#include <graph.h>
#include <abi/fb/visuals.h>
#include <pixconv.h>
#include <ddi.h>

#include "amdm37x_dispc_regs.h"

typedef struct {
	amdm37x_dispc_regs_t *regs;

	struct {
		pixel2visual_t pixel2visual;
		unsigned width;
		unsigned height;
		unsigned pitch;
		unsigned bpp;
		unsigned idx;
	} active_fb;

	size_t size;
	void *fb_data;

	vslmode_list_element_t modes[1];
} amdm37x_dispc_t;

errno_t amdm37x_dispc_init(amdm37x_dispc_t *instance, visualizer_t *vis);
errno_t amdm37x_dispc_fini(amdm37x_dispc_t *instance);

#endif
/** @}
 */
