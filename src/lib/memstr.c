/*
 * Copyright (C) 2001-2004 Jakub Jermar
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

#include <memstr.h>
#include <arch/types.h>


/** Copy block of memory
 *
 * Copy cnt bytes from src address to dst address.
 * The copying is done byte-by-byte. The source
 * and destination memory areas cannot overlap.
 *
 * @param src Origin address to copy from.
 * @param dst Origin address to copy to.
 * @param cnt Number of bytes to copy.
 *
 */
void _memcopy(__address src, __address dst, int cnt)
{
	int i;
	
	for (i=0; i<cnt; i++)
		*((__u8 *) (dst + i)) = *((__u8 *) (src + i));
}


/** Fill block of memory
 *
 * Fill cnt bytes at dst address with the value x.
 * The filling is done byte-by-byte.
 *
 * @param dst Origin address to fill.
 * @param cnt Number of bytes to fill.
 * @param x   Value to fill.
 *
 */
void _memsetb(__address dst, int cnt, __u8 x)
{
	int i;
	__u8 *p = (__u8 *) dst;
	
	for(i=0; i<cnt; i++)
		p[i] = x;
}
