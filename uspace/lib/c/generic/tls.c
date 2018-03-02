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

/** @addtogroup libc
 * @{
 */
/** @file
 *
 * Support for thread-local storage, as described in:
 * 	Drepper U.: ELF Handling For Thread-Local Storage, 2005
 */

#include <stddef.h>
#include <align.h>
#include <tls.h>
#include <stdlib.h>
#include <str.h>

#ifdef CONFIG_RTLD
#include <rtld/rtld.h>
#endif

size_t tls_get_size(void)
{
#ifdef CONFIG_RTLD
	if (runtime_env != NULL)
		return runtime_env->tls_size;
#endif
	return &_tbss_end - &_tdata_start;
}

/** Get address of static TLS block */
void *tls_get(void)
{
#ifdef CONFIG_TLS_VARIANT_1
	return (uint8_t *)__tcb_get() + sizeof(tcb_t);
#else /* CONFIG_TLS_VARIANT_2 */
	return (uint8_t *)__tcb_get() - tls_get_size();
#endif
}

/** Create TLS (Thread Local Storage) data structures.
 *
 * @return Pointer to TCB.
 */
tcb_t *tls_make(void)
{
	void *data;
	tcb_t *tcb;
	size_t tls_size = &_tbss_end - &_tdata_start;

#ifdef CONFIG_RTLD
	if (runtime_env != NULL)
		return rtld_tls_make(runtime_env);
#endif

	tcb = tls_alloc_arch(&data, tls_size);
	if (!tcb)
		return NULL;

	/*
	 * Copy thread local data from the initialization image.
	 */
	memcpy(data, &_tdata_start, &_tdata_end - &_tdata_start);
	/*
	 * Zero out the thread local uninitialized data.
	 */
	memset(data + (&_tbss_start - &_tdata_start), 0,
	    &_tbss_end - &_tbss_start);

	return tcb;
}

void tls_free(tcb_t *tcb)
{
#ifdef CONFIG_RTLD
	free(tcb->dtv);
#endif
	tls_free_arch(tcb, tls_get_size());
}

#ifdef CONFIG_TLS_VARIANT_1
/** Allocate TLS variant 1 data structures.
 *
 * @param data 		Start of TLS section. This is an output argument.
 * @param size		Size of tdata + tbss section.
 * @return 		Pointer to tcb_t structure.
 */
tcb_t *tls_alloc_variant_1(void **data, size_t size)
{
	tcb_t *tcb;

	tcb = malloc(sizeof(tcb_t) + size);
	if (!tcb)
		return NULL;

	*data = ((void *) tcb) + sizeof(tcb_t);
#ifdef CONFIG_RTLD
	tcb->dtv = NULL;
#endif

	return tcb;
}

/** Free TLS variant I data structures.
 *
 * @param tcb		Pointer to TCB structure.
 * @param size		This argument is ignored.
 */
void tls_free_variant_1(tcb_t *tcb, size_t size)
{
	free(tcb);
}
#endif

#ifdef CONFIG_TLS_VARIANT_2
/** Allocate TLS variant II data structures.
 *
 * @param data		Pointer to pointer to thread local data. This is
 * 			actually an output argument.
 * @param size		Size of thread local data.
 * @return		Pointer to TCB structure.
 */
tcb_t * tls_alloc_variant_2(void **data, size_t size)
{
	tcb_t *tcb;

	size = ALIGN_UP(size, &_tls_alignment);
	*data = memalign((uintptr_t) &_tls_alignment, sizeof(tcb_t) + size);
	if (*data == NULL)
		return NULL;
	tcb = (tcb_t *) (*data + size);
	tcb->self = tcb;
#ifdef CONFIG_RTLD
	tcb->dtv = NULL;
#endif

	return tcb;
}

/** Free TLS variant II data structures.
 *
 * @param tcb		Pointer to TCB structure.
 * @param size		Size of thread local data.
 */
void tls_free_variant_2(tcb_t *tcb, size_t size)
{
	size = ALIGN_UP(size, &_tls_alignment);
	void *start = ((void *) tcb) - size;
	free(start);
}
#endif

/** @}
 */
