/*
 * SPDX-FileCopyrightText: 2007 Jakub Jermar
 *
 * SPDX-License-Identifier: BSD-3-Clause
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
