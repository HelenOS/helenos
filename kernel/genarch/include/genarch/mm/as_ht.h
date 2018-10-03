/*
 * Copyright (c) 2006 Jakub Jermar
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

/** @addtogroup kernel_genarch_mm
 * @{
 */
/** @file
 */

#ifndef KERN_AS_HT_H_
#define KERN_AS_HT_H_

#include <mm/mm.h>
#include <adt/hash_table.h>
#include <typedefs.h>

typedef struct {
} as_genarch_t;

struct as;

typedef struct pte {
	ht_link_t link;		/**< Page hash table link. */
	struct as *as;		/**< Address space. */
	uintptr_t page;		/**< Virtual memory page. */
	uintptr_t frame;	/**< Physical memory frame. */
	unsigned g : 1;		/**< Global page. */
	unsigned x : 1;		/**< Execute. */
	unsigned w : 1;		/**< Writable. */
	unsigned k : 1;		/**< Kernel privileges required. */
	unsigned c : 1;		/**< Cacheable. */
	unsigned a : 1;		/**< Accessed. */
	unsigned d : 1;		/**< Dirty. */
	unsigned p : 1;		/**< Present. */
} pte_t;

#endif

/** @}
 */
