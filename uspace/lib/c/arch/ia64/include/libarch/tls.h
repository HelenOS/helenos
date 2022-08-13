/*
 * SPDX-FileCopyrightText: 2006 Jakub Jermar
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup libcia64
 * @{
 */
/** @file
 */

#ifndef _LIBC_ia64_TLS_H_
#define _LIBC_ia64_TLS_H_

#define CONFIG_TLS_VARIANT_1

#define ARCH_TP_OFFSET 0

/* This structure must be exactly 16 bytes long */
typedef struct {
	void **dtv;
	void *fibril_data;
} tcb_t;

static inline void __tcb_raw_set(void *tcb)
{
	asm volatile ("mov r13 = %0\n" : : "r" (tcb) : "r13");
}

static inline void *__tcb_raw_get(void)
{
	void *retval;
	asm volatile ("mov %0 = r13\n" : "=r" (retval));
	return retval;
}

#endif

/** @}
 */
