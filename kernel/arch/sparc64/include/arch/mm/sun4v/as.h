/*
 * Copyright (c) 2005 Jakub Jermar
 * Copyright (c) 2009 Pavel Rimsky
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

/** @addtogroup kernel_sparc64_mm
 * @{
 */
/** @file
 */

#ifndef KERN_sparc64_sun4v_AS_H_
#define KERN_sparc64_sun4v_AS_H_

#include <arch/mm/tte.h>
#include <arch/mm/tsb.h>

#define KERNEL_ADDRESS_SPACE_SHADOWED_ARCH  1
#define KERNEL_SEPARATE_PTL0_ARCH           0

#define KERNEL_ADDRESS_SPACE_START_ARCH  UINT64_C(0x0000000000000000)
#define KERNEL_ADDRESS_SPACE_END_ARCH    UINT64_C(0xffffffffffffffff)
#define USER_ADDRESS_SPACE_START_ARCH    UINT64_C(0x0000000000000000)
#define USER_ADDRESS_SPACE_END_ARCH      UINT64_C(0xffffffffffffffff)

#ifdef CONFIG_TSB

/**
 * TTE Tag.
 *
 * Even though for sun4v the format of the TSB Tag states that the context
 * field has 16 bits, the T1 CPU still only supports 13-bit contexts and the
 * three most significant bits are always zero.
 */
typedef union tte_tag {
	uint64_t value;
	struct {
		unsigned : 3;
		unsigned context : 13;	/**< Software ASID. */
		unsigned : 6;
		uint64_t va_tag : 42;	/**< Virtual address bits <63:22>. */
	} __attribute__((packed));
} tte_tag_t;

/** TSB entry. */
typedef struct tsb_entry {
	tte_tag_t tag;
	tte_data_t data;
} __attribute__((packed)) tsb_entry_t;

typedef struct {
	tsb_descr_t tsb_description;
} as_arch_t;

#else

typedef struct {
} as_arch_t;

#endif /* CONFIG_TSB */

#include <genarch/mm/as_ht.h>

#ifdef CONFIG_TSB
#include <arch/mm/tsb.h>
#define as_invalidate_translation_cache(as, page, cnt) \
	tsb_invalidate((as), (page), (cnt))
#else
#define as_invalidate_translation_cache(as, page, cnt)
#endif

extern void as_arch_init(void);

#endif

/** @}
 */
