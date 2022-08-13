/*
 * SPDX-FileCopyrightText: 2016 Jiri Svoboda
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup libdltest
 * @{
 */
/** @file
 */

#include <fibril.h>
#include "libdltest.h"

/** Private initialized variable */
static int private_var = dl_private_var_val;
/** Private uninitialized variable */
static int private_uvar;

/** Public initialized variable */
int dl_public_var = dl_public_var_val;
/** Public uninitialized variable */
int dl_public_uvar;

/** Private initialized fibril-local variable */
static fibril_local int dl_private_fib_var = dl_private_fib_var_val;
/** Private uninitialized fibril-local variable */
static fibril_local int dl_private_fib_uvar;

/** Public initialized fibril-local variable */
fibril_local int dl_public_fib_var = dl_public_fib_var_val;
/** Public uninitialized fibril-local variable */
fibril_local int dl_public_fib_uvar;

/** Return constant value. */
int dl_get_constant(void)
{
	return dl_constant;
}

/** Return constant value by calling another function.
 *
 * This can be used to test dynamically linked call (via PLT) even in case
 * binaries are statically linked.
 */
int dl_get_constant_via_call(void)
{
	return dl_get_constant();
}

/** Return value of private initialized variable */
int dl_get_private_var(void)
{
	return private_var;
}

/** Return address of private initialized variable */
int *dl_get_private_var_addr(void)
{
	return &private_var;
}

/** Return value of private uninitialized variable */
int dl_get_private_uvar(void)
{
	return private_uvar;
}

/** Return vaddress of private uninitialized variable */
int *dl_get_private_uvar_addr(void)
{
	return &private_uvar;
}

/** Return value of public initialized variable */
int dl_get_public_var(void)
{
	return dl_public_var;
}

/** Return address of public initialized variable */
int *dl_get_public_var_addr(void)
{
	return &dl_public_var;
}

/** Return value of public uninitialized variable */
int dl_get_public_uvar(void)
{
	return dl_public_uvar;
}

/** Return address of public uninitialized variable */
int *dl_get_public_uvar_addr(void)
{
	return &dl_public_uvar;
}

/** Return value of private initialized fibril-local variable */
int dl_get_private_fib_var(void)
{
	return dl_private_fib_var;
}

/** Return address of private initialized fibril-local variable */
int *dl_get_private_fib_var_addr(void)
{
	return &dl_private_fib_var;
}

/** Return value of private uninitialized fibril-local variable */
int dl_get_private_fib_uvar(void)
{
	return dl_private_fib_uvar;
}

/** Return address of private uninitialized fibril-local variable */
int *dl_get_private_fib_uvar_addr(void)
{
	return &dl_private_fib_uvar;
}

/** Return value of public initialized fibril-local variable */
int dl_get_public_fib_var(void)
{
	return dl_public_fib_var;
}

/** Return value of public initialized fibril-local variable */
int *dl_get_public_fib_var_addr(void)
{
	return &dl_public_fib_var;
}

/** Return value of public uninitialized fibril-local variable */
int dl_get_public_fib_uvar(void)
{
	return dl_public_fib_uvar;
}

/** Return value of public uninitialized fibril-local variable */
int *dl_get_public_fib_uvar_addr(void)
{
	return &dl_public_fib_uvar;
}

/**
 * @}
 */
