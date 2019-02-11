/*
 * Copyright (c) 2007 Jakub Jermar
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
 */

#ifndef _LIBC_TLS_H_
#define _LIBC_TLS_H_

#include <libarch/tls.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

static inline void __tcb_reset(void)
{
	__tcb_raw_set(NULL);
}

static inline void __tcb_set(tcb_t *tcb)
{
	__tcb_raw_set((uint8_t *)tcb + ARCH_TP_OFFSET);
}

static inline tcb_t *__tcb_get(void)
{
	return (tcb_t *)((uint8_t *)__tcb_raw_get() - ARCH_TP_OFFSET);
}

/*
 * The TP register is supposed to be zero when the thread is first created
 * by the kernel. We use this for some debugging assertions.
 */
static inline bool __tcb_is_set(void)
{
	return __tcb_raw_get() != NULL;
}

/** DTV Generation number - equals vector length */
#define DTV_GN(dtv) (((uintptr_t *)(dtv))[0])

extern tcb_t *tls_make(const void *);
extern tcb_t *tls_make_initial(const void *);
extern tcb_t *tls_alloc_arch(size_t, size_t);
extern void tls_free(tcb_t *);
extern void tls_free_arch(tcb_t *, size_t, size_t);
extern void *tls_get(void);

#ifdef CONFIG_TLS_VARIANT_1
extern tcb_t *tls_alloc_variant_1(size_t, size_t);
extern void tls_free_variant_1(tcb_t *, size_t, size_t);
#endif

#ifdef CONFIG_TLS_VARIANT_2
extern tcb_t *tls_alloc_variant_2(size_t, size_t);
extern void tls_free_variant_2(tcb_t *, size_t, size_t);
#endif

#endif

/** @}
 */
