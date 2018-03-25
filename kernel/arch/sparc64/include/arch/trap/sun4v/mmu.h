/*
 * Copyright (c) 2006 Jakub Jermar
 * Copyright (c) 2008 Pavel Rimsky
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

/** @addtogroup sparc64interrupt
 * @{
 */
/**
 * @file
 * @brief This file contains fast MMU trap handlers.
 */

#ifndef KERN_sparc64_sun4v_MMU_TRAP_H_
#define KERN_sparc64_sun4v_MMU_TRAP_H_

#include <arch/stack.h>
#include <arch/regdef.h>
#include <arch/arch.h>
#include <arch/sun4v/arch.h>
#include <arch/sun4v/hypercall.h>
#include <arch/mm/sun4v/mmu.h>
#include <arch/mm/tlb.h>
#include <arch/mm/mmu.h>
#include <arch/mm/tte.h>
#include <arch/trap/regwin.h>

#ifdef CONFIG_TSB
#include <arch/mm/tsb.h>
#endif

#define TT_FAST_INSTRUCTION_ACCESS_MMU_MISS	0x64
#define TT_FAST_DATA_ACCESS_MMU_MISS		0x68
#define TT_FAST_DATA_ACCESS_PROTECTION		0x6c
#define TT_CPU_MONDO				0x7c

#define FAST_MMU_HANDLER_SIZE			128

#ifdef __ASSEMBLER__
#include <arch/trap/sun4v/mmu.S>
#endif /* __ASSEMBLER__ */

#endif

/** @}
 */
