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

#define MEMSET_DST   4
#define MEMSET_VAL   8
#define MEMSET_SIZE  12

#define MEMCPY_DST   4
#define MEMCPY_SRC   8
#define MEMCPY_SIZE  12

/* Fill memory with byte pattern
 *
 * This is a conventional memset().
 *
 * @param MEMSET_DST(%esp)  Destination address.
 * @param MEMSET_VAL(%esp)  Value to fill.
 * @param MEMSET_SIZE(%esp) Size.
 *
 * @return MEMSET_DST(%esp).
 *
 */
memset:
	movl %edi, %edx  /* save %edi */
	
	movl MEMSET_DST(%esp), %edi
	movl MEMSET_VAL(%esp), %ecx
	
	/* Create byte pattern */
	movb %cl, %ch
	movw %cx, %ax
	shll $16, %eax
	orw %cx, %ax
	
	movl MEMSET_SIZE(%esp), %ecx
	shrl $2, %ecx  /* size / 4 */
	
	/* Write whole words */
	rep stosl
	
	movl MEMSET_SIZE(%esp), %ecx
	andl $3, %ecx  /* size % 4 */
	jz 0f
	
	/* Copy the rest byte by byte */
	rep stosb
	
	0:
	
		movl %edx, %edi
		
		/* MEMSET_DST(%esp), success */
		movl MEMSET_DST(%esp), %eax
		ret

/** Copy memory to/from userspace.
 *
 * This is a conventional memcpy().
 *
 * @param MEMCPY_DST(%esp)  Destination address.
 * @param MEMCPY_SRC(%esp)  Source address.
 * @param MEMCPY_SIZE(%esp) Size.
 *
 * @return MEMCPY_DST(%esp).
 *
 */
memcpy:
	movl %edi, %edx  /* save %edi */
	movl %esi, %eax  /* save %esi */
	
	movl MEMCPY_SIZE(%esp), %ecx
	shrl $2, %ecx  /* size / 4 */
	
	movl MEMCPY_DST(%esp), %edi
	movl MEMCPY_SRC(%esp), %esi
	
	/* Copy whole words */
	rep movsl
	
	movl MEMCPY_SIZE(%esp), %ecx
	andl $3, %ecx  /* size % 4 */
	jz 0f
	
	/* Copy the rest byte by byte */
	rep movsb
	
	0:
	
		movl %edx, %edi
		movl %eax, %esi
		
		/* MEMCPY_DST(%esp), success */
		movl MEMCPY_DST(%esp), %eax
		ret
