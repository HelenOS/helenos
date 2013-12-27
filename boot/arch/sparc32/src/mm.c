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

/** @addtogroup sparc32boot
 * @{
 */
/** @file
 * @brief Bootstrap.
 */

#include <arch/asm.h>
#include <arch/common.h>
#include <arch/arch.h>
#include <arch/mm.h>
#include <arch/main.h>
#include <arch/_components.h>
#include <halt.h>
#include <printf.h>
#include <memstr.h>
#include <version.h>
#include <macros.h>
#include <align.h>
#include <str.h>
#include <errno.h>
#include <inflate.h>

#define OFF2SEC(_addr)  ((_addr) >> PTL0_SHIFT)
#define SEC2OFF(_sec)   ((_sec) << PTL0_SHIFT)

static section_mapping_t mappings[] = {
	{ 0x40000000, 0x3fffffff, 0x40000000, 1 },
	{ 0x40000000, 0x2fffffff, 0x80000000, 1 },
	{ 0x80000000, 0x0fffffff, 0xb0000000, 0 },
	{ 0, 0, 0, 0 },
};

static void mmu_enable(void)
{
	boot_ctx_table = ((uintptr_t) &boot_pt[0] >> 4) | PTE_ET_DESCRIPTOR;
	
	/* Set Context Table Pointer register */
	asi_u32_write(ASI_MMUREGS, 0x100, ((uint32_t) &boot_ctx_table) >> 4);
	
	/* Select context 0 */
	asi_u32_write(ASI_MMUREGS, 0x200, 0);
	
	/* Enable MMU */
	uint32_t cr = asi_u32_read(ASI_MMUREGS, 0x000);
	cr |= 1;
	asi_u32_write(ASI_MMUREGS, 0x000, cr);
}

static void mmu_disable()
{
	uint32_t cr = asi_u32_read(ASI_MMUREGS, 0x000);
	cr &= ~1;
	asi_u32_write(ASI_MMUREGS, 0x000, cr);
}

void mmu_init(void)
{
	mmu_disable();
	
	for (unsigned int i = 0; mappings[i].size != 0; i++) {
		unsigned int ptr = 0;
		for (uint32_t sec = OFF2SEC(mappings[i].va);
		    sec < OFF2SEC(mappings[i].va + mappings[i].size);
		    sec++) {
			boot_pt[sec].ppn = ((mappings[i].pa + SEC2OFF(ptr++)) >> 12) & 0xffffff;
			boot_pt[sec].cacheable = mappings[i].cacheable;
			boot_pt[sec].acc = PTE_ACC_RWX;
			boot_pt[sec].et = PTE_ET_ENTRY;
		}
	}
	
	mmu_enable();
}
