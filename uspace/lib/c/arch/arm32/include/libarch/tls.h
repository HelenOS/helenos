/*
 * Copyright (c) 2007 Pavel Jancik
 * Copyright (c) 2007 Michal Kebrt
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
