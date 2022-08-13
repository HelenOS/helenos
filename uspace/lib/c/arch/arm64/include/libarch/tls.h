/*
 * SPDX-FileCopyrightText: 2015 Petr Pavlu
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup libcarm64
 * @{
 */
/** @file
 * @brief Thread-local storage.
 */

#ifndef _LIBC_arm64_TLS_H_
#define _LIBC_arm64_TLS_H_

#define CONFIG_TLS_VARIANT_1

/** Offsets for accessing thread-local variables are shifted 16 bytes higher. */
#define ARCH_TP_OFFSET  (sizeof(tcb_t) - 16)

/** TCB (Thread Control Block) struct.
 *
 * TLS starts just after this struct.
 */
typedef struct {
	/** Fibril data. */
	void *fibril_data;
} tcb_t;

static inline void __tcb_raw_set(void *tls)
{
	asm volatile ("msr tpidr_el0, %[tls]" : : [tls] "r" (tls));
}

static inline void *__tcb_raw_get(void)
{
	void *retval;
	asm volatile ("mrs %[tls], tpidr_el0" : [tls] "=r" (retval));
	return retval;
}

#endif

/** @}
 */
