/*
 * Copyright (c) 2006 Jakub Jermar
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

/** @addtogroup ppc32mm
 * @{
 */
/** @file
 */

#include <arch/mm/as.h>
#include <genarch/mm/as_pt.h>
#include <genarch/mm/page_pt.h>
#include <genarch/mm/asid_fifo.h>
#include <arch.h>

/** Architecture dependent address space init. */
void as_arch_init(void)
{
	as_operations = &as_pt_operations;
	asid_fifo_init();
}

/** Install address space.
 *
 * Install ASID.
 *
 * @param as Address space structure.
 *
 */
void as_install_arch(as_t *as)
{
	uint32_t sr;

	/* Lower 2 GB, user and supervisor access */
	for (sr = 0; sr < 8; sr++)
		sr_set(0x6000, as->asid, sr);

	/* Upper 2 GB, only supervisor access */
	for (sr = 8; sr < 16; sr++)
		sr_set(0x4000, as->asid, sr);
}

/** @}
 */
