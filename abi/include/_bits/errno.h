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
 */

#ifndef _BITS_ERRNO_H_
#define _BITS_ERRNO_H_

#include <_bits/native.h>
#include <_bits/decls.h>

#ifdef __OPAQUE_ERRNO__
#include <_bits/__opaque_handle.h>

__HELENOS_DECLS_BEGIN;
__opaque_handle(errno_t);
typedef errno_t sys_errno_t;
__HELENOS_DECLS_END;

#define __errno_t(val) ((errno_t) val)

#else

__HELENOS_DECLS_BEGIN;

/**
 * The type of <errno.h> constants. Normally, this is an alias for `int`,
 * but we support an alternative definition that allows us to verify
 * integrity of error handling without using external tools.
 */
typedef int errno_t;

/**
 * Same as `errno_t`, except represented as `sysarg_t`. Used in kernel in
 * places where error number is always passed, but the type must be `sysarg_t`.
 */
typedef sysarg_t sys_errno_t;

__HELENOS_DECLS_END;

/**
 * A C++-style "cast" to `errno_t`.
 * Used in <abi/errno.h> to define error constants. Normally, it doesn't do
 * anything at all.
 */
#define __errno_t(val) val

#endif

#endif
