/*
 * SPDX-FileCopyrightText: 2006 Ondrej Palkovsky
 *
 * SPDX-License-Identifier: BSD-3-Clause
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
