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

#ifndef __sparc64_TTE_H__
#define __sparc64_TTE_H__

#include <arch/types.h>

/** Translation Table Entry - Tag. */
union tte_tag {
	__u64 value;
	struct {
		unsigned g : 1;		/**< Global. */
		unsigned : 2;		/**< Reserved. */
		unsigned context : 13;	/**< Context identifier. */
		unsigned : 6;		/**< Reserved. */
		__u64 va_tag : 42;	/**< Virtual Address Tag, bits 63:22. */
	} __attribute__ ((packed));
};

typedef union tte_tag tte_tag_t;

/** Translation Table Entry - Data. */
union tte_data {
	__u64 value;
	struct {
		unsigned v : 1;		/**< Valid. */
		unsigned size : 2;	/**< Page size of this entry. */
		unsigned nfo : 1;	/**< No-Fault-Only. */
		unsigned ie : 1;	/**< Invert Endianness. */
		unsigned soft2 : 9;	/**< Software defined field. */
		unsigned diag : 9;	/**< Diagnostic data. */
		unsigned pfn : 28;	/**< Physical Address bits, bits 40:13. */
		unsigned soft : 6;	/**< Software defined field. */
		unsigned l : 1;		/**< Lock. */
		unsigned cp : 1;	/**< Cacheable in physically indexed cache. */
		unsigned cv : 1;	/**< Cacheable in virtually indexed cache. */
		unsigned e : 1;		/**< Side-effect. */
		unsigned p : 1;		/**< Privileged. */
		unsigned w : 1;		/**< Writable. */
		unsigned g : 1;		/**< Global. */
	} __attribute__ ((packed));
};

typedef union tte_data tte_data_t;

#endif
