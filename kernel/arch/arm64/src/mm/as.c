/*
 * Copyright (c) 2015 Petr Pavlu
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

/** @addtogroup kernel_arm64_mm
 * @{
 */
/** @file
 * @brief Address space functions.
 */

#include <arch/mm/as.h>
#include <arch/regutils.h>
#include <genarch/mm/asid_fifo.h>
#include <genarch/mm/page_pt.h>
#include <mm/as.h>
#include <mm/asid.h>

/** Architecture dependent address space init.
 *
 * Since ARM64 supports page tables, #as_pt_operations are used.
 */
void as_arch_init(void)
{
	as_operations = &as_pt_operations;
	asid_fifo_init();
}

/** Perform ARM64-specific tasks when an address space becomes active on the
 * processor.
 *
 * Change the level 0 page table (this is normally done by
 * SET_PTL0_ADDRESS_ARCH() on other architectures) and install ASID.
 *
 * @param as Address space.
 */
void as_install_arch(as_t *as)
{
	uint64_t val;

	val = (uint64_t) as->genarch.page_table;
	if (as->asid != ASID_KERNEL) {
		val |= (uint64_t) as->asid << TTBR0_ASID_SHIFT;
		TTBR0_EL1_write(val);
	} else
		TTBR1_EL1_write(val);
}

/** @}
 */
