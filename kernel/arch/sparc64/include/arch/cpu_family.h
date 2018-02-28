/*
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

/** @addtogroup sparc64
 * @{
 */
/** @file
 */

#ifndef KERN_sparc64_CPU_FAMILY_H_
#define KERN_sparc64_CPU_FAMILY_H_

#include <arch.h>
#include <cpu.h>
#include <arch/register.h>
#include <arch/asm.h>

/**
 * Find the processor (sub)family.
 *
 * @return 	true iff the CPU belongs to the US family
 */
static inline bool is_us(void)
{
	int impl = ((ver_reg_t) ver_read()).impl;
	return (impl == IMPL_ULTRASPARCI) || (impl == IMPL_ULTRASPARCII) ||
	       (impl == IMPL_ULTRASPARCII_I) ||  (impl == IMPL_ULTRASPARCII_E);
}

/**
 * Find the processor (sub)family.
 *
 * @return 	true iff the CPU belongs to the US-III subfamily
 */
static inline bool is_us_iii(void)
{
	int impl = ((ver_reg_t) ver_read()).impl;
	return (impl == IMPL_ULTRASPARCIII) ||
	       (impl == IMPL_ULTRASPARCIII_PLUS) ||
	       (impl == IMPL_ULTRASPARCIII_I);
}

/**
 * Find the processor (sub)family.
 *
 * @return 	true iff the CPU belongs to the US-IV subfamily
 */
static inline bool is_us_iv(void)
{
	int impl = ((ver_reg_t) ver_read()).impl;
	return (impl == IMPL_ULTRASPARCIV) || (impl == IMPL_ULTRASPARCIV_PLUS);
}

#endif

/** @}
 */

