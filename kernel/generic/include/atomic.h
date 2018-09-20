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

/** @addtogroup generic
 * @{
 */
/** @file
 */

#ifndef KERN_ATOMIC_H_
#define KERN_ATOMIC_H_

#include <stdbool.h>
#include <typedefs.h>
#include <stdatomic.h>

// TODO: Remove.
// Before <stdatomic.h> was available, there was only one atomic type
// equivalent to atomic_size_t. This means that in some places, atomic_t can
// be replaced with a more appropriate type (e.g. atomic_bool for flags or
// a signed type for potentially signed values).
// So atomic_t should be replaced with the most appropriate type on a case by
// case basis, and after there are no more uses, remove this type.
typedef atomic_size_t atomic_t;

#define atomic_predec(val) \
	(atomic_fetch_sub((val), 1) - 1)

#define atomic_preinc(val) \
	(atomic_fetch_add((val), 1) + 1)

#define atomic_postdec(val) \
	atomic_fetch_sub((val), 1)

#define atomic_postinc(val) \
	atomic_fetch_add((val), 1)

#define atomic_dec(val) \
	((void) atomic_fetch_sub(val, 1))

#define atomic_inc(val) \
	((void) atomic_fetch_add(val, 1))

#define local_atomic_exchange(var_addr, new_val) \
	atomic_exchange_explicit( \
	    (_Atomic typeof(*(var_addr)) *) (var_addr), \
	    (new_val), memory_order_relaxed)

#endif

/** @}
 */
