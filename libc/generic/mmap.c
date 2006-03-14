/*
 * Copyright (C) 2006 Jakub Jermar
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

#include <libc.h>
#include <unistd.h>

/** Mremap syscall */
void *mremap(void *address, size_t size, unsigned long flags)
{
	return (void *) __SYSCALL3(SYS_MREMAP, (sysarg_t ) address, (sysarg_t) size, (sysarg_t) flags);
}


static size_t heapsize = 0;
/* Start of heap linker symbol */
extern char _heap;

/** Sbrk emulation 
 *
 * @param size New area that should be allocated or negative, 
               if it should be shrinked
 * @return Pointer to newly allocated area
 */
void *sbrk(ssize_t incr)
{
	void *res;
	/* Check for invalid values */
	if (incr < 0 && -incr > heapsize)
		return NULL;
	/* Check for too large value */
	if (incr > 0 && incr+heapsize < heapsize)
		return NULL;
	/* Check for too small values */
	if (incr < 0 && incr+heapsize > heapsize)
		return NULL;

	res = mremap(&_heap, heapsize + incr,0);
	if (!res)
		return NULL;
	res = (void *)&_heap + incr;
	heapsize += incr;
	return res;
}
