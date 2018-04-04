/*
 * Copyright (c) 2005 Jakub Jermar
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

/** @addtogroup sparc64mm
 * @{
 */
/** @file
 */

#ifndef KERN_sparc64_sun4v_TTE_H_
#define KERN_sparc64_sun4v_TTE_H_

#define TTE_V_SHIFT	63	/**< valid */
#define TTE_TADDR_SHIFT	13	/**< target address */
#define TTE_CP_SHIFT	10	/**< cacheable physically */
#define TTE_CV_SHIFT	9	/**< caheable virtually */
#define TTE_P_SHIFT	8	/**< privileged */
#define TTE_EP_SHIFT	7	/**< execute permission */
#define TTE_W_SHIFT	6	/**< writable */
#define TTE_SZ_SHIFT	0	/**< size */

#define MMU_FLAG_ITLB	2	/**< operation applies to ITLB */
#define MMU_FLAG_DTLB	1	/**< operation applies to DTLB */

#ifndef __ASSEMBLER__

#include <stdint.h>

/** Translation Table Entry - Data. */
union tte_data {
	uint64_t value;
	struct {
		unsigned v : 1;		/**< Valid. */
		unsigned nfo : 1;	/**< No-Fault-Only. */
		unsigned soft : 6;	/**< Software defined field. */
		unsigned long ra : 43;	/**< Real address. */
		unsigned ie : 1;	/**< Invert endianess. */
		unsigned e : 1;		/**< Side-effect. */
		unsigned cp : 1;	/**< Cacheable in physically indexed cache. */
		unsigned cv : 1;	/**< Cacheable in virtually indexed cache. */
		unsigned p : 1;		/**< Privileged. */
		unsigned x : 1;		/**< Executable. */
		unsigned w : 1;		/**< Writable. */
		unsigned soft2 : 2;	/**< Software defined field. */
		unsigned size : 4;	/**< Page size. */
	} __attribute__((packed));
};

typedef union tte_data tte_data_t;

#define VA_TAG_PAGE_SHIFT	22

#endif /* !def __ASSEMBLER__ */

#endif

/** @}
 */
