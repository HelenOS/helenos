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

#ifndef __ia32_MEMSTR_H__
#define __ia32_MEMSTR_H__

extern void memsetw(__address dst, size_t cnt, __u16 x);
extern void memsetb(__address dst, size_t cnt, __u8 x);

extern int memcmp(__address src, __address dst, int cnt);

/** Copy memory
 *
 * Copy a given number of bytes (3rd argument)
 * from the memory location defined by 2nd argument
 * to the memory location defined by 1st argument.
 * The memory areas cannot overlap.
 *
 * @param destination
 * @param source
 * @param number of bytes
 * @return destination
 */
static inline void * memcpy(void * dst, const void * src, size_t cnt)
{
        __u32 d0, d1, d2;

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
                : "0" (cnt / 4), "g" (cnt), "1" ((__u32) dst), "2" ((__u32) src)
                : "memory");

        return dst;
}



#endif
