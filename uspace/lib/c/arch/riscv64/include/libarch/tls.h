/*
 * SPDX-FileCopyrightText: 2016 Martin Decky
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup libcriscv64
 * @{
 */
/** @file
 */

#ifndef _LIBC_riscv64_TLS_H_
#define _LIBC_riscv64_TLS_H_

#define CONFIG_TLS_VARIANT_2

#include <libc.h>

/* Some architectures store the value with an offset. Some do not. */
#define ARCH_TP_OFFSET 0

typedef struct {
	void *self;
	void *fibril_data;
} tcb_t;

static inline void __tcb_raw_set(void *tls)
{
	// TODO
}

static inline void *__tcb_raw_get(void)
{
	// TODO
	return 0;
}

#endif

/** @}
 */
