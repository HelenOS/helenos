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

/** @addtogroup sparc64
 * @{
 */
/** @file
 */

#ifndef KERN_sparc64_CPU_H_
#define KERN_sparc64_CPU_H_

#define MANUF_FUJITSU		0x04
#define MANUF_ULTRASPARC	0x17	/**< UltraSPARC I, UltraSPARC II */
#define MANUF_SUN		0x3e

#define IMPL_ULTRASPARCI	0x10
#define IMPL_ULTRASPARCII	0x11
#define IMPL_ULTRASPARCII_I	0x12
#define IMPL_ULTRASPARCII_E	0x13
#define IMPL_ULTRASPARCIII	0x14
#define IMPL_ULTRASPARCIII_PLUS	0x15
#define IMPL_ULTRASPARCIII_I	0x16
#define IMPL_ULTRASPARCIV	0x18
#define IMPL_ULTRASPARCIV_PLUS	0x19

#define IMPL_SPARC64V		0x5

#ifndef __ASSEMBLER__

#include <arch/register.h>
#include <arch/regdef.h>
#include <arch/asm.h>

#if defined (SUN4U)
#include <arch/sun4u/cpu.h>
#elif defined (SUN4V)
#include <arch/sun4v/cpu.h>
#endif


#endif

#endif

/** @}
 */
