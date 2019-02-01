/*
 * Copyright (c) 2006 Ondrej Palkovsky
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

/** @addtogroup libcmips32
 * @{
 */
/** @file
 * @ingroup libcmips32
 */

/* TLS for MIPS is described in http://www.linux-mips.org/wiki/NPTL */

#ifndef _LIBC_mips32_TLS_H_
#define _LIBC_mips32_TLS_H_

/*
 * FIXME: Note that the use of variant I contradicts the observations made in
 * the note below. Nevertheless the scheme we have used for allocating and
 * deallocatin TLS corresponds to TLS variant I.
 */
#define CONFIG_TLS_VARIANT_1

#include <libc.h>

/*
 * I did not find any specification (neither MIPS nor PowerPC), but
 * as I found it
 * - it uses Variant II
 * - TCB is at Address(First TLS Block)+0x7000.
 * - DTV is at Address(First TLS Block)+0x8000
 * - What would happen if the TLS data was larger then 0x7000?
 * - The linker never accesses DTV directly, has the second definition any
 *   sense?
 * We will make it this way:
 * - TCB is at TP-0x7000-sizeof(tcb)
 * - No assumption about DTV etc., but it will not have a fixed address
 */
#define ARCH_TP_OFFSET (0x7000 + sizeof(tcb_t))

typedef struct {
	void *fibril_data;
} tcb_t;

static inline void __tcb_raw_set(void *tls)
{
	/* Move tls to K1 */
	asm volatile ("add $27, %0, $0" :: "r" (tls));
}

static inline void *__tcb_raw_get(void)
{
	void *retval;
	asm volatile ("add %0, $27, $0" : "=r" (retval));
	return retval;
}

#endif

/** @}
 */
