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

#include <assert.h>
#include <stddef.h>
#include <align.h>
#include <tls.h>
#include <stdlib.h>
#include <str.h>
#include <macros.h>
#include <elf/elf.h>

#ifdef CONFIG_RTLD
#include <rtld/rtld.h>
#endif

#if !defined(CONFIG_TLS_VARIANT_1) && !defined(CONFIG_TLS_VARIANT_2)
#error Unknown TLS variant.
#endif

/** Get address of static TLS block */
void *tls_get(void)
{
#ifdef CONFIG_RTLD
	assert(runtime_env == NULL);
#endif

	const elf_segment_header_t *tls =
	    elf_get_phdr(__executable_start, PT_TLS);

	if (tls == NULL)
		return NULL;

#ifdef CONFIG_TLS_VARIANT_1
	return (uint8_t *)__tcb_get() + ALIGN_UP(sizeof(tcb_t), tls->p_align);
#else /* CONFIG_TLS_VARIANT_2 */
	return (uint8_t *)__tcb_get() - ALIGN_UP(tls->p_memsz, tls->p_align);
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

#ifdef CONFIG_RTLD
	if (runtime_env != NULL)
		return rtld_tls_make(runtime_env);
#endif

	const elf_segment_header_t *tls =
	    elf_get_phdr(__executable_start, PT_TLS);
	if (tls == NULL)
		return NULL;

	uintptr_t bias = elf_get_bias(__executable_start);
	size_t align = max(tls->p_align, _Alignof(tcb_t));

#ifdef CONFIG_TLS_VARIANT_1
	tcb = tls_alloc_arch(
	    ALIGN_UP(sizeof(tcb_t), align) + tls->p_memsz, align);
	data = (void *) tcb + ALIGN_UP(sizeof(tcb_t), align);
#else
	tcb = tls_alloc_arch(
	    ALIGN_UP(tls->p_memsz, align) + sizeof(tcb_t), align);
	data = (void *) tcb - ALIGN_UP(tls->p_memsz, tls->p_align);
#endif

	/*
	 * Copy thread local data from the initialization image.
	 */
	memcpy(data, (void *)(tls->p_vaddr + bias), tls->p_filesz);
	/*
	 * Zero out the thread local uninitialized data.
	 */
	memset(data + tls->p_filesz, 0, tls->p_memsz - tls->p_filesz);

	return tcb;
}

void tls_free(tcb_t *tcb)
{
#ifdef CONFIG_RTLD
	free(tcb->dtv);

	if (runtime_env != NULL) {
		tls_free_arch(tcb, runtime_env->tls_size, runtime_env->tls_align);
		return;
	}
#endif
	const elf_segment_header_t *tls =
	    elf_get_phdr(__executable_start, PT_TLS);

	assert(tls != NULL);
	tls_free_arch(tcb,
	    ALIGN_UP(tls->p_memsz, tls->p_align) + sizeof(tcb_t),
	    max(tls->p_align, _Alignof(tcb_t)));
}

#ifdef CONFIG_TLS_VARIANT_1
/** Allocate TLS variant 1 data structures.
 *
 * @param data 		Start of TLS section. This is an output argument.
 * @param size		Size of tdata + tbss section.
 * @return 		Pointer to tcb_t structure.
 */
tcb_t *tls_alloc_variant_1(size_t size, size_t align)
{
	tcb_t *tcb = memalign(align, size);
	if (!tcb)
		return NULL;
	memset(tcb, 0, sizeof(tcb_t));
	return tcb;
}

/** Free TLS variant I data structures.
 *
 * @param tcb		Pointer to TCB structure.
 * @param size		This argument is ignored.
 */
void tls_free_variant_1(tcb_t *tcb, size_t size, size_t align)
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
 * @param align		Alignment of thread local data.
 * @return		Pointer to TCB structure.
 */
tcb_t *tls_alloc_variant_2(size_t size, size_t align)
{
	void *data = memalign(align, size);
	if (data == NULL)
		return NULL;

	tcb_t *tcb = (tcb_t *) (data + size - sizeof(tcb_t));
	memset(tcb, 0, sizeof(tcb_t));
	tcb->self = tcb;
	return tcb;
}

/** Free TLS variant II data structures.
 *
 * @param tcb		Pointer to TCB structure.
 * @param size		Size of thread local data.
 * @param align		Alignment of thread local data.
 */
void tls_free_variant_2(tcb_t *tcb, size_t size, size_t align)
{
	if (tcb != NULL) {
		void *start = ((void *) tcb) + sizeof(tcb_t) - size;
		free(start);
	}
}
#endif

/** @}
 */
