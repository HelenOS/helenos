/*
 * Copyright (c) 2005 Jakub Jermar
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

/** @addtogroup kernel_sparc64_interrupt
 * @{
 */
/**
 * @file
 * @brief This file contains level N interrupt and inter-processor interrupt
 * trap handler.
 */
#ifndef KERN_sparc64_INTERRUPT_TRAP_H_
#define KERN_sparc64_INTERRUPT_TRAP_H_

#define TT_INTERRUPT_LEVEL_1			0x41
#define TT_INTERRUPT_LEVEL_2			0x42
#define TT_INTERRUPT_LEVEL_3			0x43
#define TT_INTERRUPT_LEVEL_4			0x44
#define TT_INTERRUPT_LEVEL_5			0x45
#define TT_INTERRUPT_LEVEL_6			0x46
#define TT_INTERRUPT_LEVEL_7			0x47
#define TT_INTERRUPT_LEVEL_8			0x48
#define TT_INTERRUPT_LEVEL_9			0x49
#define TT_INTERRUPT_LEVEL_10			0x4a
#define TT_INTERRUPT_LEVEL_11			0x4b
#define TT_INTERRUPT_LEVEL_12			0x4c
#define TT_INTERRUPT_LEVEL_13			0x4d
#define TT_INTERRUPT_LEVEL_14			0x4e
#define TT_INTERRUPT_LEVEL_15			0x4f

#define INTERRUPT_LEVEL_N_HANDLER_SIZE		TRAP_TABLE_ENTRY_SIZE

/* IMAP register bits */
#define IGN_MASK	0x7c0
#define INO_MASK	0x1f
#define IMAP_V_MASK	(1ULL << 31)

#define IGN_SHIFT	6

#ifndef __ASSEMBLER__

#include <arch/interrupt.h>

extern void interrupt(unsigned int n, istate_t *istate);

#endif /* !def __ASSEMBLER__ */

#if defined (SUN4U)
#include <arch/trap/sun4u/interrupt.h>
#elif defined (SUN4V)
#include <arch/trap/sun4v/interrupt.h>
#endif

#endif

/** @}
 */
