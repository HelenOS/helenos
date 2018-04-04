/*
 * Copyright (c) 2009 Jakub Jermar
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

/** @file
 * @ingroup libsparc64
 */

#ifndef LIBC_sparc64_DDI_H_
#define LIBC_sparc64_DDI_H_

#include <ddi.h>

static inline void memory_barrier(void)
{
	asm volatile (
	    "membar #LoadLoad | #StoreStore\n"
	    ::: "memory"
	);
}

static inline void arch_pio_write_8(ioport8_t *port, uint8_t v)
{
	*port = v;
	memory_barrier();
}

static inline void arch_pio_write_16(ioport16_t *port, uint16_t v)
{
	*port = v;
	memory_barrier();
}

static inline void arch_pio_write_32(ioport32_t *port, uint32_t v)
{
	*port = v;
	memory_barrier();
}

static inline void arch_pio_write_64(ioport64_t *port, uint64_t v)
{
	*port = v;
	memory_barrier();
}


static inline uint8_t arch_pio_read_8(const ioport8_t *port)
{
	uint8_t rv;

	rv = *port;
	memory_barrier();

	return rv;
}

static inline uint16_t arch_pio_read_16(const ioport16_t *port)
{
	uint16_t rv;

	rv = *port;
	memory_barrier();

	return rv;
}

static inline uint32_t arch_pio_read_32(const ioport32_t *port)
{
	uint32_t rv;

	rv = *port;
	memory_barrier();

	return rv;
}

static inline uint64_t arch_pio_read_64(const ioport64_t *port)
{
	uint64_t rv;

	rv = *port;
	memory_barrier();

	return rv;
}

#endif
