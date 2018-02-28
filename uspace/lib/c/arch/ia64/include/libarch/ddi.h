/*
 * Copyright (c) 2005 Jakub Vana
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

/** @addtogroup libcia64
 * @{
 */
/** @file
 */

#ifndef LIBC_ia64_DDI_H_
#define LIBC_ia64_DDI_H_

#include <ddi.h>

#define IO_SPACE_BOUNDARY	((void *) (64 * 1024))

uint64_t get_ia64_iospace_address(void);

extern uint64_t ia64_iospace_address;

#define IA64_IOSPACE_ADDRESS \
	(ia64_iospace_address ? \
	    ia64_iospace_address : \
	    (ia64_iospace_address = get_ia64_iospace_address()))

static inline void arch_pio_write_8(ioport8_t *port, uint8_t v)
{
	if (port < (ioport8_t *) IO_SPACE_BOUNDARY) {
		uintptr_t prt = (uintptr_t) port;

		*((ioport8_t *)(IA64_IOSPACE_ADDRESS +
		    ((prt & 0xfff) | ((prt >> 2) << 12)))) = v;
	} else {
		*port = v;
	}

	asm volatile ("mf\n" ::: "memory");
	asm volatile ("mf.a\n" ::: "memory");
}

static inline void arch_pio_write_16(ioport16_t *port, uint16_t v)
{
	if (port < (ioport16_t *) IO_SPACE_BOUNDARY) {
		uintptr_t prt = (uintptr_t) port;

		*((ioport16_t *)(IA64_IOSPACE_ADDRESS +
		    ((prt & 0xfff) | ((prt >> 2) << 12)))) = v;
	} else {
		*port = v;
	}

	asm volatile ("mf\n" ::: "memory");
	asm volatile ("mf.a\n" ::: "memory");
}

static inline void arch_pio_write_32(ioport32_t *port, uint32_t v)
{
	if (port < (ioport32_t *) IO_SPACE_BOUNDARY) {
		uintptr_t prt = (uintptr_t) port;

		*((ioport32_t *)(IA64_IOSPACE_ADDRESS +
		    ((prt & 0xfff) | ((prt >> 2) << 12)))) = v;
	} else {
		*port = v;
	}

	asm volatile ("mf\n" ::: "memory");
	asm volatile ("mf.a\n" ::: "memory");
}

static inline void arch_pio_write_64(ioport64_t *port, uint64_t v)
{
	*port = v;

	asm volatile ("mf\n" ::: "memory");
	asm volatile ("mf.a\n" ::: "memory");
}

static inline uint8_t arch_pio_read_8(const ioport8_t *port)
{
	uint8_t v;

	asm volatile ("mf\n" ::: "memory");

	if (port < (ioport8_t *) IO_SPACE_BOUNDARY) {
		uintptr_t prt = (uintptr_t) port;

		v = *((ioport8_t *)(IA64_IOSPACE_ADDRESS +
		    ((prt & 0xfff) | ((prt >> 2) << 12))));
	} else {
		v = *port;
	}

	asm volatile ("mf.a\n" ::: "memory");

	return v;
}

static inline uint16_t arch_pio_read_16(const ioport16_t *port)
{
	uint16_t v;

	asm volatile ("mf\n" ::: "memory");

	if (port < (ioport16_t *) IO_SPACE_BOUNDARY) {
		uintptr_t prt = (uintptr_t) port;

		v = *((ioport16_t *)(IA64_IOSPACE_ADDRESS +
		    ((prt & 0xfff) | ((prt >> 2) << 12))));
	} else {
		v = *port;
	}

	asm volatile ("mf.a\n" ::: "memory");

	return v;
}

static inline uint32_t arch_pio_read_32(const ioport32_t *port)
{
	uint32_t v;

	asm volatile ("mf\n" ::: "memory");

	if (port < (ioport32_t *) IO_SPACE_BOUNDARY) {
		uintptr_t prt = (uintptr_t) port;

		v = *((ioport32_t *)(IA64_IOSPACE_ADDRESS +
		    ((prt & 0xfff) | ((prt >> 2) << 12))));
	} else {
		v = *port;
	}

	asm volatile ("mf.a\n" ::: "memory");

	return v;
}

static inline uint64_t arch_pio_read_64(const ioport64_t *port)
{
	uint64_t v;

	asm volatile ("mf\n" ::: "memory");

	v = *port;

	asm volatile ("mf.a\n" ::: "memory");

	return v;
}

#endif

/** @}
 */
