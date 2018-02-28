/*
 * Copyright (c) 2006 Martin Decky
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

/** @addtogroup genericmm
 * @{
 */

/**
 * @file
 * @brief RAM disk support.
 *
 * Support for RAM disk images.
 */

#include <assert.h>
#include <log.h>
#include <lib/rd.h>
#include <mm/frame.h>
#include <sysinfo/sysinfo.h>
#include <ddi/ddi.h>

/** Physical memory area for RAM disk. */
static parea_t rd_parea;

/** RAM disk initialization routine
 *
 * The information about the RAM disk is provided as sysinfo
 * values to the uspace tasks.
 *
 */
void init_rd(void *data, size_t size)
{
	uintptr_t base = (uintptr_t) data;
	assert((base % FRAME_SIZE) == 0);

	rd_parea.pbase = base;
	rd_parea.frames = SIZE2FRAMES(size);
	rd_parea.unpriv = false;
	rd_parea.mapped = false;
	ddi_parea_register(&rd_parea);

	sysinfo_set_item_val("rd", NULL, true);
	sysinfo_set_item_val("rd.size", NULL, size);
	sysinfo_set_item_val("rd.address.physical", NULL, (sysarg_t) base);

	log(LF_OTHER, LVL_NOTE, "RAM disk at %p (size %zu bytes)", (void *) base,
	    size);
}

/** @}
 */
