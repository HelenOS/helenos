/*
 * Copyright (c) 2013 Jakub Klama
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

/** @addtogroup sparc32mm
 * @{
 */

#include <mm/as.h>
#include <arch/arch.h>
#include <arch/asm.h>
#include <arch/mm/as.h>
#include <arch/mm/page.h>
#include <genarch/mm/page_pt.h>

uintptr_t as_context_table;

static ptd_t context_table[ASID_MAX_ARCH] __attribute__((aligned(1024)));

void as_arch_init(void)
{
	as_operations = &as_pt_operations;
	as_context_table = (uintptr_t) &context_table;
}

void as_install_arch(as_t *as)
{
	context_table[as->asid].table_pointer =
	    (uintptr_t) as->genarch.page_table >> 6;
	context_table[as->asid].et = PTE_ET_DESCRIPTOR;
	asi_u32_write(ASI_MMUREGS, 0x200, as->asid);
	asi_u32_write(ASI_MMUCACHE, 0, 1);
	asi_u32_write(ASI_MMUFLUSH, 0x400, 1);
}

/** @}
 */
