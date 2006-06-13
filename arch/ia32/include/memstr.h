/*
 * Copyright (C) 2005 Sergey Bondari
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

 /** @addtogroup ia32	
 * @{
 */
/** @file
 */

#ifndef __ia32_MEMSTR_H__
#define __ia32_MEMSTR_H__

/** Copy memory
 *
 * Copy a given number of bytes (3rd argument)
 * from the memory location defined by 2nd argument
 * to the memory location defined by 1st argument.
 * The memory areas cannot overlap.
 *
 * @param dst Destination
 * @param src Source
 * @param cnt Number of bytes
 * @return Destination
 */
static inline void * memcpy(void * dst, const void * src, size_t cnt)
{
        __native d0, d1, d2;

        __asm__ __volatile__(
                /* copy all full dwords */
                "rep movsl\n\t"
                /* load count again */
                "movl %4, %%ecx\n\t"
                /* ecx = ecx mod 4 */
                "andl $3, %%ecx\n\t"
                /* are there last <=3 bytes? */
                "jz 1f\n\t"
                /* copy last <=3 bytes */
                "rep movsb\n\t"
                /* exit from asm block */
                "1:\n"
                : "=&c" (d0), "=&D" (d1), "=&S" (d2)
                : "0" ((__native) (cnt / 4)), "g" ((__native) cnt), "1" ((__native) dst), "2" ((__native) src)
                : "memory");

        return dst;
}


/** Compare memory regions for equality
 *
 * Compare a given number of bytes (3rd argument)
 * at memory locations defined by 1st and 2nd argument
 * for equality. If bytes are equal function returns 0.
 *
 * @param src Region 1
 * @param dst Region 2
 * @param cnt Number of bytes
 * @return Zero if bytes are equal, non-zero otherwise
 */
static inline int memcmp(const void * src, const void * dst, size_t cnt)
{
	__u32 d0, d1, d2;
	int ret;
	
	__asm__ (
		"repe cmpsb\n\t"
		"je 1f\n\t"
		"movl %3, %0\n\t"
		"addl $1, %0\n\t"
		"1:\n"
		: "=a" (ret), "=%S" (d0), "=&D" (d1), "=&c" (d2)
		: "0" (0), "1" ((__native) src), "2" ((__native) dst), "3" ((__native) cnt)
	);
	
	return ret;
}

/** Fill memory with words
 * Fill a given number of words (2nd argument)
 * at memory defined by 1st argument with the
 * word value defined by 3rd argument.
 *
 * @param dst Destination
 * @param cnt Number of words
 * @param x Value to fill
 */
static inline void memsetw(__address dst, size_t cnt, __u16 x)
{
	__u32 d0, d1;
	
	__asm__ __volatile__ (
		"rep stosw\n\t"
		: "=&D" (d0), "=&c" (d1), "=a" (x)
		: "0" (dst), "1" (cnt), "2" (x)
		: "memory"
	);

}

/** Fill memory with bytes
 * Fill a given number of bytes (2nd argument)
 * at memory defined by 1st argument with the
 * word value defined by 3rd argument.
 *
 * @param dst Destination
 * @param cnt Number of bytes
 * @param x Value to fill
 */
static inline void memsetb(__address dst, size_t cnt, __u8 x)
{
	__u32 d0, d1;
	
	__asm__ __volatile__ (
		"rep stosb\n\t"
		: "=&D" (d0), "=&c" (d1), "=a" (x)
		: "0" (dst), "1" (cnt), "2" (x)
		: "memory"
	);

}

#endif

 /** @}
 */

