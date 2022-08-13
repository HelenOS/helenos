/*
 * SPDX-FileCopyrightText: 2006 Ondrej Palkovsky
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup libcamd64
 * @{
 */
/** @file
 */

#ifndef _LIBC_amd64_TLS_H_
#define _LIBC_amd64_TLS_H_

#define CONFIG_TLS_VARIANT_2

#define ARCH_TP_OFFSET 0

#include <libc.h>

typedef struct {
	void *self;
	void *fibril_data;
	void **dtv;
	void *pad;
} tcb_t;

static inline void __tcb_raw_set(void *tls)
{
	asm volatile ("movq %0, %%fs:0" :: "r" (tls));
}

static inline void *__tcb_raw_get(void)
{
	void *retval;
	asm volatile ("movq %%fs:0, %0" : "=r" (retval));
	return retval;
}

#endif

/** @}
 */
