/*
 * SPDX-FileCopyrightText: 2017 CZ.NIC, z.s.p.o.
 *
 * SPDX-License-Identifier: BSD-3-Clause
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
