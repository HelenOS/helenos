/*
 * Copyright (c) 2006 Ondrej Palkovsky
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
 * @ingroup libcia32, libcamd64
 */

#ifndef LIBC_ia32_DDI_H_
#define LIBC_ia32_DDI_H_

#include <ddi.h>

#define IO_SPACE_BOUNDARY  ((void *) (64 * 1024))

static inline uint8_t arch_pio_read_8(const ioport8_t *port)
{
	if (port < (ioport8_t *) IO_SPACE_BOUNDARY) {
		uint8_t val;

		asm volatile (
		    "inb %w[port], %b[val]\n"
		    : [val] "=a" (val)
		    : [port] "d" (port)
		);

		return val;
	} else
		return (uint8_t) *port;
}

static inline uint16_t arch_pio_read_16(const ioport16_t *port)
{
	if (port < (ioport16_t *) IO_SPACE_BOUNDARY) {
		uint16_t val;

		asm volatile (
		    "inw %w[port], %w[val]\n"
		    : [val] "=a" (val)
		    : [port] "d" (port)
		);

		return val;
	} else
		return (uint16_t) *port;
}

static inline uint32_t arch_pio_read_32(const ioport32_t *port)
{
	if (port < (ioport32_t *) IO_SPACE_BOUNDARY) {
		uint32_t val;

		asm volatile (
		    "inl %w[port], %[val]\n"
		    : [val] "=a" (val)
		    : [port] "d" (port)
		);

		return val;
	} else
		return (uint32_t) *port;
}

static inline uint64_t arch_pio_read_64(const ioport64_t *port)
{
	return (uint64_t) *port;
}

static inline void arch_pio_write_8(ioport8_t *port, uint8_t val)
{
	if (port < (ioport8_t *) IO_SPACE_BOUNDARY) {
		asm volatile (
		    "outb %b[val], %w[port]\n"
		    :: [val] "a" (val), [port] "d" (port)
		);
	} else
		*port = val;
}

static inline void arch_pio_write_16(ioport16_t *port, uint16_t val)
{
	if (port < (ioport16_t *) IO_SPACE_BOUNDARY) {
		asm volatile (
		    "outw %w[val], %w[port]\n"
		    :: [val] "a" (val), [port] "d" (port)
		);
	} else
		*port = val;
}

static inline void arch_pio_write_32(ioport32_t *port, uint32_t val)
{
	if (port < (ioport32_t *) IO_SPACE_BOUNDARY) {
		asm volatile (
		    "outl %[val], %w[port]\n"
		    :: [val] "a" (val), [port] "d" (port)
		);
	} else
		*port = val;
}

static inline void arch_pio_write_64(ioport64_t *port, uint64_t val)
{
	*port = val;
}

#endif
