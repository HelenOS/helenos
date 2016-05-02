/*
 * Copyright (c) 2016 Jiri Svoboda
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

/** libdltest library handle */
void *handle;

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

/** Test calling a function that returns contents of a private initialized
 * variable.
 */
static bool test_dlfcn_dl_get_private_var(void)
{
	int (*p_dl_get_private_var)(void);
	int val;

	printf("Call dlsym/dl_get_private_var...\n");

	p_dl_get_private_var = dlsym(handle, "dl_get_private_var");
	if (p_dl_get_private_var == NULL) {
		printf("FAILED\n");
		return false;
	}

	val = p_dl_get_private_var();

	printf("Got %d, expected %d... ", val, dl_private_var_val);
	if (val != dl_private_var_val) {
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
	int val;

	printf("Call dlsym/dl_get_private_uvar...\n");

	p_dl_get_private_uvar = dlsym(handle, "dl_get_private_uvar");
	if (p_dl_get_private_uvar == NULL) {
		printf("FAILED\n");
		return false;
	}

	val = p_dl_get_private_uvar();

	printf("Got %d, expected %d... ", val, 0);
	if (val != 0) {
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
	int val;

	printf("Call dlsym/dl_get_public_var...\n");

	p_dl_get_public_var = dlsym(handle, "dl_get_public_var");
	if (p_dl_get_public_var == NULL) {
		printf("FAILED\n");
		return false;
	}

	val = p_dl_get_public_var();

	printf("Got %d, expected %d... ", val, dl_public_var_val);
	if (val != dl_public_var_val) {
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
	int val;

	printf("Call dlsym/dl_get_public_uvar...\n");

	p_dl_get_public_uvar = dlsym(handle, "dl_get_public_uvar");
	if (p_dl_get_public_uvar == NULL) {
		printf("FAILED\n");
		return false;
	}

	val = p_dl_get_public_uvar();

	printf("Got %d, expected %d... ", val, 0);
	if (val != 0) {
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
	int val;

	printf("Read dlsym/dl_public_var...\n");

	p_dl_public_var = dlsym(handle, "dl_public_var");
	if (p_dl_public_var == NULL) {
		printf("FAILED\n");
		return false;
	}

	val = *p_dl_public_var;

	printf("Got %d, expected %d... ", val, dl_public_var_val);
	if (val != dl_public_var_val) {
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
	int val;

	printf("Read dlsym/dl_public_uvar...\n");

	p_dl_public_uvar = dlsym(handle, "dl_public_uvar");
	if (p_dl_public_uvar == NULL) {
		printf("FAILED\n");
		return false;
	}

	val = *p_dl_public_uvar;

	printf("Got %d, expected %d... ", val, 0);
	if (val != 0) {
		printf("FAILED\n");
		return false;
	}

	printf("Passed\n");
	return true;
}

int main(int argc, char *argv[])
{

	printf("Dynamic linking test\n");

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

//	printf("dlclose()... ");
//	dlclose(handle);
//	printf("Passed\n");

	printf("All passed.\n");
	return 0;
}

/**
 * @}
 */
