/*
 * Copyright (c) 2024 Jiri Svoboda
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
/** Public variable initialized with the address of a symbol */
int *dl_public_ptr_var = &dl_public_var;
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
