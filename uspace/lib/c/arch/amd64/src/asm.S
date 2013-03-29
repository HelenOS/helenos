/*
 * Copyright (c) 2013 Martin Decky
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

.text
.global memset
.global memcpy

#define MEMSET_DST   %rdi
#define MEMSET_VAL   %rsi
#define MEMSET_SIZE  %rdx

#define MEMCPY_DST   %rdi
#define MEMCPY_SRC   %rsi
#define MEMCPY_SIZE  %rdx

/* Fill memory with byte pattern
 *
 * This is a conventional memset().
 *
 * @param MEMSET_DST  Destination address.
 * @param MEMSET_VAL  Value to fill.
 * @param MEMSET_SIZE Size.
 *
 * @return MEMSET_DST.
 *
 */
memset:
	movq MEMSET_DST, %r8    /* save %rdi */
	
	/* Create byte pattern */
	movzbl %sil, %esi       /* MEMSET_VAL */
	movabs $0x0101010101010101, %rax
	imulq %rsi, %rax
	
	movq MEMSET_SIZE, %rcx
	shrq $3, %rcx           /* size / 8 */
	
	rep stosq               /* store as much as possible word by word */
	
	movq MEMSET_SIZE, %rcx
	andq $7, %rcx           /* size % 8 */
	jz 0f
	
	rep stosb               /* store the rest byte by byte */
	
	0:
		movq %r8, %rax
		ret                 /* return MEMCPY_SRC, success */

/** Copy memory from/to userspace.
 *
 * This is a conventional memcpy().
 *
 * @param MEMCPY_DST  Destination address.
 * @param MEMCPY_SRC  Source address.
 * @param MEMCPY_SIZE Number of bytes to copy.
 *
 * @retrun MEMCPY_DST on success, 0 on failure.
 *
 */
memcpy:
	movq MEMCPY_DST, %rax
	
	movq MEMCPY_SIZE, %rcx
	shrq $3, %rcx           /* size / 8 */
	
	rep movsq               /* copy as much as possible word by word */
	
	movq MEMCPY_SIZE, %rcx
	andq $7, %rcx           /* size % 8 */
	jz 0f
	
	rep movsb               /* copy the rest byte by byte */
	
	0:
		ret                 /* return MEMCPY_SRC, success */
