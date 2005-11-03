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

#ifndef __ia64_REGISTER_H__
#define __ia64_REGISTER_H__

#include <arch/types.h>

#define CR_IVR_MASK	0xf
#define PSR_I_MASK	0x4000

/** External Interrupt Vector Register */
union cr_ivr {
	__u8  vector;
	__u64 value;
};

typedef union cr_ivr cr_ivr_t;

/** Task Priority Register */
union cr_tpr {
	struct {
		unsigned : 4;
		unsigned mic: 4;		/**< Mask Interrupt Class. */
		unsigned : 8;
		unsigned mmi: 1;		/**< Mask Maskable Interrupts. */
	} __attribute__ ((packed));
	__u64 value;
};

typedef union cr_tpr cr_tpr_t;

/** Interval Timer Vector */
union cr_itv {
	struct {
		unsigned vector : 8;
		unsigned : 4;
		unsigned : 1;
		unsigned : 3;
		unsigned m : 1;			/**< Mask. */
	} __attribute__ ((packed));
	__u64 value;
};

typedef union cr_itv cr_itv_t;

#endif
