/*
 * Copyright (C) 2005 Jakub Jermar
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

#ifndef __ia64_INTERRUPT_H__
#define __ia64_INTERRUPT_H__

#include <arch/types.h>

/** External interrupt vectors. */
#define INTERRUPT_TIMER		0
#define INTERRUPT_SPURIOUS	15

#define EOI	0		/**< The actual value doesn't matter. */

struct exception_regdump {
	__address ar_bsp;
	__address ar_bspstore;
	__u64 ar_rnat;
	__u64 ar_ifs;
	__u64 ar_pfs;
	__u64 ar_rsc;
	__address cr_ifa;
	__u64 cr_isr;
	__address cr_iipa;
	__u64 cr_ips;
	__address cr_iip;
	__u64 pr;
} __attribute__ ((packed));

extern void *ivt;

extern void general_exception(__u64 vector, struct exception_regdump *pstate);
extern void break_instruction(__u64 vector, struct exception_regdump *pstate);
extern void universal_handler(__u64 vector, struct exception_regdump *pstate);
extern void external_interrupt(__u64 vector, struct exception_regdump *pstate);

#endif
