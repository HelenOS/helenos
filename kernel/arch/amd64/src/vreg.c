/*
 * Copyright (c) 2016 Jakub Jermar
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

/** @addtogroup kernel_amd64
 * @{
 */
/** @file
 */

#include <arch/vreg.h>
#include <arch/pm.h>
#include <config.h>
#include <typedefs.h>
#include <arch/asm.h>
#include <panic.h>
#include <mm/km.h>
#include <mm/frame.h>

/*
 * During initialization, we need to make sure that context_save() and
 * context_restore() touch some meaningful address when saving/restoring VREGs.
 * When a processor is initialized, we set its FS base to a private page and
 * vreg_ptr to zero.
 */
static uint64_t vreg_tp_dummy;
uint64_t *vreg_ptr = &vreg_tp_dummy;

/**
 * Allocate and initialize a per-CPU user page to be accessible via the FS
 * segment register and to hold the virtual registers.
 */
void vreg_init(void)
{
	uintptr_t frame;
	uint64_t *page;

	frame = frame_alloc(1, FRAME_ATOMIC | FRAME_HIGHMEM, 0);
	if (!frame)
		panic("Cannot allocate VREG frame.");

	page = (uint64_t *) km_map(frame, PAGE_SIZE, PAGE_SIZE,
	    PAGE_READ | PAGE_WRITE | PAGE_USER | PAGE_CACHEABLE);

	write_msr(AMD_MSR_FS, (uintptr_t) page);

	vreg_ptr = NULL;
}

/** @}
 */
