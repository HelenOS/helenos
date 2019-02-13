/*
 * Copyright (c) 2017 CZ.NIC, z.s.p.o.
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

/*
 * Authors:
 *	Jiří Zárevúcky (jzr) <zarevucky.jiri@gmail.com>
 */

/** @addtogroup bits
 * @{
 */
/** @file
 * A bunch of type aliases HelenOS code uses.
 *
 * They were originally defined as either u/int32_t or u/int64_t,
 * specifically for each architecture, but in practice they are
 * currently assumed to be identical to u/intptr_t, so we do just that.
 */

#ifndef _BITS_NATIVE_H_
#define _BITS_NATIVE_H_

#include <inttypes.h>
#include <_bits/decls.h>

__HELENOS_DECLS_BEGIN;

typedef uintptr_t pfn_t;
typedef uintptr_t ipl_t;
typedef uintptr_t sysarg_t;
typedef intptr_t  native_t;

__HELENOS_DECLS_END;

#endif

/** @}
 */
