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

static inline void outb(int16_t port, uint8_t b)
{
	asm volatile ("outb %0, %1\n" :: "a" (b), "d" (port));
}

static inline void outw(int16_t port, int16_t w)
{
	asm volatile ("outw %0, %1\n" :: "a" (w), "d" (port));
}

static inline void outl(int16_t port, uint32_t l)
{
	asm volatile ("outl %0, %1\n" :: "a" (l), "d" (port));
}

static inline uint8_t inb(int16_t port)
{
	uint8_t val;

	asm volatile ("inb %1, %0 \n" : "=a" (val) : "d"(port));
	return val;
}

static inline int16_t inw(int16_t port)
{
	int16_t val;

	asm volatile ("inw %1, %0 \n" : "=a" (val) : "d"(port));
	return val;
}

static inline uint32_t inl(int16_t port)
{
	uint32_t val;

	asm volatile ("inl %1, %0 \n" : "=a" (val) : "d"(port));
	return val;
}

#endif
