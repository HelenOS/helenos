/*
 * SPDX-FileCopyrightText: 2006 Martin Decky
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup libcppc32
 * @{
 */
/** @file
 */

#ifndef _LIBC_ppc32_TLS_H_
#define _LIBC_ppc32_TLS_H_

#define CONFIG_TLS_VARIANT_1

#include <libc.h>

#define ARCH_TP_OFFSET (0x7000 + sizeof(tcb_t))

typedef struct {
	void **dtv;
	void *pad;
	void *fibril_data;
} tcb_t;

static inline void __tcb_raw_set(void *tls)
{
	asm volatile ("mr %%r2, %0\n" :: "r" (tls));
}

static inline void *__tcb_raw_get(void)
{
	void *retval;
	asm volatile ("mr %0, %%r2\n" : "=r" (retval));
	return retval;
}

#endif

/** @}
 */
