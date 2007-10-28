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

/** @addtogroup libcsparc64 sparc64
 * @ingroup lc
 * @{
 */
/** @file
 *
 */

#include <thread.h>
#include <malloc.h>
#include <align.h>

/*
 * sparc64 uses thread-local storage data structures, variant II, as described
 * in:
 * 	Drepper U.: ELF Handling For Thread-Local Storage, 2005
 */

/** Allocate TLS variant II data structures for a thread.
 *
 * Only static model is supported.
 *
 * @param data Pointer to pointer to thread local data. This is actually an
 * 	output argument.
 * @param size Size of thread local data.
 * @return Pointer to TCB structure.
 */
tcb_t * __alloc_tls(void **data, size_t size)
{
	tcb_t *tcb;
	
	size = ALIGN_UP(size, &_tls_alignment);
	*data = memalign(&_tls_alignment, sizeof(tcb_t) + size);

	tcb = (tcb_t *) (*data + size);
	tcb->self = tcb;

	return tcb;
}

/** Free TLS variant II data structures of a thread.
 *
 * Only static model is supported.
 *
 * @param tcb Pointer to TCB structure.
 * @param size Size of thread local data.
 */
void __free_tls_arch(tcb_t *tcb, size_t size)
{
	size = ALIGN_UP(size, &_tls_alignment);
	void *start = ((void *) tcb) - size;
	free(start);
}

/** @}
 */
