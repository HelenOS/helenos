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

#ifndef __sparc64_PAGE_H__
#define __sparc64_PAGE_H__

#include <mm/page.h>
#include <arch/mm/frame.h>
#include <arch/types.h>

#define PAGE_SIZE	FRAME_SIZE

#define KA2PA(x)	((__address) (x))
#define PA2KA(x)	((__address) (x))

#define SET_PTL0_ADDRESS_ARCH(x)	/**< To be removed as situation permits. */

/** Implementation of page hash table interface. */
#define HT_WIDTH_ARCH			20	/* 1M */
#define HT_HASH_ARCH(page, asid)	0
#define HT_COMPARE_ARCH(page, asid, t)	0
#define HT_SLOT_EMPTY_ARCH(t)		1
#define HT_INVALIDATE_SLOT_ARCH(t)
#define HT_GET_NEXT_ARCH(t)		0
#define HT_SET_NEXT_ARCH(t, s)
#define HT_SET_RECORD_ARCH(t, page, asid, frame, flags)

union page_address {
	__address address;
	struct {
		__u64 vpn : 51;		/**< Virtual Page Number. */
		unsigned offset : 13;	/**< Offset. */
	} __attribute__ ((packed));
};

typedef union page_address page_address_t;

extern void page_arch_init(void);

#endif
