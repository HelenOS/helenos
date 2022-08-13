/*
 * SPDX-FileCopyrightText: 2006 Ondrej Palkovsky
 * SPDX-FileCopyrightText: 2006 Jakub Jermar
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup libcsparc64
 * @{
 */
/**
 * @file
 * @brief	sparc64 TLS functions.
 */

#ifndef _LIBC_sparc64_TLS_H_
#define _LIBC_sparc64_TLS_H_

#define CONFIG_TLS_VARIANT_2

#define ARCH_TP_OFFSET 0

typedef struct {
	void *self;
	void *fibril_data;
	void **dtv;
	void *pad;
} tcb_t;

static inline void __tcb_raw_set(void *tcb)
{
	asm volatile ("mov %0, %%g7\n" : : "r" (tcb) : "g7");
}

static inline void *__tcb_raw_get(void)
{
	void *retval;
	asm volatile ("mov %%g7, %0\n" : "=r" (retval));
	return retval;
}

#endif

/** @}
 */
