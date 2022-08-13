/*
 * SPDX-FileCopyrightText: 2010 Martin Decky
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup libcabs32le
 * @{
 */
/** @file
 */

#ifndef _LIBC_abs32le_TLS_H_
#define _LIBC_abs32le_TLS_H_

#define CONFIG_TLS_VARIANT_2

#include <libc.h>
#include <stddef.h>

/* Some architectures store the value with an offset. Some do not. */
#define ARCH_TP_OFFSET 0

typedef struct {
	void *self;
	void *fibril_data;
} tcb_t;

static inline void __tcb_raw_set(void *tls)
{
	/* Usually a short assembly assigning to an ABI-defined register. */
}

static inline void *__tcb_raw_get(void)
{
	/* Usually a short assembly reading from an ABI-defined register. */
	return NULL;
}

extern uintptr_t __aeabi_read_tp(void);

#endif

/** @}
 */
