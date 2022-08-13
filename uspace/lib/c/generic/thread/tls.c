/*
 * SPDX-FileCopyrightText: 2006 Jakub Jermar
 *
 * SPDX-License-Identifier: BSD-3-Clause
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
#include <stdalign.h>
#include <stddef.h>
#include <align.h>
#include <tls.h>
#include <stdlib.h>
#include <str.h>
#include <macros.h>
#include <elf/elf.h>
#include <as.h>

#include <libarch/config.h>

#ifdef CONFIG_RTLD
#include <rtld/rtld.h>
#endif

#include "../private/libc.h"

#if !defined(CONFIG_TLS_VARIANT_1) && !defined(CONFIG_TLS_VARIANT_2)
#error Unknown TLS variant.
#endif

static ptrdiff_t _tcb_data_offset(void)
{
	const elf_segment_header_t *tls =
	    elf_get_phdr(__progsymbols.elfstart, PT_TLS);

	size_t tls_align = tls ? tls->p_align : 1;

#ifdef CONFIG_TLS_VARIANT_1
	return ALIGN_UP((ptrdiff_t) sizeof(tcb_t), tls_align);
#else
	size_t tls_size = tls ? tls->p_memsz : 0;
	return -ALIGN_UP((ptrdiff_t) tls_size, max(tls_align, alignof(tcb_t)));
#endif
}

/** Get address of static TLS block */
void *tls_get(void)
{
#ifdef CONFIG_RTLD
	assert(runtime_env == NULL);
#endif
	return (uint8_t *)__tcb_get() + _tcb_data_offset();
}

static tcb_t *tls_make_generic(const void *elf, void *(*alloc)(size_t, size_t))
{
	assert(!elf_get_phdr(elf, PT_DYNAMIC));
#ifdef CONFIG_RTLD
	assert(runtime_env == NULL);
#endif

	const elf_segment_header_t *tls = elf_get_phdr(elf, PT_TLS);
	size_t tls_size = tls ? tls->p_memsz : 0;
	size_t tls_align = tls ? tls->p_align : 1;

	/*
	 * We don't currently support alignment this big,
	 * and neither should we need to.
	 */
	assert(tls_align <= PAGE_SIZE);

#ifdef CONFIG_TLS_VARIANT_1
	size_t alloc_size =
	    ALIGN_UP(sizeof(tcb_t), tls_align) + tls_size;
#else
	size_t alloc_size =
	    ALIGN_UP(tls_size, max(tls_align, alignof(tcb_t))) + sizeof(tcb_t);
#endif

	void *area = alloc(max(tls_align, alignof(tcb_t)), alloc_size);
	if (!area)
		return NULL;

#ifdef CONFIG_TLS_VARIANT_1
	tcb_t *tcb = area;
	uint8_t *data = (uint8_t *)tcb + _tcb_data_offset();
	memset(tcb, 0, sizeof(*tcb));
#else
	uint8_t *data = area;
	tcb_t *tcb = (tcb_t *) (data - _tcb_data_offset());
	memset(tcb, 0, sizeof(tcb_t));
	tcb->self = tcb;
#endif

	if (!tls)
		return tcb;

	uintptr_t bias = elf_get_bias(elf);

	/* Copy thread local data from the initialization image. */
	memcpy(data, (void *)(tls->p_vaddr + bias), tls->p_filesz);
	/* Zero out the thread local uninitialized data. */
	memset(data + tls->p_filesz, 0, tls->p_memsz - tls->p_filesz);

	return tcb;
}

static void *early_alloc(size_t align, size_t alloc_size)
{
	assert(align <= PAGE_SIZE);
	alloc_size = ALIGN_UP(alloc_size, PAGE_SIZE);

	void *area = as_area_create(AS_AREA_ANY, alloc_size,
	    AS_AREA_READ | AS_AREA_WRITE | AS_AREA_CACHEABLE, AS_AREA_UNPAGED);
	if (area == AS_MAP_FAILED)
		return NULL;
	return area;
}

/** Same as tls_make(), but uses as_area_create() instead of memalign().
 *  Only used in __libc_main() if the program was created by the kernel.
 */
tcb_t *tls_make_initial(const void *elf)
{
	return tls_make_generic(elf, early_alloc);
}

/** Create TLS (Thread Local Storage) data structures.
 *
 * @return Pointer to TCB.
 */
tcb_t *tls_make(const void *elf)
{
	// TODO: Always use rtld.

#ifdef CONFIG_RTLD
	if (runtime_env != NULL)
		return rtld_tls_make(runtime_env);
#endif

	return tls_make_generic(elf, memalign);
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
	    elf_get_phdr(__progsymbols.elfstart, PT_TLS);

	assert(tls != NULL);
	tls_free_arch(tcb,
	    ALIGN_UP(tls->p_memsz, tls->p_align) + sizeof(tcb_t),
	    max(tls->p_align, alignof(tcb_t)));
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
