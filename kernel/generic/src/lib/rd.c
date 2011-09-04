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
 * @brief	RAM disk support.
 *
 * Support for RAM disk images.
 */

#include <lib/rd.h>
#include <byteorder.h>
#include <mm/frame.h>
#include <sysinfo/sysinfo.h>
#include <ddi/ddi.h>
#include <align.h>

static parea_t rd_parea;		/**< Physical memory area for rd. */

/**
 * RAM disk initialization routine. At this point, the RAM disk memory is shared
 * and information about the share is provided as sysinfo values to the
 * userspace tasks.
 */  
int init_rd(rd_header_t *header, size_t size)
{
	/* Identify RAM disk */
	if ((header->magic[0] != RD_MAG0) || (header->magic[1] != RD_MAG1) ||
	    (header->magic[2] != RD_MAG2) || (header->magic[3] != RD_MAG3))
		return RE_INVALID;
	
	/* Identify version */	
	if (header->version != RD_VERSION)
		return RE_UNSUPPORTED;
	
	uint32_t hsize;
	uint64_t dsize;
	switch (header->data_type) {
	case RD_DATA_LSB:
		hsize = uint32_t_le2host(header->header_size);
		dsize = uint64_t_le2host(header->data_size);
		break;
	case RD_DATA_MSB:
		hsize = uint32_t_be2host(header->header_size);
		dsize = uint64_t_be2host(header->data_size);
		break;
	default:
		return RE_UNSUPPORTED;
	}
	
	if ((hsize % FRAME_SIZE) || (dsize % FRAME_SIZE))
		return RE_UNSUPPORTED;
		
	if (hsize > size)
		return RE_INVALID;
	
	if ((uint64_t) hsize + dsize > size)
		dsize = size - hsize;
	
	rd_parea.pbase = ALIGN_DOWN((uintptr_t) KA2PA((void *) header + hsize),
	    FRAME_SIZE);
	rd_parea.frames = SIZE2FRAMES(dsize);
	rd_parea.unpriv = false;
	rd_parea.mapped = false;
	ddi_parea_register(&rd_parea);

	sysinfo_set_item_val("rd", NULL, true);
	sysinfo_set_item_val("rd.header_size", NULL, hsize);	
	sysinfo_set_item_val("rd.size", NULL, dsize);
	sysinfo_set_item_val("rd.address.physical", NULL,
	    (sysarg_t) KA2PA((void *) header + hsize));

	return RE_OK;
}

/** @}
 */
