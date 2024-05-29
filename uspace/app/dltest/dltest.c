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

/** @addtogroup dltest
 * @{
 */

/**
 * @file
 * @brief	Test dynamic linking
 */

#include <dlfcn.h>
#include <libdltest.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <str.h>

/** libdltest library handle */
static void *handle;

/** If true, do not run dlfcn tests */
static bool no_dlfcn = false;

/** Test dlsym() function */
static bool test_dlsym(void)
{
	int (*p_dl_get_constant)(void);

	printf("dlsym()... ");
	p_dl_get_constant = dlsym(handle, "dl_get_constant");
	if (p_dl_get_constant == NULL) {
		printf("FAILED\n");
		return false;
	}

	printf("Passed\n");
	return true;
}

/** Test calling function that returns a constant */
static bool test_dlfcn_dl_get_constant(void)
{
	int (*p_dl_get_constant)(void);
	int val;

	printf("Call dlsym/dl_get_constant...\n");

	p_dl_get_constant = dlsym(handle, "dl_get_constant");
	if (p_dl_get_constant == NULL) {
		printf("FAILED\n");
		return false;
	}

	val = p_dl_get_constant();

	printf("Got %d, expected %d... ", val, dl_constant);
	if (val != dl_constant) {
		printf("FAILED\n");
		return false;
	}

	printf("Passed\n");
	return true;
}

/** Test calling function that calls a function that returns a constant */
static bool test_dlfcn_dl_get_constant_via_call(void)
{
	int (*p_dl_get_constant)(void);
	int val;

	printf("Call dlsym/dl_get_constant_via_call...\n");

	p_dl_get_constant = dlsym(handle, "dl_get_constant_via_call");
	if (p_dl_get_constant == NULL) {
		printf("FAILED\n");
		return false;
	}

	val = p_dl_get_constant();

	printf("Got %d, expected %d... ", val, dl_constant);
	if (val != dl_constant) {
		printf("FAILED\n");
		return false;
	}

	printf("Passed\n");
	return true;
}

/** Test calling a function that returns contents of a private initialized
 * variable.
 */
static bool test_dlfcn_dl_get_private_var(void)
{
	int (*p_dl_get_private_var)(void);
	int *(*p_dl_get_private_var_addr)(void);
	int val;

	printf("Call dlsym/dl_get_private_var...\n");

	p_dl_get_private_var = dlsym(handle, "dl_get_private_var");
	if (p_dl_get_private_var == NULL) {
		printf("FAILED\n");
		return false;
	}

	p_dl_get_private_var_addr = dlsym(handle, "dl_get_private_var_addr");
	if (p_dl_get_private_var_addr == NULL) {
		printf("FAILED\n");
		return false;
	}

	val = p_dl_get_private_var();

	printf("Got %d, expected %d... ", val, dl_private_var_val);
	if (val != dl_private_var_val) {
		printf("dl_get_private_var_addr -> %p\n",
		    p_dl_get_private_var_addr());
		printf("FAILED\n");
		return false;
	}

	printf("Passed\n");
	return true;
}

/** Test calling a function that returns contents of a private uninitialized
 * variable.
 */
static bool test_dlfcn_dl_get_private_uvar(void)
{
	int (*p_dl_get_private_uvar)(void);
	int *(*p_dl_get_private_uvar_addr)(void);
	int val;

	printf("Call dlsym/dl_get_private_uvar...\n");

	p_dl_get_private_uvar = dlsym(handle, "dl_get_private_uvar");
	if (p_dl_get_private_uvar == NULL) {
		printf("FAILED\n");
		return false;
	}

	p_dl_get_private_uvar_addr = dlsym(handle, "dl_get_private_uvar_addr");
	if (p_dl_get_private_uvar_addr == NULL) {
		printf("FAILED\n");
		return false;
	}

	val = p_dl_get_private_uvar();

	printf("Got %d, expected %d... ", val, 0);
	if (val != 0) {
		printf("dl_get_private_uvar_addr -> %p\n",
		    p_dl_get_private_uvar_addr());
		printf("FAILED\n");
		return false;
	}

	printf("Passed\n");
	return true;
}

/** Test calling a function that returns the contents of a public initialized
 * variable.
 */
static bool test_dlfcn_dl_get_public_var(void)
{
	int (*p_dl_get_public_var)(void);
	int *(*p_dl_get_public_var_addr)(void);
	int val;

	printf("Call dlsym/dl_get_public_var...\n");

	p_dl_get_public_var = dlsym(handle, "dl_get_public_var");
	if (p_dl_get_public_var == NULL) {
		printf("FAILED\n");
		return false;
	}

	p_dl_get_public_var_addr = dlsym(handle, "dl_get_public_var_addr");
	if (p_dl_get_public_var_addr == NULL) {
		printf("FAILED\n");
		return false;
	}

	val = p_dl_get_public_var();

	printf("Got %d, expected %d... ", val, dl_public_var_val);
	if (val != dl_public_var_val) {
		printf("dl_get_public_var_addr -> %p\n",
		    p_dl_get_public_var_addr());
		printf("FAILED\n");
		return false;
	}

	printf("Passed\n");
	return true;
}

/** Test calling a function that returns the contents of a public uninitialized
 * variable.
 */
static bool test_dlfcn_dl_get_public_uvar(void)
{
	int (*p_dl_get_public_uvar)(void);
	int *(*p_dl_get_public_uvar_addr)(void);
	int val;

	printf("Call dlsym/dl_get_public_uvar...\n");

	p_dl_get_public_uvar = dlsym(handle, "dl_get_public_uvar");
	if (p_dl_get_public_uvar == NULL) {
		printf("FAILED\n");
		return false;
	}

	p_dl_get_public_uvar_addr = dlsym(handle, "dl_get_public_uvar_addr");
	if (p_dl_get_public_uvar_addr == NULL) {
		printf("FAILED\n");
		return false;
	}

	val = p_dl_get_public_uvar();

	printf("Got %d, expected %d... ", val, 0);
	if (val != 0) {
		printf("dl_get_public_uvar_addr -> %p\n",
		    p_dl_get_public_uvar_addr());
		printf("FAILED\n");
		return false;
	}

	printf("Passed\n");
	return true;
}

/** Test directly reading a public initialized variable whose address was
 * obtained using dlsym.
 */
static bool test_dlfcn_read_public_var(void)
{
	int *p_dl_public_var;
	int *(*p_dl_get_public_var_addr)(void);
	int val;

	printf("Read dlsym/dl_public_var...\n");

	p_dl_public_var = dlsym(handle, "dl_public_var");
	if (p_dl_public_var == NULL) {
		printf("FAILED\n");
		return false;
	}

	p_dl_get_public_var_addr = dlsym(handle, "dl_get_public_var_addr");
	if (p_dl_get_public_var_addr == NULL) {
		printf("FAILED\n");
		return false;
	}

	val = *p_dl_public_var;

	printf("Got %d, expected %d... ", val, dl_public_var_val);
	if (val != dl_public_var_val) {
		printf("&dl_public_var = %p, "
		    "dl_get_public_var_addr -> %p\n",
		    p_dl_public_var, p_dl_get_public_var_addr());
		printf("FAILED\n");
		return false;
	}

	printf("Passed\n");
	return true;
}

/** Test directly reading a public uninitialized variable whose address was
 * obtained using dlsym.
 */
static bool test_dlfcn_read_public_uvar(void)
{
	int *p_dl_public_uvar;
	int *(*p_dl_get_public_uvar_addr)(void);
	int val;

	printf("Read dlsym/dl_public_uvar...\n");

	p_dl_public_uvar = dlsym(handle, "dl_public_uvar");
	if (p_dl_public_uvar == NULL) {
		printf("FAILED\n");
		return false;
	}

	p_dl_get_public_uvar_addr = dlsym(handle, "dl_get_public_uvar_addr");
	if (p_dl_get_public_uvar_addr == NULL) {
		printf("FAILED\n");
		return false;
	}

	val = *p_dl_public_uvar;

	printf("Got %d, expected %d... ", val, 0);
	if (val != 0) {
		printf("&dl_public_uvar = %p, "
		    "dl_get_public_uvar_addr -> %p\n",
		    p_dl_public_uvar, p_dl_get_public_uvar_addr());
		printf("FAILED\n");
		return false;
	}

	printf("Passed\n");
	return true;
}

#ifndef STATIC_EXE

/** Test calling a function that returns contents of a private initialized
 * fibril-local variable.
 */
static bool test_dlfcn_dl_get_private_fib_var(void)
{
	int (*p_dl_get_private_fib_var)(void);
	int *(*p_dl_get_private_fib_var_addr)(void);
	int val;

	printf("Call dlsym/dl_get_private_fib_var...\n");

	p_dl_get_private_fib_var = dlsym(handle, "dl_get_private_fib_var");
	if (p_dl_get_private_fib_var == NULL) {
		printf("FAILED\n");
		return false;
	}

	p_dl_get_private_fib_var_addr = dlsym(handle, "dl_get_private_fib_var_addr");
	if (p_dl_get_private_fib_var_addr == NULL) {
		printf("FAILED\n");
		return false;
	}

	val = p_dl_get_private_fib_var();

	printf("Got %d, expected %d... ", val, dl_private_fib_var_val);
	if (val != dl_private_fib_var_val) {
		printf("dl_get_private_fib_var_addr -> %p\n",
		    p_dl_get_private_fib_var_addr());
		printf("FAILED\n");
		return false;
	}

	printf("Passed\n");
	return true;
}

/** Test calling a function that returns contents of a private uninitialized
 * fibril-local variable.
 */
static bool test_dlfcn_dl_get_private_fib_uvar(void)
{
	int (*p_dl_get_private_fib_uvar)(void);
	int *(*p_dl_get_private_fib_uvar_addr)(void);
	int val;

	printf("Call dlsym/dl_get_private_fib_uvar...\n");

	p_dl_get_private_fib_uvar = dlsym(handle, "dl_get_private_fib_uvar");
	if (p_dl_get_private_fib_uvar == NULL) {
		printf("FAILED\n");
		return false;
	}

	p_dl_get_private_fib_uvar_addr = dlsym(handle, "dl_get_private_fib_uvar_addr");
	if (p_dl_get_private_fib_uvar_addr == NULL) {
		printf("FAILED\n");
		return false;
	}

	val = p_dl_get_private_fib_uvar();

	printf("Got %d, expected %d... ", val, 0);
	if (val != 0) {
		printf("dl_get_private_fib_uvar_addr -> %p\n",
		    p_dl_get_private_fib_uvar_addr());
		printf("FAILED\n");
		return false;
	}

	printf("Passed\n");
	return true;
}

/** Test calling a function that returns the contents of a public initialized
 * fibril-local variable.
 */
static bool test_dlfcn_dl_get_public_fib_var(void)
{
	int (*p_dl_get_public_fib_var)(void);
	int *(*p_dl_get_public_fib_var_addr)(void);
	int val;

	printf("Call dlsym/dl_get_public_fib_var...\n");

	p_dl_get_public_fib_var = dlsym(handle, "dl_get_public_fib_var");
	if (p_dl_get_public_fib_var == NULL) {
		printf("FAILED\n");
		return false;
	}

	p_dl_get_public_fib_var_addr = dlsym(handle, "dl_get_public_fib_var_addr");
	if (p_dl_get_public_fib_var_addr == NULL) {
		printf("FAILED\n");
		return false;
	}

	val = p_dl_get_public_fib_var();

	printf("Got %d, expected %d... ", val, dl_public_fib_var_val);
	if (val != dl_public_fib_var_val) {
		printf("dl_get_public_fib_var_addr -> %p\n",
		    p_dl_get_public_fib_var_addr());
		printf("FAILED\n");
		return false;
	}

	printf("Passed\n");
	return true;
}

/** Test calling a function that returns the contents of a public uninitialized
 * fibril-local variable.
 */
static bool test_dlfcn_dl_get_public_fib_uvar(void)
{
	int (*p_dl_get_public_fib_uvar)(void);
	int *(*p_dl_get_public_fib_uvar_addr)(void);
	int val;

	printf("Call dlsym/dl_get_public_fib_uvar...\n");

	p_dl_get_public_fib_uvar = dlsym(handle, "dl_get_public_fib_uvar");
	if (p_dl_get_public_fib_uvar == NULL) {
		printf("FAILED\n");
		return false;
	}

	p_dl_get_public_fib_uvar_addr = dlsym(handle, "dl_get_public_fib_uvar_addr");
	if (p_dl_get_public_fib_uvar_addr == NULL) {
		printf("FAILED\n");
		return false;
	}

	val = p_dl_get_public_fib_uvar();

	printf("Got %d, expected %d... ", val, 0);
	if (val != 0) {
		printf("dl_get_public_fib_uvar_addr -> %p\n",
		    p_dl_get_public_fib_uvar_addr());
		printf("FAILED\n");
		return false;
	}

	printf("Passed\n");
	return true;
}

/** Test directly reading a public initialized fibril-local variable
 * whose address was obtained using dlsym.
 */
static bool test_dlfcn_read_public_fib_var(void)
{
	int *p_dl_public_fib_var;
	int *(*p_dl_get_public_fib_var_addr)(void);
	int val;

	printf("Read dlsym/dl_public_fib_var...\n");

	p_dl_public_fib_var = dlsym(handle, "dl_public_fib_var");
	if (p_dl_public_fib_var == NULL) {
		printf("FAILED\n");
		return false;
	}

	p_dl_get_public_fib_var_addr = dlsym(handle, "dl_get_public_fib_var_addr");
	if (p_dl_get_public_fib_var_addr == NULL) {
		printf("FAILED\n");
		return false;
	}

	val = *p_dl_public_fib_var;

	printf("Got %d, expected %d... ", val, dl_public_fib_var_val);
	if (val != dl_public_fib_var_val) {
		printf("&dl_public_fib_var = %p, "
		    "dl_get_public_fib_var_addr -> %p\n",
		    p_dl_public_fib_var, p_dl_get_public_fib_var_addr());
		printf("FAILED\n");
		return false;
	}

	printf("Passed\n");
	return true;
}

/** Test directly reading a public uninitialized fibril-local variable
 * whose address was obtained using dlsym.
 */
static bool test_dlfcn_read_public_fib_uvar(void)
{
	int *p_dl_public_fib_uvar;
	int *(*p_dl_get_public_fib_uvar_addr)(void);
	int val;

	printf("Read dlsym/dl_public_fib_uvar...\n");

	p_dl_public_fib_uvar = dlsym(handle, "dl_public_fib_uvar");
	if (p_dl_public_fib_uvar == NULL) {
		printf("FAILED\n");
		return false;
	}

	p_dl_get_public_fib_uvar_addr = dlsym(handle, "dl_get_public_fib_uvar_addr");
	if (p_dl_get_public_fib_uvar_addr == NULL) {
		printf("FAILED\n");
		return false;
	}

	val = *p_dl_public_fib_uvar;

	printf("Got %d, expected %d... ", val, 0);
	if (val != 0) {
		printf("&dl_public_fib_uvar = %p, "
		    "dl_get_public_fib_uvar_addr -> %p\n",
		    p_dl_public_fib_uvar, p_dl_get_public_fib_uvar_addr());
		printf("FAILED\n");
		return false;
	}

	printf("Passed\n");
	return true;
}

#endif /* STATIC_EXE */

#ifdef DLTEST_LINKED

/** Test if we can read the correct value of a public pointer variable.
 *
 * dl_public_ptr_var is initialized in libdltest to point to dl_public_var.
 * This is done using a relocation. The main program (unless compiled with
 * PIC or PIE) will contain a copy of dl_public_ptr_var. This needs
 * to be copied using a COPY relocation. The relocations in the main
 * program need to be processed after the relocations in the shared
 * libraries (so that we copy the correct value).
 */
static bool test_public_ptr_var(void)
{
	int *ptr;

	printf("Read dl_public_ptr_var directly...\n");
	ptr = dl_public_ptr_var;

	if (ptr != &dl_public_var) {
		printf("FAILED\n");
		return false;
	}

	printf("Passed\n");
	return true;
}

/** Test directly calling function that returns a constant */
static bool test_lnk_dl_get_constant(void)
{
	int val;

	printf("Call linked dl_get_constant...\n");

	val = dl_get_constant();

	printf("Got %d, expected %d... ", val, dl_constant);
	if (val != dl_constant) {
		printf("FAILED\n");
		return false;
	}

	printf("Passed\n");
	return true;
}

/** Test directly calling function that calls a function that returns a constant */
static bool test_lnk_dl_get_constant_via_call(void)
{
	int val;

	printf("Call linked dl_get_constant_via_call...\n");

	val = dl_get_constant_via_call();

	printf("Got %d, expected %d... ", val, dl_constant);
	if (val != dl_constant) {
		printf("FAILED\n");
		return false;
	}

	printf("Passed\n");
	return true;
}

/** Test dircetly calling a function that returns contents of a private
 * initialized variable.
 */
static bool test_lnk_dl_get_private_var(void)
{
	int val;

	printf("Call linked dl_get_private_var...\n");

	val = dl_get_private_var();

	printf("Got %d, expected %d... ", val, dl_private_var_val);
	if (val != dl_private_var_val) {
		printf("dl_get_private_var_addr -> %p\n",
		    dl_get_private_var_addr());
		printf("FAILED\n");
		return false;
	}

	printf("Passed\n");
	return true;
}

/** Test dircetly calling a function that returns contents of a private
 * uninitialized variable.
 */
static bool test_lnk_dl_get_private_uvar(void)
{
	int val;

	printf("Call linked dl_get_private_uvar...\n");

	val = dl_get_private_uvar();

	printf("Got %d, expected %d... ", val, 0);
	if (val != 0) {
		printf("dl_get_private_uvar_addr -> %p\n",
		    dl_get_private_uvar_addr());
		printf("FAILED\n");
		return false;
	}

	printf("Passed\n");
	return true;
}

/** Test directly calling a function that returns the contents of a public
 * initialized variable.
 */
static bool test_lnk_dl_get_public_var(void)
{
	int val;

	printf("Call linked dl_get_public_var...\n");

	val = dl_get_public_var();

	printf("Got %d, expected %d... ", val, dl_public_var_val);
	if (val != dl_public_var_val) {
		printf("dl_get_public_var_addr -> %p\n",
		    dl_get_public_var_addr());
		printf("FAILED\n");
		return false;
	}

	printf("Passed\n");
	return true;
}

/** Test directly calling a function that returns the contents of a public
 * uninitialized variable.
 */
static bool test_lnk_dl_get_public_uvar(void)
{
	int val;

	printf("Call linked dl_get_public_uvar...\n");

	val = dl_get_public_uvar();

	printf("Got %d, expected %d... ", val, 0);
	if (val != 0) {
		printf("dl_get_public_uvar_addr -> %p\n",
		    dl_get_public_uvar_addr());
		printf("FAILED\n");
		return false;
	}

	printf("Passed\n");
	return true;
}

/** Test directly reading a public initialized variable. */
static bool test_lnk_read_public_var(void)
{
	int val;

	printf("Read linked dl_public_var...\n");

	val = dl_public_var;

	printf("Got %d, expected %d... ", val, dl_public_var_val);
	if (val != dl_public_var_val) {
		printf("&dl_public_var = %p, dl_get_public_var_addr -> %p\n",
		    &dl_public_var, dl_get_public_var_addr());
		printf("FAILED\n");
		return false;
	}

	printf("Passed\n");
	return true;
}

/** Test directly reading a public uninitialized variable. */
static bool test_lnk_read_public_uvar(void)
{
	int val;

	printf("Read linked dl_public_uvar...\n");

	val = dl_public_uvar;

	printf("Got %d, expected %d... ", val, 0);
	if (val != 0) {
		printf("&dl_public_uvar = %p, dl_get_public_uvar_addr -> %p\n",
		    &dl_public_uvar, dl_get_public_uvar_addr());
		printf("FAILED\n");
		return false;
	}

	printf("Passed\n");
	return true;
}

/** Test dircetly calling a function that returns contents of a private
 * initialized fibril-local variable.
 */
static bool test_lnk_dl_get_private_fib_var(void)
{
	int val;

	printf("Call linked dl_get_private_fib_var...\n");

	val = dl_get_private_fib_var();

	printf("Got %d, expected %d... ", val, dl_private_fib_var_val);
	if (val != dl_private_fib_var_val) {
		printf("dl_get_private_fib_var_addr -> %p\n",
		    dl_get_private_fib_var_addr());
		printf("FAILED\n");
		return false;
	}

	printf("Passed\n");
	return true;
}

/** Test dircetly calling a function that returns contents of a private
 * uninitialized fibril-local variable.
 */
static bool test_lnk_dl_get_private_fib_uvar(void)
{
	int val;

	printf("Call linked dl_get_private_fib_uvar...\n");

	val = dl_get_private_fib_uvar();

	printf("Got %d, expected %d... ", val, 0);
	if (val != 0) {
		printf("dl_get_private_fib_uvar_addr -> %p\n",
		    dl_get_private_fib_var_addr());
		printf("FAILED\n");
		return false;
	}

	printf("Passed\n");
	return true;
}

/** Test directly calling a function that returns the contents of a public
 * initialized fibril-local variable.
 */
static bool test_lnk_dl_get_public_fib_var(void)
{
	int val;

	printf("Call linked dl_get_public_fib_var...\n");

	val = dl_get_public_fib_var();

	printf("Got %d, expected %d... ", val, dl_public_fib_var_val);
	if (val != dl_public_fib_var_val) {
		printf("dl_get_public_fib_var_addr -> %p\n",
		    dl_get_public_fib_var_addr());
		printf("FAILED\n");
		return false;
	}

	printf("Passed\n");
	return true;
}

/** Test directly calling a function that returns the contents of a public
 * uninitialized fibril-local variable.
 */
static bool test_lnk_dl_get_public_fib_uvar(void)
{
	int val;

	printf("Call linked dl_get_public_fib_uvar...\n");

	val = dl_get_public_fib_uvar();

	printf("Got %d, expected %d... ", val, 0);
	if (val != 0) {
		printf("dl_get_public_fib_uvar_addr -> %p\n",
		    dl_get_public_fib_uvar_addr());
		printf("FAILED\n");
		return false;
	}

	printf("Passed\n");
	return true;
}

/** Test directly reading a public initialized fibril-local variable. */
static bool test_lnk_read_public_fib_var(void)
{
	int val;

	printf("Read linked dl_public_fib_var...\n");

	val = dl_public_fib_var;

	printf("Got %d, expected %d... ", val, dl_public_fib_var_val);
	if (val != dl_public_fib_var_val) {
		printf("&dl_public_fib_var = %p, "
		    "dl_get_public_fib_var_addr -> %p\n",
		    &dl_public_fib_var, dl_get_public_fib_var_addr());
		printf("FAILED\n");
		return false;
	}

	printf("Passed\n");
	return true;
}

/** Test directly reading a public uninitialized fibril-local variable. */
static bool test_lnk_read_public_fib_uvar(void)
{
	int val;

	printf("Read linked dl_public_fib_uvar...\n");

	val = dl_public_fib_uvar;

	printf("Got %d, expected %d... ", val, 0);
	if (val != 0) {
		printf("&dl_public_fib_uvar = %p, "
		    "dl_get_public_fib_uvar_addr -> %p\n",
		    &dl_public_fib_uvar, dl_get_public_fib_uvar_addr());
		printf("FAILED\n");
		return false;
	}

	printf("Passed\n");
	return true;
}

#endif /* DLTEST_LINKED */

static int test_dlfcn(void)
{
	printf("dlopen()... ");
	handle = dlopen("libdltest.so.0", 0);
	if (handle == NULL) {
		printf("FAILED\n");
		return 1;
	}

	printf("Passed\n");

	if (!test_dlsym())
		return 1;

	if (!test_dlfcn_dl_get_constant())
		return 1;

	if (!test_dlfcn_dl_get_constant_via_call())
		return 1;

	if (!test_dlfcn_dl_get_private_var())
		return 1;

	if (!test_dlfcn_dl_get_private_uvar())
		return 1;

	if (!test_dlfcn_dl_get_public_var())
		return 1;

	if (!test_dlfcn_dl_get_public_uvar())
		return 1;

	if (!test_dlfcn_read_public_var())
		return 1;

	if (!test_dlfcn_read_public_uvar())
		return 1;

#ifndef STATIC_EXE

	if (!test_dlfcn_dl_get_private_fib_var())
		return 1;

	if (!test_dlfcn_dl_get_private_fib_uvar())
		return 1;

	if (!test_dlfcn_dl_get_public_fib_var())
		return 1;

	if (!test_dlfcn_dl_get_public_fib_uvar())
		return 1;

	if (!test_dlfcn_read_public_fib_var())
		return 1;

	if (!test_dlfcn_read_public_fib_uvar())
		return 1;
#endif /* STATIC_EXE */

#if 0
	printf("dlclose()... ");
	dlclose(handle);
	printf("Passed\n");
#endif

	return 0;
}

#ifdef DLTEST_LINKED

static int test_lnk(void)
{
	if (!test_lnk_dl_get_constant())
		return 1;

	if (!test_lnk_dl_get_constant_via_call())
		return 1;

	if (!test_lnk_dl_get_private_var())
		return 1;

	if (!test_lnk_dl_get_private_uvar())
		return 1;

	if (!test_lnk_dl_get_public_var())
		return 1;

	if (!test_lnk_dl_get_public_uvar())
		return 1;

	if (!test_lnk_read_public_var())
		return 1;

	if (!test_lnk_read_public_uvar())
		return 1;

	if (!test_public_ptr_var())
		return 1;

	if (!test_lnk_dl_get_private_fib_var())
		return 1;

	if (!test_lnk_dl_get_private_fib_uvar())
		return 1;

	if (!test_lnk_dl_get_public_fib_var())
		return 1;

	if (!test_lnk_dl_get_public_fib_uvar())
		return 1;

	if (!test_lnk_read_public_fib_var())
		return 1;

	if (!test_lnk_read_public_fib_uvar())
		return 1;

	return 0;
}

#endif /* DLTEST_LINKED */

static void print_syntax(void)
{
	fprintf(stderr, "syntax: dltest [-n]\n");
	fprintf(stderr, "\t-n Do not run dlfcn tests\n");
}

int main(int argc, char *argv[])
{
	printf("Dynamic linking test\n");

	if (argc > 1) {
		if (argc > 2) {
			print_syntax();
			return 1;
		}

		if (str_cmp(argv[1], "-n") == 0) {
			no_dlfcn = true;
		} else {
			print_syntax();
			return 1;
		}
	}

	if (!no_dlfcn) {
		if (test_dlfcn() != 0)
			return 1;
	}

#ifdef DLTEST_LINKED
	if (test_lnk() != 0)
		return 1;
#endif

	printf("All passed.\n");
	return 0;
}

/**
 * @}
 */
