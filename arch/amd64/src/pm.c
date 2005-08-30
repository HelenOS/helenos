/*
 * Copyright (C) 2001-2004 Jakub Jermar
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

#include <arch/pm.h>
#include <arch/mm/page.h>
#include <arch/types.h>


/*
 * There is no segmentation in long mode so we set up flat mode. In this
 * mode, we use, for each privilege level, two segments spanning the
 * whole memory. One is for code and one is for data.
 */

struct descriptor gdt[GDT_ITEMS] = {
	/* NULL descriptor */
	{ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
	/* KTEXT descriptor */
	{ .limit_0_15  = 0xffff, 
	  .base_0_15   = 0, 
	  .base_16_23  = 0, 
	  .access      = AR_PRESENT | AR_CODE | DPL_KERNEL | AR_READABLE , 
	  .limit_16_19 = 0xf, 
	  .available   = 0, 
	  .longmode    = 1, 
	  .special     = 0,
	  .granularity = 1, 
	  .base_24_31  = 0 },
	/* KDATA descriptor */
	{ .limit_0_15  = 0xffff, 
	  .base_0_15   = 0, 
	  .base_16_23  = 0, 
	  .access      = AR_PRESENT | AR_DATA | AR_WRITABLE | DPL_KERNEL, 
	  .limit_16_19 = 0xf, 
	  .available   = 0, 
	  .longmode    = 0, 
	  .special     = 0, 
	  .granularity = 1, 
	  .base_24_31  = 0 },
	/* UTEXT descriptor */
	{ .limit_0_15  = 0xffff, 
	  .base_0_15   = 0, 
	  .base_16_23  = 0, 
	  .access      = AR_PRESENT | AR_CODE | DPL_USER, 
	  .limit_16_19 = 0xf, 
	  .available   = 0, 
	  .longmode    = 1, 
	  .special     = 0, 
	  .granularity = 0, 
	  .base_24_31  = 0 },
	/* UDATA descriptor */
	{ .limit_0_15  = 0xffff, 
	  .base_0_15   = 0, 
	  .base_16_23  = 0, 
	  .access      = AR_PRESENT | AR_DATA | AR_WRITABLE | DPL_USER, 
	  .limit_16_19 = 0xf, 
	  .available   = 0, 
	  .longmode    = 0, 
	  .special     = 1, 
	  .granularity = 1, 
	  .base_24_31  = 0 },
	/* KTEXT 16-bit protected */
	{ .limit_0_15  = 0xffff, 
	  .base_0_15   = 0, 
	  .base_16_23  = 0, 
	  .access      = AR_PRESENT | AR_CODE | DPL_KERNEL | AR_READABLE, 
	  .limit_16_19 = 0xf, 
	  .available   = 0, 
	  .longmode    = 0, 
	  .special     = 0,
	  .granularity = 1, 
	  .base_24_31  = 0 },
	/* TSS descriptor - set up will be completed later */
	{ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 }
};

struct idescriptor idt[IDT_ITEMS];

static struct tss tss;

/* Does not compile correctly if it does not exist */
int __attribute__ ((section ("K_DATA_START"))) __fake;
