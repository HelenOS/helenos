/*
 * Copyright (c) 2026 Jiri Svoboda
 * Copyright (c) 2018 Jakub Jermar
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

/** @addtogroup libc
 * @{
 */
/** @file
 */

#ifndef _LIBC_TIMEINT_H_
#define _LIBC_TIMEINT_H_

#include <_bits/decls.h>

__HELENOS_DECLS_BEGIN;

typedef long long sec_t;
typedef long long msec_t;
typedef long long usec_t;
typedef long long nsec_t;	/* good for +/- 292 years */

#define SEC2MSEC(s)	((s) * 1000ll)
#define SEC2USEC(s)	((s) * 1000000ll)
#define SEC2NSEC(s)	((s) * 1000000000ll)

#define MSEC2SEC(ms)	((ms) / 1000ll)
#define MSEC2USEC(ms)	((ms) * 1000ll)
#define MSEC2NSEC(ms)	((ms) * 1000000ll)

#define USEC2SEC(us)	((us) / 1000000ll)
#define USEC2MSEC(us)	((us) / 1000ll)
#define USEC2NSEC(us)	((us) * 1000ll)

#define NSEC2SEC(ns)	((ns) / 1000000000ll)
#define NSEC2MSEC(ns)	((ns) / 1000000ll)
#define NSEC2USEC(ns)	((ns) / 1000ll)

__HELENOS_DECLS_END;

#endif

/** @}
 */
