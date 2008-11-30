/*
 * Copyright (c) 2005 Jakub Jermar, Jakub Vana
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

/** @addtogroup ia64	
 * @{
 */
/** @file
 */

#ifndef LIBC_ia64_DDI_H_
#define LIBC_ia64_DDI_H_

#include <libarch/types.h>

typedef uint64_t ioport_t;

uint64_t get_ia64_iospace_address(void);

extern uint64_t ia64_iospace_address;

#define IA64_IOSPACE_ADDRESS (ia64_iospace_address?ia64_iospace_address:(ia64_iospace_address=get_ia64_iospace_address()))

static inline void  outb(ioport_t port,uint8_t v)
{
	*((uint8_t *)(IA64_IOSPACE_ADDRESS + ( (port & 0xfff) | ( (port >> 2) << 12 )))) = v;

	asm volatile ("mf\n" ::: "memory");
}

static inline void  outw(ioport_t port,uint16_t v)
{
	*((uint16_t *)(IA64_IOSPACE_ADDRESS + ( (port & 0xfff) | ( (port >> 2) << 12 )))) = v;

	asm volatile ("mf\n" ::: "memory");
}

static inline void  outl(ioport_t port,uint32_t v)
{
	*((uint32_t *)(IA64_IOSPACE_ADDRESS + ( (port & 0xfff) | ( (port >> 2) << 12 )))) = v;

	asm volatile ("mf\n" ::: "memory");
}



static inline uint8_t inb(ioport_t port)
{
	asm volatile ("mf\n" ::: "memory");

	return *((uint8_t *)(IA64_IOSPACE_ADDRESS + ( (port & 0xfff) | ( (port >> 2) << 12 ))));
}

static inline uint16_t inw(ioport_t port)
{
	asm volatile ("mf\n" ::: "memory");

	return *((uint16_t *)(IA64_IOSPACE_ADDRESS + ( (port & 0xffE) | ( (port >> 2) << 12 ))));
}

static inline uint32_t inl(ioport_t port)
{
	asm volatile ("mf\n" ::: "memory");

	return *((uint32_t *)(IA64_IOSPACE_ADDRESS + ( (port & 0xfff) | ( (port >> 2) << 12 ))));
}




#endif

/** @}
 */
