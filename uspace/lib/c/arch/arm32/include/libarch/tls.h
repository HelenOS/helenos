/*
 * SPDX-FileCopyrightText: 2007 Pavel Jancik
 * SPDX-FileCopyrightText: 2007 Michal Kebrt
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup libcarm32
 * @{
 */
/** @file
 */

#ifndef _LIBC_arm32_TLS_H_
#define _LIBC_arm32_TLS_H_

#include <stdint.h>

#define CONFIG_TLS_VARIANT_1

/** Offsets for accessing thread-local variables are shifted 8 bytes higher. */
#define ARCH_TP_OFFSET  (sizeof(tcb_t) - 8)

/** TCB (Thread Control Block) struct.
 *
 *  TLS starts just after this struct.
 */
typedef struct {
	void **dtv;
	void *pad;
	/** Fibril data. */
	void *fibril_data;
} tcb_t;

static inline void *__tcb_raw_get(void)
{
	uint8_t *ret;
	asm volatile ("mov %0, r9" : "=r" (ret));
	return ret;
}

static inline void __tcb_raw_set(void *tls)
{
	asm volatile ("mov r9, %0" :: "r" (tls));
}

/** Returns TLS address stored.
 *
 *  Implemented in assembly.
 *
 *  @return		TLS address stored in r9 register
 */
extern uintptr_t __aeabi_read_tp(void);

#endif

/** @}
 */
