/*
 * Copyright (c) 2006 Jakub Jermar
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

#include <arch/arch.h>

#define DCACHE_SIZE		(16 * 1024)
#define DCACHE_LINE_SIZE	32	

#define DCACHE_TAG_SHIFT	2

.register %g2, #scratch
.register %g3, #scratch

/** Flush the whole D-cache. */
.global dcache_flush
dcache_flush:
	set (DCACHE_SIZE - DCACHE_LINE_SIZE), %g1
	stxa %g0, [%g1] ASI_DCACHE_TAG
0:	membar #Sync
	subcc %g1, DCACHE_LINE_SIZE, %g1
	bnz,pt %xcc, 0b
	stxa %g0, [%g1] ASI_DCACHE_TAG
	retl
	membar #Sync

/** Flush only D-cache lines of one virtual color.
 *
 * @param o0	Virtual color to be flushed.
 */
.global dcache_flush_color
dcache_flush_color:
	mov (DCACHE_SIZE / DCACHE_LINE_SIZE) / 2, %g1
	set DCACHE_SIZE / 2, %g2
	sllx %g2, %o0, %g2
	sub %g2, DCACHE_LINE_SIZE, %g2
0:	stxa %g0, [%g2] ASI_DCACHE_TAG
	membar #Sync
	subcc %g1, 1, %g1
	bnz,pt %xcc, 0b
	sub %g2, DCACHE_LINE_SIZE, %g2
	retl
	nop

/** Flush only D-cache lines of one virtual color and one tag.
 *
 * @param o0	Virtual color to lookup the tag.
 * @param o1	Tag of the cachelines to be flushed.
 */
.global dcache_flush_tag
dcache_flush_tag:
	mov (DCACHE_SIZE / DCACHE_LINE_SIZE) / 2, %g1
	set DCACHE_SIZE / 2, %g2
	sllx %g2, %o0, %g2
	sub %g2, DCACHE_LINE_SIZE, %g2
0:	ldxa [%g2] ASI_DCACHE_TAG, %g3
	srlx %g3, DCACHE_TAG_SHIFT, %g3
	cmp %g3, %o1
	bnz 1f
	nop
	stxa %g0, [%g2] ASI_DCACHE_TAG
	membar #Sync
1:	subcc %g1, 1, %g1
	bnz,pt %xcc, 0b
	sub %g2, DCACHE_LINE_SIZE, %g2
	retl
	nop
